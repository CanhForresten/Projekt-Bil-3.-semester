#pragma once

#include <linux/gpio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include "AGVstate.h"

#define GPIO_CHIP_PATH "/dev/gpiochip0"

class Shutdown_Knap
{
private:
    int chip_fd;
    gpiohandle_request req{};
    gpiohandle_data data{};
    int pin;
    AGVstate* state_;

public:
    Shutdown_Knap(int gpio_pin, AGVstate* state);   // constructor
    ~Shutdown_Knap();              // destructor

    void init();                   // initialize GPIO
    bool isPressed();              // TRUE hvis knappen er trykket
    void checkForShutdown();  // THREAD 

};
