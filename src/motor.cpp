#include "motor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <linux/gpio.h>
#include <poll.h>
#include <chrono>

// Define path to GPIO chip device
#define GPIO_CHIP_PATH "/dev/gpiochip0"

// Define pwm path to GPIO device
#define GPIO_PWM_CHIP_PATH "/sys/class/pwm/pwmchip0"


Motor::Motor(AGVstate *state)
{
    state_ = state;
}

Motor::~Motor()
{
    if (req_in.fd > 0)
    {
        close(req_in.fd);
    }
    // Close GPIO chip device
    if (chip_fd > 0)
    {
        close(chip_fd);
        return;
    }
}

void Motor::init(int GPIO_IN1, int GPIO_IN2, int GPIO_IN3, int GPIO_IN4, int PWM_channel_A_, int PWM_channel_B_){
    std::lock_guard(state_->controller_mtx);
    PWM_channel_A = PWM_channel_A_;
    PWM_channel_B = PWM_channel_B_;

        // Open gpiochip0 as read-only device
    chip_fd = open(GPIO_CHIP_PATH, O_RDWR);

    if (chip_fd < 0)
    {
        perror("open");
        return;
    }
    // Populate structure to be safe
    memset(&req_in, 0, sizeof(req_in));

    // Specify GPIO lines we want to access
    req_in.lineoffsets[0] = GPIO_IN1;
    req_in.lineoffsets[1] = GPIO_IN2;
    req_in.lineoffsets[2] = GPIO_IN3;
    req_in.lineoffsets[3] = GPIO_IN4;

    // sætter alle GPIO lines som output
    req_in.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req_in.lines = 4; // Number of lines to request

    // Set a name to be shown when gpioinfo is called
    strcpy(req_in.consumer_label, "Motor-Driver");

    int ret_out = ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, &req_in);

    if (ret_out < 0)
    {
        perror("ioctl");
        close(chip_fd);
        return;
    }
    // --------------PWM configuration for Channel A------------------
    const std::string pwm_path_a = std::string(GPIO_PWM_CHIP_PATH) + "/pwm" + std::to_string(PWM_channel_A);
    std::ifstream check_file_a(pwm_path_a + "/period");
    if (!check_file_a.good())
    {
        std::ofstream export_file_a(std::string(GPIO_PWM_CHIP_PATH) + "/export"); // Export the PWM channel
        if (!export_file_a)
        {
            std::cout << "Failed to export PWM" << std::endl;
            return;
        }
        export_file_a << PWM_channel_A;
        export_file_a.close();
        // Wait for initialization
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // Set period for PWM channel A to 50 hz = 20 ms
    std::ofstream period_file_a(pwm_path_a + "/period");
    if (period_file_a)
    {
        period_file_a << 20000000; // 20 ms
        period_file_a.close();
    }
    // Enable PWM channel A
    std::ofstream enable_file_a(pwm_path_a + "/enable");
    if (enable_file_a)
    {
        enable_file_a << 1;
        enable_file_a.close();
    }
    // --------------PWM configuration for Channel B------------------
    const std::string pwm_path_b = std::string(GPIO_PWM_CHIP_PATH) + "/pwm" + std::to_string(PWM_channel_B);
    std::ifstream check_file_b(pwm_path_b + "/period");
    if (!check_file_b.good())
    {
        std::ofstream export_file_b(std::string(GPIO_PWM_CHIP_PATH) + "/export"); // Export the PWM channel
        if (!export_file_b)
        {
            std::cout << "Failed to export PWM" << std::endl;
            return;
        }
        export_file_b << PWM_channel_B;
        export_file_b.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Set period for PWM channel B to 50 hz = 20 ms
    std::ofstream period_file_b(pwm_path_b + "/period");
    if (period_file_b)
    {
        period_file_b << 20000000; // 20 ms
        period_file_b.close();
    }
    // Enable PWM channel B
    std::ofstream enable_file_b(pwm_path_b + "/enable");
    if (enable_file_b)
    {
        enable_file_b << 1;
        enable_file_b.close();
    }
}

/*
-----------Huske regel!----------
| Input1 (Logisk) | Input2 (Logisk) | Input3 (Logisk)  | Input4 (Logisk) | Spinning Direction  |
|-----------------|-----------------|------------------|-----------------|---------------------|
| Low (0)         | Low (0)         | Low (0)          | Low (0)         | Motor off
| High (1)        | Low (0)         | High (1)         | Low (0)         | Forward
| Low (0)         | High (1)        | Low (0)          | High (1)        | Backward
| High (1)        | High (1)        | High (1)         | High (1)        | Motor off
*/
void Motor::Drive(bool way, int speed)
{
    // Retning
    if (way == true) // Forward
    {
        data.values[0] = 1; // sætter IN1 til HIGH
        data.values[1] = 0; // Sætter IN2 til LOW
        data.values[2] = 1; // Sætter IN3 til High
        data.values[3] = 0; // Sætter IN4 til Low
    }
    else // false = Backward
    {
        data.values[0] = 0; // Sætter IN1 til Low
        data.values[1] = 1; // Sætter IN2 til High
        data.values[2] = 0; // Sætter IN3 til Low
        data.values[3] = 1; // Sætter IN4 til High
    }

    SpeedControl_A(speed); // Ændre hastigheden motor A
    SpeedControl_B(speed); // Ændre hastigheden motor B

    if (ioctl(req_in.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data) < 0)
    {
        perror("ioctl");
    }
}

void Motor::SpeedControl_A(int speed)
{
    if (speed < 0)
    {
        speed_A = 0;
    }
    if (speed > 100)
    {
        speed_A = 100;
    }
    speed_A = speed;
    // Period er 20 ms = 20000000 ns
    long duty_cycle_ns = (20000000 * speed_A) / 100;
    
    const std::string pwm_path_a = std::string(GPIO_PWM_CHIP_PATH) + "/pwm" + std::to_string(PWM_channel_A);
    std::ofstream duty_cycle_file_a(pwm_path_a + "/duty_cycle");
    if (duty_cycle_file_a) {
        duty_cycle_file_a << duty_cycle_ns;
        duty_cycle_file_a.close();
    }   
}

void Motor::SpeedControl_B(int speed)
{
    if (speed < 0)
    {
        speed_B = 0;
    }
    if (speed > 100)
    {
        speed_B = 100;
    }
    speed_B = speed;

    // Period er 20 ms = 20000000 ns
    long duty_cycle_ns = (20000000 * speed_B) / 100;
    
    const std::string pwm_path_b = std::string(GPIO_PWM_CHIP_PATH) + "/pwm" + std::to_string(PWM_channel_B);
    std::ofstream duty_cycle_file_b(pwm_path_b + "/duty_cycle");
    if (duty_cycle_file_b) {
        duty_cycle_file_b << duty_cycle_ns;
        duty_cycle_file_b.close();
    }
}
 
void Motor::Steering(int adjust, bool right)
{
    std::string direction = (right == true) ? "Right" : "Left";

    std::cout << "Adjusting with number: " << adjust << " and " <<  direction << std::endl;
    if (right) {
        int base = GetSpeedA();
        int newA = base - adjust;
        if (newA < 0) newA = 0;
        SpeedControl_A(newA);
    } else {
        int base = GetSpeedB();
        int newB = base - adjust;
        if (newB < 0) newB = 0;
        SpeedControl_B(newB);
    }
}

void Motor::RotateRight()
{
    data.values[0] = 1; // sætter IN1 til HIGH
    data.values[1] = 0; // Sætter IN2 til LOW
    data.values[2] = 0; // Sætter IN3 til Low
    data.values[3] = 1; // Sætter IN4 til High
    SpeedControl_A(100);
    SpeedControl_B(100);
    if (ioctl(req_in.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data) < 0)
    {
        perror("ioctl");
    }
}

void Motor::GoRight(int speed)
{
    SpeedControl_A(0);
    SpeedControl_B(speed);
}

void Motor::GoLeft(int speed)
{
    SpeedControl_B(0);
    SpeedControl_A(speed);
}

int Motor::GetSpeedA()
{
    return this->speed_A;
}

int Motor::GetSpeedB()
{
    return this->speed_B;
}

void Motor::stop()
{
    SpeedControl_A(0);
    SpeedControl_B(0);
}
