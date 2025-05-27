#ifndef IRRIGATION_SYSTEM_WATCHER_H_
#define IRRIGATION_SYSTEM_WATCHER_H_
// ESP32 InformationPanel
// (C)2025 bekki.jp

// Include ----------------------
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <mqtt_client.h>

#include <functional>
#include <memory>
#include <unordered_map>

#include "logger.h"

namespace IrrigationSystem {

/// MQTTClient
class MQTTClient final {
 public:
  using ReceiveTopicFunction = std::function<void(const std::string& data)>;

  class SubscribeTopic {
   public:
    SubscribeTopic(const uint16_t subscribe_id, const std::string& topic,
                   MQTTClient::ReceiveTopicFunction function)
        : subscribe_id_(subscribe_id),
          topic_(topic),
          function_(function),
          property_() {
      property_.subscribe_id = subscribe_id;
    }

    uint16_t subscribe_id_;
    std::string topic_;
    MQTTClient::ReceiveTopicFunction function_;
    esp_mqtt5_subscribe_property_config_t property_;
  };

 public:
  MQTTClient();
  ~MQTTClient();

  void SetConnectEvent(std::function<void()>);
  void SetDisconnectEvent(std::function<void()>);
  void SetBrokerHost(const std::string& host);
  void AddSubscribeTopic(SubscribeTopic topic);
  void SetWillMessage(const std::string& topic, const std::string& message);
  void Start();

  bool IsConnected() const;

  void Publish(const std::string& topic, const std::string& data);

 private:
  void EventError(esp_mqtt_client_handle_t client,
                  const esp_mqtt_event_handle_t event);
  void EventConnected(esp_mqtt_client_handle_t client,
                      const esp_mqtt_event_handle_t event);
  void EventDisconnected(esp_mqtt_client_handle_t client,
                         const esp_mqtt_event_handle_t event);
  void EventSubscribed(esp_mqtt_client_handle_t client,
                       const esp_mqtt_event_handle_t event);
  void EventUnsubscribed(esp_mqtt_client_handle_t client,
                         const esp_mqtt_event_handle_t event);
  void EventPublished(esp_mqtt_client_handle_t client,
                      const esp_mqtt_event_handle_t event);
  void EventData(esp_mqtt_client_handle_t client,
                 const esp_mqtt_event_handle_t event);
  void EventBeforeConnect(esp_mqtt_client_handle_t client,
                          const esp_mqtt_event_handle_t event);
  void EventDeleted(esp_mqtt_client_handle_t client,
                    const esp_mqtt_event_handle_t event);
  void EventUser(esp_mqtt_client_handle_t client,
                 const esp_mqtt_event_handle_t event);

 private:
  static void Mqtt5EventHandler(void* handler_args, esp_event_base_t base,
                                int32_t event_id, void* event_data);

 private:
  static const std::function<void(MQTTClient&, esp_mqtt_client_handle_t client,
                                  const esp_mqtt_event_handle_t event)>
      EVENT_FUNCTIONS[MQTT_USER_EVENT + 1];

 private:
  esp_mqtt_client_handle_t client_;
  bool is_connected;
  std::string broker_host_;
  std::string will_topic_;
  std::string will_message_;
  std::function<void()> connect_function_;
  std::function<void()> disconnect_function_;
  std::unordered_map<uint16_t, SubscribeTopic> subscribe_topics_;
};

}  // namespace IrrigationSystem

#endif  // IRRIGATION_SYSTEM_WATCHER_H_
// EOF
