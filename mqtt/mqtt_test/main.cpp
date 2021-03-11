#include "mqtt_client.h"
#include<thread>
using namespace  micagent;
using std::thread;
int main()
{
    Imqtt_client client("test.mosquitto.org");
    mqtt_init_param param1(2000,2000,true);
    mqtt_connect_param param2;
    param2.client_id="mqtt_test_1111111111111111";
    param2.connect_flags=MQTT_CONNECT_CLEAN_SESSION;
    client.init(param1,param2);
    string topic="datetime1";
    thread t([&](){
        client.loop();
    });
    subscribe_param param3;
    auto func=[](const string &topic_name,shared_ptr<uint8_t>message,size_t message_size){
        printf("subscribe message#  %s  #for topic %s\r\n",string(reinterpret_cast<char *>(message.get()),message_size).c_str(),topic_name.c_str());
    };
    param3.topic_name=topic;
    param3.max_qos_level=2;
    param3.message_cb=func;
    client.subscribe(param3);
    param3.topic_name="datetime";
    client.subscribe(param3);
    thread t2([&](){
        std::this_thread::sleep_for(std::chrono::seconds(10));
        printf("disconnect \r\n");
        client.disconnect();
        std::this_thread::sleep_for(std::chrono::seconds(10));
        printf("reconnect \r\n");
        client.reconnect();
    });
    t2.detach();
    while(fgetc(stdin) == '\n') {
        /* get the current time */
        time_t timer;
        time(&timer);
        struct tm* tm_info = localtime(&timer);
        char timebuf[26];
        strftime(timebuf, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        /* print a message */
        char application_message[256];
        application_message[255]='\0';
        snprintf(application_message, sizeof(application_message), "The time is %s", timebuf);
        printf("published : \" %s \" \r\n",application_message);
        /* publish the time */
        client.unsubscribe(param3);
        auto error=client.publish( topic, application_message, strlen(application_message) + 1, MQTT_PUBLISH_QOS_0|MQTT_PUBLISH_RETAIN);

        /* check for errors */
        if (error != MQTT_OK) {
            fprintf(stderr, "error: %s\n", client.get_mqtt_error(error).c_str());
        }
    }
    client.exit();
    t.join();
    return 0;
}
