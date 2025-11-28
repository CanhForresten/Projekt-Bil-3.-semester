#include "Boks_LED.h"

LED::LED(const std::vector<int>& gpio_pins) : pins(gpio_pins) //constructor
{
    chip_fd = open(GPIO_CHIP_PATH, O_RDWR);
    if (chip_fd < 0)
        throw std::runtime_error("Failed to open GPIO chip");

    for (int pin : pins)
    {
        gpiohandle_request req{};
        req.lineoffsets[0] = pin;
        req.lines = 1;
        req.flags = GPIOHANDLE_REQUEST_OUTPUT;
        req.default_values[0] = 0;
        strcpy(req.consumer_label, "Box_LED");

        if (ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0)
            throw std::runtime_error("Failed to request LED line");

        led_reqs.push_back(req);
    }
}

LED::~LED() //destructor
{
    for (auto& req : led_reqs)
        close(req.fd);
    close(chip_fd);
}

void LED::init() //init
{
    std::cout << "[INIT] " << pins.size() << " box LEDs initialized." << std::endl;
}

void LED::Turn_Off_All_LED() //Funktion der slukker alle LED-lys
{
    gpiohandle_data data{};
    data.values[0] = 0;

    for (auto& req : led_reqs)
        ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);

    std::cout << "All box LEDs OFF" << std::endl;
}

void LED::Turn_On_Box(int boxNumber) //Funktion der tænder LED-lys til en given boks
{
    Turn_Off_All_LED();
    if (boxNumber < 1 || boxNumber > (int)pins.size())
        return;

    gpiohandle_data data{};
    data.values[0] = 1;
    ioctl(led_reqs[boxNumber - 1].fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);

    std::cout << "Box LED " << boxNumber << " ON" << std::endl;
}
