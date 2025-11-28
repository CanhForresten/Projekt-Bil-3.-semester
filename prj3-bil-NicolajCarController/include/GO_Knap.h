#pragma once
#include <linux/gpio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define GPIO_CHIP_PATH "/dev/gpiochip0"

class GO_Knap
{
private:
    int chip_fd;
    gpiohandle_request req{};
    gpiohandle_data data{};
    int pin;

public:
    GO_Knap(int gpio_pin); //constructor
    ~GO_Knap(); //destructor
    void init(); //init
    bool isPressed(); //Bool som tjekker om knapper er trykket eller ej
};
