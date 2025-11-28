#pragma once
#include <linux/gpio.h>
#include "AGVstate.h"


class Motor
{
private:
    int chip_fd = -1;
    struct gpiohandle_request req_in;
    struct gpiohandle_data data;
    int speed_A = 0;
    int speed_B = 0;
    int PWM_channel_A; int PWM_channel_B;
    AGVstate *state_;
public:
    Motor(AGVstate *state);
    ~Motor();
    void init(int, int, int, int, int, int);
    void Drive(bool way, int speed);
    void Steering(int adjust, bool right); // true er højre og false er venstre.
    void stop();
    void RotateRight();
    void GoRight(int speed); // den ene motor er slukket
    void GoLeft(int speed); // den ene motor er slukket
    // Hjælpefunktioner
    void SpeedControl_A(int speed);
    void SpeedControl_B(int speed);
    int GetSpeedA();
    int GetSpeedB();
};


