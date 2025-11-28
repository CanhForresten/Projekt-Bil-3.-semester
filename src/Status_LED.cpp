#include "Status_LED.h"

Status_LED::Status_LED(int gpio_pin) : pin(gpio_pin)
{
    chip_fd = open(GPIO_CHIP_PATH, O_RDWR);
    if (chip_fd < 0)
        throw std::runtime_error("Failed to open GPIO chip");

    req.lineoffsets[0] = pin;
    req.lines = 1;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req.default_values[0] = 0;
    strcpy(req.consumer_label, "Status_LED");

    if (ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0)
        throw std::runtime_error("Failed to request Status_LED line");
}

Status_LED::~Status_LED()
{
    close(req.fd);
    close(chip_fd);
}

void Status_LED::init()
{
    std::cout << "Status LED initialized on GPIO " << pin << std::endl;
}

void Status_LED::turnOn()
{
    gpiohandle_data data{};
    data.values[0] = 1;
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    std::cout << "Status LED ON" << std::endl;
}

void Status_LED::turnOff()
{
    gpiohandle_data data{};
    data.values[0] = 0;
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    std::cout << "Status LED OFF" << std::endl;
}
