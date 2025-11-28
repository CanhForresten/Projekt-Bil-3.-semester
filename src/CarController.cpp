#include "CarController.h"

CarController::CarController(AGVstate *state)
:   state_(state),
    motor(state_),
    bokse_led({5,6,13,19}),
    go_button(17),
    shutdown_button(17),
    status_led(22)
{
}

void CarController::init() {
    std::cout << "Initialising the carController" << std::endl;

    motor.init(4, 20, 25, 21, 2, 3);

    bokse_led.init();
    go_button.init();
    shutdown_button.init();
    status_led.init();
    status_led.turnOn();

    std::cout << "Sender status til server (power_on_NR)" << std::endl;
    state_->server_state_change = true;
    state_->current_state = AGVstate::AGV_states::power_on_NR;
    state_->cv_AGV.notify_one();
}

void CarController::start()
{
    std::unique_lock lock(state_->controller_mtx);
    state_->cv_sensor.wait(lock, [&]() { return state_->StartSensor_flag; });
    std::cout << "Sender status til server (power_on_R)" << std::endl;
    state_->server_state_change = true;
    state_->current_state = AGVstate::AGV_states::power_on_R;
    state_->cv_AGV.notify_one();
}


/* ---------------- PAK ---------------- */

void CarController::pak() {
    std::unique_lock lock(state_->mtx);

    while (true) {

        state_->cv_order.wait(lock, [&] {
            return state_->order_selected || state_->go_pushed;
        });

        if (state_->go_pushed) {
            bokse_led.Turn_Off_All_LED();
            return;
        }

        int station = state_->selected_station;
        int orderId = state_->selected_order_id;

        int boxIndex = -1;
        for (int i = 0; i < (int)state_->boxes.size(); i++) {
            if (!state_->boxes[i].packed) {
                boxIndex = i;
                break;
            }
        }

        if (boxIndex == -1) {
            std::cout << "FEJL: Ingen ledige bokse!" << std::endl;
            state_->order_selected = false;
            continue;
        }

        bokse_led.Turn_On_Box(boxIndex + 1);
        std::cout << "Boks " << boxIndex + 1 << " tændt, venter på bekræftelse." << std::endl;

        state_->cv_confirm.wait(lock, [&] {
            return state_->order_confirmed;
        });

        state_->boxes[boxIndex].packed = true;
        state_->boxes[boxIndex].station = station;
        state_->boxes[boxIndex].order_id = orderId;

        state_->order_confirmed = false;
        state_->order_selected = false;

        bokse_led.Turn_Off_All_LED();
        std::cout << "Boks " << boxIndex + 1 << "pakket og slukket." << std::endl;
    }
}


/* ---------------- DRIVE ---------------- */

void CarController::drive() {
    std::cout << "Drive funktion startet - venter på GO knap" << std::endl;

    {
        std::lock_guard<std::mutex> lock(state_->controller_mtx);
        state_->go_pushed = false;
        state_->driving = false;
    }

    // Vent på at sensor detekterer perfekt linje (simuleret GO knap)
    
    {
        std::unique_lock<std::mutex> lock(state_->controller_mtx);
        state_->cv_button.wait(lock, [&]() { return state_->go_pushed; });
    }
    
    std::cout << "GO knap trykket - starter kørsel!" << std::endl;

    {
        std::lock_guard<std::mutex> lock(state_->controller_mtx);
        state_->driving = true;
        state_->server_state_change = true;
        state_->current_state = AGVstate::AGV_states::power_on_D;
    }

    state_->cv_AGV.notify_one();
    state_->cv_sensor.notify_one();

    std::cout << "Status sendt til server (power_on_D)" << std::endl;

    while (true) {
        {
            std::unique_lock<std::mutex> lock(state_->controller_mtx);
            if (!state_->driving) {
                std::cout << "Driving stoppet - station nået sender til GUI" << std::endl;

                state_->server_state_change = true;

                switch (state_->station) {
                    case 1:
                        state_->current_state = AGVstate::AGV_states::AGV_arrived_1;
                        break;
                    case 2:
                        state_->current_state = AGVstate::AGV_states::AGV_arrived_2;
                        break;
                    case 3:
                        state_->current_state = AGVstate::AGV_states::AGV_arrived_3;
                        break;
                    case 4:
                        state_->current_state = AGVstate::AGV_states::AGV_arrived_4;
                        break;
                    default:
                        break;  
                    }
                state_->cv_AGV.notify_one();
                break;
            }
            adjust();
            Obstacle();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Drive funktion afsluttet" << std::endl;
}


/* ---------------- OBSTACLE ---------------- */

void CarController::Obstacle() {
    motor.stop();

    bool helpCalled = false;
    int wallSeconds = 0;
    int clearSeconds = 0;

    std::cout << "Checking for obstical" << std::endl;
    while(true) {
        if(state_->wall) {
            clearSeconds = 0;
            std::cout << "Der er stadig væg" << std::endl;
            wallSeconds++;
            if(!helpCalled && wallSeconds >= 10) {
                std::cout << "Help called" << std::endl;
                callHelp();
                helpCalled = true;
            }
        }
        else {
            std::cout << "Nu er der ikke væg" << std::endl;
            wallSeconds = 0;
            clearSeconds++;
            if(clearSeconds >= 5) {
                std::cout << "Vi kører igen" << std::endl;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


/* ---------------- ADJUST ---------------- */

void CarController::adjust() {
    switch (state_->motorAdj) {

        case 1:
            motor.Drive(true, 100);
            break;

        case 2:
            motor.Steering(20, false);
            break;

        case 3:
            motor.Steering(60, false);
            break;

        case 4:
            motor.Steering(20, true);
            break;

        case 5:
            motor.Steering(60, true);
            break;

        case 0:
            motor.stop();
            break;

        default:
            break;
    }
}


/* ---------------- MAIN THREAD ---------------- */

void CarController::carControllerThread() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    init();

    // start();

    std::cout << "Start position fundet" << std::endl;

    drive();

     while (state_->running) {

        if (shutdown_button.isPressed()) {

            std::cout << "Shutdown-knap trykket" << std::endl;

            {
                std::lock_guard<std::mutex> lock(state_->controller_mtx);
                state_->running = false;
                state_->server_state_change = true;
                state_->current_state = AGVstate::AGV_states::Power_off;
            }

            state_->cv_AGV.notify_one();

            motor.stop();
            status_led.turnOff();
            bokse_led.Turn_Off_All_LED();

            return; // afslutter alt helt
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void CarController::callHelp()
{
    state_->server_state_change = true;
    state_->current_state = AGVstate::AGV_states::AGV_blocked;
    state_->cv_AGV.notify_one();
}

/* ---------------- STATION ---------------- */

void CarController::station(Ordrestate& ordre) {
    
    int boxIndex = -1;
    for (int i = 0; i < (int)state_->boxes.size(); i++) { //finder boksen der tilhører order og station
        if (state_->boxes[i].station == ordre.station &&
            state_->boxes[i].order_id == ordre.id)
        {
            boxIndex = i;
            break;
        }
    }

    if (boxIndex == -1) {
        std::cout << "Ingen boks matcher ordreID " 
                  << ordre.id << " på station " << ordre.station << std::endl;
        return;
    }

   
    bokse_led.Turn_On_Box(boxIndex + 1); //tænder led på boks. og status led
    status_led.turnOn();

    std::cout << "Boks " << boxIndex + 1 << " tændt, vent på at personale trykker GO" << std::endl;

    {
        std::unique_lock<std::mutex> lock(state_->controller_mtx);
        state_->go_pushed = false; // reset når go er trykket
        state_->cv_button.wait(lock, [&]() { return state_->go_pushed; });
    }

    std::cout << "GO trykket, udlevering bekræftet." << std::endl;

    bokse_led.Turn_Off_All_LED();
    status_led.turnOff();

    state_->boxes[boxIndex].packed   = false; //nulstiller boks
    state_->boxes[boxIndex].order_id = -1;
    state_->boxes[boxIndex].station  = -1;

    std::cout << "Boks nulstillet efter udlevering." << std::endl;

    
    if (ordre.hasNextStation()) { // er der flere stationer?
        std::cout << "Flere stationer, fortsætter til næste station." << std::endl;

        {
            std::lock_guard<std::mutex> lock(state_->controller_mtx);
            state_->server_state_change = true;
            state_->current_state = AGVstate::AGV_states::power_on_D;
        }
        state_->cv_AGV.notify_one();
        return;
    }

    //kører tilbage til hjemmestation hvis ikke flere stationer.
    std::cout << "ikke flere stationer. Kører til hjemmestation." << std::endl;

    {   
        std::lock_guard<std::mutex> lock(state_->controller_mtx);
        state_->server_state_change = true;
        state_->current_state = AGVstate::AGV_states::power_on_D;
    }

    state_->cv_AGV.notify_one();
    returnToStart(0);

}
void CarController::returnToStart(int startStationID) {

    {
        std::lock_guard<std::mutex> lock(state_->controller_mtx);  //slukker sensor indtil kaldt
        state_->motorAdj = 0;          
        state_->driving = false;       
    }

    motor.RotateRight();                     
    std::this_thread::sleep_for(std::chrono::milliseconds(800)); //venter 800 ms, så sensor ikke registrer med det samme

    {
        std::lock_guard<std::mutex> lock(state_->controller_mtx); //kald på sensor
        state_->driving = true; 
    }
    state_->cv_sensor.notify_one();


    while (true) 
    {
        std::lock_guard<std::mutex> lock(state_->controller_mtx);
        if (state_->motorAdj == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    while (true) { 
        adjust();            
        Obstacle();
        {
            std::lock_guard<std::mutex> lock(state_->controller_mtx);

            if (state_->station) {              // Sensortråden sætter dette når en station detekteres
                state_->station = false;        // reset flag
                state_->currentStationID--;     // vi kører tilbage → stationnummer går ned
                std::cout << "station kørt forbi " << state_->currentStationID << std::endl;
            }

            // stopper når AGV er hjemme
            if (state_->currentStationID == startStationID) {
                motor.stop();
                break;
            }   

        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    {
        std::lock_guard<std::mutex> lock(state_->controller_mtx); //sensor slukkes igen
        state_->motorAdj = 0;
        state_->driving = false;
    }

    motor.RotateRight();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    {
        std::lock_guard<std::mutex> lock(state_->controller_mtx); //sensor tændes igen
        state_->driving = true;
    }
    state_->cv_sensor.notify_one();
    
    std::cout << "AGV tilbage i start " << startStationID << ", klar til næste tur" << std::endl;
}

