
#include <mosquitto.h>
#include <stdio.h>
#include <string>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <iostream>

struct AGV_status_info_t {

    AGV_status_info_t(std::string status, std::string topic) : m_status(status), m_topic(topic) {}
    std::string m_status;
    std::string m_topic;
};


std::condition_variable cv; // cv to notify when msg's are pushed to the queue from the mqtt callback function
std::mutex q_mut; // mutex to protect queue which will be accessed both by restinio and mqtt subscriber
std::mutex status_string_mut; //mutex til at beskytte fælles string som tilgås af både restinio og mqtt subscriberen

std::queue<AGV_status_info_t> status_q;
std::string AGV_status; // fælles string der også tilgås af rest server






void on_mqtt_message(struct mosquitto *mosq, void *userdata,
                     const struct mosquitto_message *msg)
{
    std::string topic = msg->topic;
    std::string status_payload((char*)msg->payload);

    std::unique_lock<std::mutex> lock(q_mut);

    AGV_status_info_t temp_store_status(status_payload,topic);
    status_q.push(temp_store_status);
    cv.notify_one();
}

void process_status_messages() // Denne funktion vil køre  i sin egen tråd
{
    while (true)
    {   
        {
            std::unique_lock<std::mutex> lock(q_mut);
            cv.wait(lock, [] { return !status_q.empty(); }); // venter på notificering af on_mqtt_message

            auto msg = status_q.front();
            status_q.pop();
        
        
            std::unique_lock<std::mutex> lock2(status_string_mut);
            AGV_status = msg.m_status; // Sæt den fælles string til den nyligt poppede status værdi poppet fra køen
            std::cout << "[STATUS] Updated AGV_status to: " << AGV_status << std::endl;


        }

    }
}

auto mqtt_subscriber() {

    mosquitto_lib_init();
    mosquitto *mosq = mosquitto_new("rest_server_subscriber", true, NULL); // Laver ny mosquitto client instans med navn, true for clean session i.e fjerner alle subcribtions og msg efter disconnect
    mosquitto_threaded_set(mosq,true);
    mosquitto_connect(mosq, "localhost", 1883, 60);
    mosquitto_subscribe(mosq, nullptr, "/personale/AGV_status", 0);
    mosquitto_message_callback_set(mosq, on_mqtt_message); // Hvis broker modtager en kaldes on_message
    mosquitto_loop_forever(mosq, -1, 1);
};
