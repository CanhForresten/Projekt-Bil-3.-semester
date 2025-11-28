#include "lineSensor.h"

// Define path to GPIO chip device
#define GPIO_CHIP_PATH "/dev/gpiochip0"

KY033::KY033(int gpio1, int gpio2, int gpio3, int gpio4, AGVstate *state) : state_(state)
{
    std::lock_guard lock(state_->controller_mtx);
    // Open gpiochip0 as read-only device
    chip_fd = open(GPIO_CHIP_PATH, O_RDWR);

    if (chip_fd < 0)
    {
        perror("open");
        return;
    }

    // ---- signal as input ----

    // Populate structure to be safe
    memset(&signal_req, 0, sizeof(signal_req));
 
    // Specify GPIO lines we want to access
    signal_req.lineoffsets[0] = gpio1;
    signal_req.lineoffsets[1] = gpio2;
    signal_req.lineoffsets[2] = gpio3;

    // Access all lines as inputs
    signal_req.flags = GPIOHANDLE_REQUEST_INPUT;

    // Number of lines we want to access
    signal_req.lines = 3;

    // Set a name to be shown when gpioinfo is called
    strcpy(signal_req.consumer_label, "KY-033 signal");

    // Use ioctl to get GPIO line handle from kernel
    int ret_signal = ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &signal_req);

    if (ret_signal < 0)
    {
        perror("ioctl");
        close(chip_fd);
        return;
    }

    // ---- station signal as input ----

    // Populate structure to be safe
    memset(&station_signal_req, 0, sizeof(station_signal_req));
 
    // Specify GPIO lines we want to access
    station_signal_req.lineoffsets[0] = gpio4;


    // Access all lines as inputs
    station_signal_req.flags = GPIOHANDLE_REQUEST_INPUT;

    // Number of lines we want to access
    station_signal_req.lines = 1;

    // Set a name to be shown when gpioinfo is called
    strcpy(station_signal_req.consumer_label, "KY-033 signal");

    // Use ioctl to get GPIO line handle from kernel
    int station_ret_signal = ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &station_signal_req);

    if (station_ret_signal < 0)
    {
        perror("ioctl");
        close(chip_fd);
        return;
    }

    std::cout << "chip_fd = " << chip_fd << std::endl;
    std::cout << "signal_req.fd = " << signal_req.fd << std::endl;
    std::cout << "station_signal_req.fd = " << station_signal_req.fd << std::endl;
    std::cout << "state ptr = " << state_ << std::endl;


}

KY033::~KY033()
{
    close(chip_fd);
    close(signal_req.fd);
    close(station_signal_req.fd);
}

void KY033::lineSensorThread()
{
    startPosition();
    while(state_->running){
        onLine();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Lille pause
    }   
}


void KY033::onLine()
{
    {
        std::unique_lock lock(state_->controller_mtx);
        state_->cv_sensor.wait(lock,[&](){return state_->driving; });
    }
    
    readLineGPIO();

    int tracker1 = data_in.values[0]; // Left
    int tracker2 = data_in.values[1]; // center
    int tracker3 = data_in.values[2]; // right

    readStationGPIO();
    int trackerStation = station_data_in.values[0];

    std::cout << "t1: " << tracker1 << " t2: " << tracker2 << " t3: " << tracker3 
              << " Station: " << trackerStation << " MotorAdj: " << state_->motorAdj << std::endl;

    

    if(trackerStation == 1)
    {
        {
            std::unique_lock lock(state_->controller_mtx);

            std::cout << "Found station" << std::endl;

            // AGV kører FREM
            if (!state_->returning) {
                state_->currentStationID++;
            } 
            // AGV kører tilbage
            else {
                state_->currentStationID--;
            }

            std::cout << "Station ID now: " << state_->currentStationID << std::endl;
            state_->station = true;
            state_->driving = false;
        }
    }

    if (tracker1 == 1 && tracker2 == 1 && tracker3 == 1)
    {
        state_->motorAdj = 1; // Perfect on line (111)
    }
    else if (tracker1 == 1 && tracker2 == 1 && tracker3 == 0)
    {
        std::cout << "softLeft" << std::endl;
        state_->motorAdj = 2; // Soft offline LEFT (110)
    }
    else if (tracker1 == 1 && tracker2 == 0 && tracker3 == 0)
    {
        std::cout << "shartleft" << std::endl;
        state_->motorAdj = 3; // Sharp offline LEFT (100)
    }
    else if (tracker1 == 0 && tracker2 == 1 && tracker3 == 1)
    {
        std::cout << "soft right" << std::endl;
        state_->motorAdj = 4; // Soft offline RIGHT (011)
    }
    else if (tracker1 == 0 && tracker2 == 0 && tracker3 == 1)
    {
        std::cout << "sharpright" << std::endl;
        state_->motorAdj = 5; // Sharp offline RIGHT (001)
    }
    else {
        std::cout << "Lost line" << std::endl;
        state_->motorAdj = 0; // Lost line
    }
}

void KY033::startPosition()
{
    bool startState = false;
    std::chrono::steady_clock::time_point duration;
    bool isTracking = false;
     while(!startState){
        readLineGPIO();
        readStationGPIO();
        if (station_data_in.values[0] == 1 && data_in.values[2] == 1 && data_in.values[1] == 1 && data_in.values[0] == 1)
        {
            if (!isTracking)
            {
                std::cout << "Found line" << std::endl;
                isTracking = true;
                duration = std::chrono::steady_clock::now();
            }
            else
            {
                if (std::chrono::steady_clock::now() - duration > std::chrono::seconds(1))
                {
                    std::unique_lock lock(state_->controller_mtx);
                    std::cout << "Klar til start" << std::endl;
                    state_->StartSensor_flag = true;
                    startState = true;
                    state_->cv_sensor.notify_one();

                }
            }
        }
        else
        {
            isTracking = false;
        }
    }
}

void KY033::readLineGPIO()
{
    int ret = ioctl(signal_req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data_in);

    if(ret < 0){
        perror("ioctl");
        close(signal_req.fd);
        close(chip_fd);
        return;
    }
}

void KY033::readStationGPIO()
{
    int ret = ioctl(station_signal_req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &station_data_in);

    if(ret < 0){
        perror("ioctl");
        close(station_signal_req.fd);
        close(chip_fd);
        return;
    }
}
