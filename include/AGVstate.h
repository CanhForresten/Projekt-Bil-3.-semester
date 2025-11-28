#pragma once
#include <condition_variable>
#include <mutex>
#include <array>


struct Boks {
    bool packed = false;       
    int station = -1;          
    int order_id = -1;      
};

struct AGVstate
{
    std::array<Boks, 4> boxes = {{
        Boks{}, Boks{}, Boks{}, Boks{}
    }};


    int selected_station = -1; 
    int selected_order_id = -1; 
    bool order_selected = false;
    bool order_confirmed = false;
    int currentStationID = 0;     
    bool returning = false;       // false = frem, true = retur, til at tjekke stationummer når den levere men også når den kører tilbage til start


    std::condition_variable cv_sensor;
    std::condition_variable cv_button;
    std::condition_variable cv_order;  
    std::condition_variable cv_confirm;
    std::condition_variable cv_AGV;
    std::condition_variable connection_cv;
   
    std::mutex mtx;
    std::mutex publisher_mtx;
    std::mutex connection_mtx; // Mutex der låser connected flag
    std::mutex sensor_mtx;
    std::mutex controller_mtx;
    
    //flags to controll packing
    bool packingSelected = false;
    bool packingcanncled = false;
    bool packidone;

    // Flags to control the states
    bool StartSensor_flag = false;
    bool connected = false;
    bool go_pushed = false;
    bool running = true;
    bool wall = false;
    bool server_state_change = false;
    bool driving = false;
    int station = false;
    bool turnOff_pushed = false;


    // Variables to control the motor adjust
    int motorAdj = 1;

    int AGV_arrived = -1;
    
    // Enum class til at indeholde forskellige states for AVG. Dette gør, at der ikke skal jungleres mellem flere bools
    enum class AGV_states {
        Power_off,
        power_on_NR,
        power_on_R,
        power_on_D,
        AGV_arrived_1,
        AGV_arrived_2,
        AGV_arrived_3,
        AGV_arrived_4,
        AGV_blocked
    };


    AGV_states current_state = AGV_states::Power_off; // sæt til standard state ved opstart. Dette flag skal opdateres af carcontroller

};
