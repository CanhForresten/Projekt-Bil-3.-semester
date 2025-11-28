#include "GO_Knap.h"

GO_Knap::GO_Knap(int gpio_pin) : pin(gpio_pin)
{
    chip_fd = open(GPIO_CHIP_PATH, O_RDWR);
    if (chip_fd < 0)
        throw std::runtime_error("Failed to open GPIO chip");

    req.lineoffsets[0] = pin;
    req.lines = 1;
    req.flags = GPIOHANDLE_REQUEST_INPUT;
    strcpy(req.consumer_label, "GO_Button");

    if (ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0)
        throw std::runtime_error("Failed to request GO_Button line");
}

GO_Knap::~GO_Knap()
{
    close(req.fd);
    close(chip_fd);
}

void GO_Knap::init()
{
    std::cout << "GO button initialized on GPIO " << pin << std::endl;
}

bool GO_Knap::isPressed()
{
    if (ioctl(req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data) < 0)
        throw std::runtime_error("Failed to read GO button");

    return (data.values[0] == 0); // aktiv lav
}
