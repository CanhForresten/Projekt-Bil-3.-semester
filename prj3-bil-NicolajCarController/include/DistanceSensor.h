#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <thread>
#include <iostream>
#include <linux/gpio.h>
#include <poll.h>
#include <chrono>
#include "AGVstate.h"


class HCSR04 {
    public:
        HCSR04(int, int, AGVstate *state);
        ~HCSR04();
        void distanceSensorThread();
        void isWall();
        
    private:
        int scan_1;
        int scan_2;
        int scan_3;
        int pulse();
        void trigHigh();
        void trigLow();
        void readGPIO();
        struct gpiohandle_request trig_req;
        struct gpiohandle_request echo_req;
        struct gpiohandle_data data_out;
        struct gpiohandle_data data_in;
        int chip_fd = -1;
        AGVstate *state_;
};