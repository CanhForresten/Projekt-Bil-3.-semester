#include <iostream>
#include <thread>
#include "CarController.h"
#include "lineSensor.h"
#include "AGVstate.h"
#include "DistanceSensor.h"
#include "mqtt_publisher.hpp"
#include "Shutdown_Knap.h"
#include "mqtt_subscriber.hpp"

int main() {
    std::cout << "Starting AGV system..." << std::endl;

    // --- Shared states ---
    AGVstate agvState;
    Ordrestate ordreState; // til MQTT ordre-kø

    // --- Hardware og controllers ---
    CarController controller(&agvState); // CarController får reference til AGVstate
    KY033 lineTracker(23, 5, 12, 16, &agvState); // GPIO pins til line tracker
    Shutdown_Knap shutdownKnap(17, &agvState);
    shutdownKnap.init(); // GPIO for shutdown knap

    // HCSR04 distance sensor (valgfri, pins fx 18 og 24)
    // HCSR04 distanceSensor(&agvState, 18, 24);

    // --- Threads ---

    // MQTT Publisher: sender AGV status til server/personale
    std::thread mqttPubThread(mqtt_publisher, &agvState);

    // MQTT Subscriber: modtager ordre fra server og fylder ordre-kø
    std::thread mqttSubThread(mqtt_subscriber, &ordreState);

    // Line sensor tråd
    std::thread lineSensorThread(&KY033::lineSensorThread, &lineTracker);

    // Distance sensor tråd (valgfri)
    // std::thread distanceSensorThread(&HCSR04::distanceSensorThread, &distanceSensor);

    // CarController hovedtråd
    std::thread carControllerThread(&CarController::carControllerThread, &controller);

    // shutdown controller
    std::thread shutdownThread(&Shutdown_Knap::checkForShutdown, &shutdownKnap);


    // --- Join threads ---
    lineSensorThread.join();
    shutdownThread.join();
    // distanceSensorThread.join();
    carControllerThread.join();
    mqttPubThread.join();
    mqttSubThread.join();

    return 0;
}

