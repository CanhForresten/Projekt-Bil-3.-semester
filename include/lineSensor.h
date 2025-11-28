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
#include <thread>
#include "AGVstate.h"


class KY033 {
    private:
        void readLineGPIO();
        void readStationGPIO();
        struct gpiohandle_data data_in;
        struct gpiohandle_request signal_req;

        struct gpiohandle_data station_data_in;
        struct gpiohandle_request station_signal_req;

        int chip_fd = -1;
        int GPIO_PIN;
        AGVstate *state_;
    public:
        KY033(int, int, int, int, AGVstate *state);
        ~KY033();
        void lineSensorThread();
        void onLine();
        void startPosition();
    
};