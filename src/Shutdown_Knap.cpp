#include "Shutdown_Knap.h"
#include <thread>
#include <chrono>

Shutdown_Knap::Shutdown_Knap(int gpio_pin, AGVstate* state)
    : pin(gpio_pin), state_(state)
{
}

Shutdown_Knap::~Shutdown_Knap()
{
    close(req.fd);
    close(chip_fd);
}

void Shutdown_Knap::init()
{
    chip_fd = open(GPIO_CHIP_PATH, O_RDWR);
    if (chip_fd < 0)
        throw std::runtime_error("Failed to open GPIO chip");

    req.lineoffsets[0] = pin;
    req.lines = 1;
    req.flags = GPIOHANDLE_REQUEST_INPUT;
    strcpy(req.consumer_label, "Shutdown_Button");

    if (ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0)
        throw std::runtime_error("Failed to request Shutdown_Button line");

    std::cout << "Shutdown button initialized on GPIO " << pin << std::endl;
}

bool Shutdown_Knap::isPressed()
{
    if (ioctl(req.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data) < 0)
        throw std::runtime_error("Failed to read Shutdown button");

    return (data.values[0] == 0); // aktiv-lav
} 

void Shutdown_Knap::checkForShutdown()
{
    std::cout << "Shutdown watchdog started..." << std::endl;

    while (true) {
        if (isPressed()) {
            std::cout << "SHUTDOWN BUTTON PRESSED" << std::endl;
            std::cout << "Shutting down system..." << std::endl;

            system("sudo shutdown now");
            return; 
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
