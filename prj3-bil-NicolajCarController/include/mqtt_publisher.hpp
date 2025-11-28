#include <mosquitto.h>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>
#include <string>
#include <cstring>
#include "AGVstate.h"


/////////////////////////////////Denne fil skal compileres sammen med carcontroller og ikke server/////////////

//Commandoer til as installere mosquitto broker på pi, hvilket er nødvendigt inden mosquitto.h kan bruges.

//Step 1. sudo apt update && sudo apt upgrade
//Step 2. sudo apt install -y mosquitto mosquitto-clients
//////// Commando til as installere lib til at bruge mosquitto.h: sudo apt install libmosquitto-dev

//Start, stop eller tjek status af mosquitto broker med sudo systemctl status mosquitto.service. Udskift status med enten (stop, start, restart)
 // Sørg for at mosquitto lib er inkluderet i cmakelist filen med target_link_libraries(${PROJECT_NAME} mosquitto)


//Disse skal nok rykkes ind i AGVstate


//callback funktion
void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    AGVstate *state = static_cast<AGVstate*>(obj);

    if (rc == 0) {
        std::cout << "Connected successfully!" << std::endl;
        {
            std::lock_guard<std::mutex> lock(state->connection_mtx);
            state->connected = true;
        }
        state->connection_cv.notify_all();
    }
    else {
        std::cout << "Connect failed: " << mosquitto_strerror(rc) << std::endl;
    }
}




void publish_NR (struct mosquitto *mosq) { // Publish "tændt/ikke" klar NR =not ready
    
    std::cout << "SENDER STATUS: tændt" << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("tændt"), "tændt", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }

}

void publish_off (struct mosquitto *mosq) { // Publish "slukket"

    std::cout << "SENDER STATUS: slukket" << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("slukket"), "slukket", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }

}

void publish_R (struct mosquitto *mosq) { // Publish "tændt/klar"

    std::cout << "SENDER STATUS: tændt/klar" << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("tændt/klar"), "tændt/klar", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }
}

void publish_D (struct mosquitto *mosq) { // Publish "tændt/klar"

    std::cout << "SENDER STATUS: tændt/kører" << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("tændt/kører"), "tændt/kører", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }
}


void publish_station_1 (struct mosquitto *mosq) {

     std::cout << "SENDER STATUS: Levering:1" << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("Levering:1"), "Levering:2", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }

}

void publish_station_2 (struct mosquitto *mosq) {

     std::cout << "SENDER STATUS: Levering:2 " << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("Levering:2"), "Levering:2", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }

}

void publish_station_3 (struct mosquitto *mosq) {

     std::cout << "SENDER STATUS: Levering:3 " << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("Levering:3"), "Levering:3", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }

}

void publish_station_4 (struct mosquitto *mosq) {

     std::cout << "SENDER STATUS: Levering:4 " << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("Levering:4"), "Levering:4", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }

}

void publish_blocked (struct mosquitto *mosq) {

     std::cout << "SENDER STATUS: Blokeret" << std::endl;
    int rc = mosquitto_publish(mosq, nullptr, "/personale/AGV_status", strlen("blokeret"), "blokeret", 0 , false );

     if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto publish failed: " << mosquitto_strerror(rc) << std::endl;
    }

}




auto mqtt_publisher(AGVstate *state) {


    int rc = mosquitto_lib_init();

    if (rc != MOSQ_ERR_SUCCESS) {

        std::cerr << "Mosquitto init failed: " << mosquitto_strerror(rc) << std::endl;
    }

    mosquitto *mosq = mosquitto_new("carcontroller_publisher", true, state); // Laver ny mosquitto client instans med navn, true for clean session i.e fjerner alle subcribtions og msg efter disconnect

    mosquitto_connect_callback_set(mosq,on_connect); // Dette sætter hvilken funktion der skal kaldes ved fremtidig connection (næste linje)

    rc = mosquitto_connect(mosq, "10.192.89.32", 1883, 60); // Tjek pi's IP adresse med hostname -I, og indsæt denne.
    

    mosquitto_loop_start(mosq);


    //Her forhindres det, at clienten fortsætter inden der er oprettet forbindelse. herved burde man ikke risikere at der kaldes publish inden connect er færdig
    {
        std::unique_lock<std::mutex> lock2(state->connection_mtx);
        state->connection_cv.wait(lock2, [&] { return state->connected; });
    }


    std::unique_lock<std::mutex> lock (state->publisher_mtx);
    
    while (true)
    {

        state->cv_AGV.wait(lock, [&] { return state->server_state_change == true; }); // Vent på at fælles bool ændres af carcontroller

        state->server_state_change = false; // reset flaget


        AGVstate::AGV_states new_state = state->current_state; // lav ny state med state fra fælles nyligt opdateret current_state

        lock.unlock(); // der låses op, da man ikke ønsker at skabe risiko for deadlocks ved at låse mens der foretages I/O. som der sker ved publish kaldene

        switch (new_state)
        {
        case AGVstate::AGV_states::Power_off:

            publish_off(mosq);

            break;

        case AGVstate::AGV_states::power_on_NR:

            publish_NR(mosq);

            break;

        case AGVstate::AGV_states::power_on_R:
        
            publish_R(mosq);

            break;
        
        case AGVstate::AGV_states::power_on_D:

            publish_D(mosq);

            break;

        case AGVstate::AGV_states::AGV_arrived_1:

            publish_station_1(mosq);

            break;

        case AGVstate::AGV_states::AGV_arrived_2:

            publish_station_2(mosq);

            break;

        case AGVstate::AGV_states::AGV_arrived_3:

            publish_station_3(mosq);

            break;

        case AGVstate::AGV_states::AGV_arrived_4:

            publish_station_4(mosq);

            break;

        case AGVstate::AGV_states::AGV_blocked:

            publish_blocked(mosq);

            break;

        default:
            break;
        }

        lock.lock(); // Lås igen, da wait kaldet i starten af loopet har brug at låsen er låst igen
    }
    
};