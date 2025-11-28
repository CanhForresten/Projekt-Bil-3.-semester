#include "DistanceSensor.h"
// Define path to GPIO chip device
#define GPIO_CHIP_PATH "/dev/gpiochip0"

HCSR04::HCSR04(int GPIO_TRIG, int GPIO_ECHO, AGVstate *state) : state_(state)
{
    std::lock_guard lock(state_->controller_mtx);
    // Open gpiochip0 as read-only device
    chip_fd = open(GPIO_CHIP_PATH, O_RDWR);

    if (chip_fd < 0)
    {
        perror("open");
        return;
    }
    // ---- TRIG as output ----

    // Populate structure to be safe
    memset(&trig_req, 0, sizeof(trig_req));

    // Specify GPIO lines we want to access
    trig_req.lineoffsets[0] = GPIO_TRIG;

    // Access all lines as output
    trig_req.flags = GPIOHANDLE_REQUEST_OUTPUT;

    // Number of lines we want to access
    trig_req.lines = 1;

    // Set a name to be shown when gpioinfo is called
    strcpy(trig_req.consumer_label, "HCSR04 TRIG");

    // Use ioctl to get GPIO line handle from kernel
    int ret_trig = ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &trig_req);

    if (ret_trig < 0)
    {
        perror("ioctl");
        close(chip_fd);
        return;
    }

    // ---- ECHO as input ----

    // Populate structure to be safe
    memset(&echo_req, 0, sizeof(echo_req));

    // Specify GPIO lines we want to access
    echo_req.lineoffsets[0] = GPIO_ECHO;

    // Access all lines as inputs
    echo_req.flags = GPIOHANDLE_REQUEST_INPUT;

    // Number of lines we want to access
    echo_req.lines = 1;

    // Set a name to be shown when gpioinfo is called
    strcpy(echo_req.consumer_label, "HCSR04 ECHO");

    // Use ioctl to get GPIO line handle from kernel
    int ret_echo = ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &echo_req);

    if (ret_echo < 0)
    {
        perror("ioctl");
        close(chip_fd);
        return;
    }
}

HCSR04::~HCSR04() {
    close(chip_fd);
    close(trig_req.fd);
    close(echo_req.fd);
}

void HCSR04::trigHigh() {
    data_out.values[0] = 1;   // sæt trig high
    ioctl(trig_req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data_out);
}

void HCSR04::trigLow() {
    data_out.values[0] = 0;
    ioctl(trig_req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data_out);
}

void HCSR04::readGPIO() {
    int ret = ioctl(echo_req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data_in);

    if(ret < 0) {
        perror("ioctl");
        close(echo_req.fd);
        close(trig_req.fd);
        close(chip_fd); 
        return;
    }
}



int HCSR04::pulse() {
    trigHigh();
    std::this_thread::sleep_for(std::chrono::microseconds(5)); // måske for lavt
    trigLow();

    readGPIO();

    while(data_in.values[0] == 0) {
        readGPIO();
    }

    int count;
    while(data_in.values[0] == 1) {
        readGPIO();
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        count++;

        // Stop if echo is too long (e.g., over 38ms)
        if(count >= 38000) {
            //pulse lost
            return count;
        }
    }
    return count;

    // if(count <= 81) {
    //     std::cout << "wall" << std::endl;
    //     return co8ht;
    // }
    // else {
    //     std::cout << "Not wall" << std::endl;
    //     return count;
    // }

    // double distance = ((count * 0.5) / 58) * 100;

    // if(distance < 500){
    //     std::cout << distance << "cm" << std::endl;
    // }
}

void HCSR04::isWall() {
    {
        std::unique_lock lock(state_->controller_mtx);
        state_->cv_sensor.wait(lock, [&]() { return state_->StartSensor_flag; });
    }
    scan_1 = pulse();
    std::cout << "first scan" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scan_2 = pulse();
    std::cout << "second scan done" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    scan_3 = pulse();
    std::cout << "Third scan done" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    if(scan_1 <= 81 && scan_2 <= 81 && scan_3 <= 81 ) {
        std::unique_lock lock(state_->controller_mtx);
        state_->wall = true;
        state_->cv_sensor.notify_one();
    }
    else{
        std::unique_lock lock(state_->controller_mtx);
        state_->wall = false;
        state_->cv_sensor.notify_one();
    }
}

void HCSR04::distanceSensorThread() {
    while(state_->running) {
        isWall();
    }
}



