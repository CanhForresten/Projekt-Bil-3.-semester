#pragma once
#include <linux/gpio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <iostream>

#define GPIO_CHIP_PATH "/dev/gpiochip0"

class LED
{
private:
    int chip_fd;
    std::vector<gpiohandle_request> led_reqs;
    std::vector<int> pins;

public:
    LED(const std::vector<int>& gpio_pins); //constructor
    ~LED(); //destructor
    void init(); //init
    void Turn_Off_All_LED(); //Funktion der slukker alle LED-lys
    void Turn_On_Box(int boxNumber); //Funktion der tænder LED-lys til en given boks
};
