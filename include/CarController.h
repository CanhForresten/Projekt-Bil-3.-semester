#pragma once
#include "AGVstate.h"
#include "Boks_LED.h"
#include "GO_Knap.h"
#include "Shutdown_Knap.h"
#include "Status_LED.h"
#include "lineSensor.h"
#include "motor.h"

#include <iostream>
#include <queue>
#include <string>
#include <vector>


struct CompareStationStrAsNumber {
    bool operator()(Ordrestate& a, Ordrestate& b) const {
        return std::stoi(a.station) > std::stoi(b.station);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Ordrestate& o) {
    os << " ID: " << o.id
       << " |Station: " << o.station;
    return os;
}

struct Ordrestate {
    std::string id;
    std::string station;

    std::condition_variable cv_order; // cv at notify når msg's bliver pushed til queue fra mqtt callback functionen
    std::mutex q_order_mut; // mutex til at beskytte fælles queue som tilgås af både beskedprocesserings-tråden og mqtt subscriberen
    std::queue<int> stations_q;
};


class CarController {
private:
    AGVstate* state_;
    Motor motor;
    LED bokse_led;       
    GO_Knap go_button;        
    Shutdown_Knap shutdown_button;
    Status_LED status_led; 
    std::priority_queue<Ordrestate, std::vector<Ordrestate>, CompareStationStrAsNumber> OrdrestateQueue;

public:
     CarController(AGVstate* state);
    void init();
    void start();
    void pak();
    void drive();
    void station(Ordrestate& ordre);
    void Obstacle();
    void carControllerThread();
    void returnToStart(int startStationID);
    void callHelp();
    void adjust();

};
