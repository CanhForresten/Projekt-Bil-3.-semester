#include <mosquitto.h>
#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include "CarController.h"

void on_mqtt_message(struct mosquitto *mosq, void *userdata,
                     const struct mosquitto_message *msg)
{
    Ordrestate *ordre = static_cast<Ordrestate*>(userdata);

    std::string status_payload((char*)msg->payload,msg->payloadlen); // Laver string med samme størrelse som msg payload, og converter msg payload til string

    std::unique_lock<std::mutex> lock(ordre->q_order_mut);


   for (char c : status_payload)
    {
        if (c == '1' || c == '2' || c == '3' || c == '4') {
            ordre->stations_q.push(c - '0');  // converter fra ASCII til int. fx er '0' = 48 numerisk. og '3' = 51. dvs 51-48=3. Herved fås numerisk værdi af char-tallet
        }
    }
    
    ordre->cv_order.notify_one(); //notifier carcontroller der skal poppe beskeden fra køen
}




auto mqtt_subscriber_carcontroller(Ordrestate *ordre) {

    mosquitto_lib_init();
    mosquitto *mosq = mosquitto_new("carcontroller_subscriber", true, &ordre); // Laver ny mosquitto client instans med navn, true for clean session i.e fjerner alle subcribtions og msg efter disconnect
    mosquitto_connect(mosq, "localhost", 1883, 60); // Indsæt server pi adresse
    mosquitto_subscribe(mosq, nullptr, "/service/order", 0);
    mosquitto_message_callback_set(mosq, on_mqtt_message); // Hvis broker modtager en besked kaldes on_message
    mosquitto_loop_forever(mosq, -1, 1);
};
