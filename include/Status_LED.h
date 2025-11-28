#pragma once
#include <linux/gpio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define GPIO_CHIP_PATH "/dev/gpiochip0"

class Status_LED
{
private:
    int chip_fd;
    gpiohandle_request req{};
    int pin;

public:
    Status_LED(int gpio_pin); //constructor
    ~Status_LED(); //destructor
    void init(); //init
    void turnOn(); //Funktion der tænder status LED
    void turnOff(); //Funktion der slukker status LED
};

