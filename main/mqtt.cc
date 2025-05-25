// ESP32 InformationPanel
// (C)2025 bekki.jp

#include "mqtt.h"

#include <esp_system.h>

#include "util.h"

namespace IrrigationSystem {

const std::function<void(MQTTClient&, esp_mqtt_client_handle_t client,
                         const esp_mqtt_event_handle_t event)>
    MQTTClient::EVENT_FUNCTIONS[MQTT_USER_EVENT + 1] = {
        &MQTTClient::EventError,          // MQTT_EVENT_ERROR
        &MQTTClient::EventConnected,      // MQTT_EVENT_CONNECTED
        &MQTTClient::EventDisconnected,   // MQTT_EVENT_DISCONNECTED
        &MQTTClient::EventSubscribed,     // MQTT_EVENT_SUBSCRIBED
        &MQTTClient::EventUnsubscribed,   // MQTT_EVENT_UNSUBSCRIBED
        &MQTTClient::EventPublished,      // MQTT_EVENT_PUBLISHED
        &MQTTClient::EventData,           // MQTT_EVENT_DATA
        &MQTTClient::EventBeforeConnect,  // MQTT_EVENT_BEFORE_CONNECT
        &MQTTClient::EventDeleted,        // MQTT_EVENT_DELETED
        &MQTTClient::EventUser,           // MQTT_USER_EVENT
};

MQTTClient::MQTTClient()
    : client_(nullptr),
      broker_host_(),
      will_topic_(),
      will_message_(),
      connect_function_(),
      disconnect_function_(),
      subscribe_topics_() {}

MQTTClient::~MQTTClient() = default;

void MQTTClient::SetConnectEvent(std::function<void()> func) {
  connect_function_ = func;
}

void MQTTClient::SetDisconnectEvent(std::function<void()> func) {
  disconnect_function_ = func;
}

void MQTTClient::SetBrokerHost(const std::string& host) { broker_host_ = host; }

void MQTTClient::AddSubscribeTopic(SubscribeTopic subscribe) {
  subscribe_topics_.emplace(subscribe.subscribe_id_, subscribe);
}

void MQTTClient::SetWillMessage(const std::string& topic, const std::string& message) {
  will_topic_ = topic;
  will_message_ = message;
}


void MQTTClient::Start() {
  esp_mqtt_client_config_t mqtt5_cfg = {};
  mqtt5_cfg.session.protocol_ver = MQTT_PROTOCOL_V_5;

  mqtt5_cfg.broker.address.uri = broker_host_.c_str();
  mqtt5_cfg.network.disable_auto_reconnect = false;

  mqtt5_cfg.credentials.client_id = CONFIG_MQTT_CLIENT_ID;
  // mqtt5_cfg.credentials.username = "";
  // mqtt5_cfg.credentials.authentication.password = "";

  if (0 < will_topic_.length()) {
    mqtt5_cfg.session.last_will.topic = will_topic_.c_str();
    mqtt5_cfg.session.last_will.msg = will_message_.c_str();
    mqtt5_cfg.session.last_will.msg_len = 0;
    mqtt5_cfg.session.last_will.qos = 0;
    mqtt5_cfg.session.last_will.retain = false;
  }

  client_ = esp_mqtt_client_init(&mqtt5_cfg);
  esp_mqtt_client_register_event(
      client_, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
      MQTTClient::Mqtt5EventHandler, this);
  esp_mqtt_client_start(client_);
}

void MQTTClient::Publish(const std::string& topic, const std::string& data) {
  if (!client_) {
    ESP_LOGW(TAG, "MQTT Client is null");
    return;
  }

  int32_t msg_id = esp_mqtt_client_publish(client_, topic.c_str(), data.c_str(), 0, 0, 0);
  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}

void MQTTClient::EventError(esp_mqtt_client_handle_t client,
                            const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
  ESP_LOGI(TAG, "MQTT5 return code is %d",
           event->error_handle->connect_return_code);
  if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
    ESP_LOGI(TAG, "Last errno string (%s)",
             strerror(event->error_handle->esp_transport_sock_errno));
  }
}

void MQTTClient::EventConnected(esp_mqtt_client_handle_t client,
                                const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

  for (const auto& pair : subscribe_topics_) {
    esp_mqtt5_client_set_subscribe_property(client, &pair.second.property_);
    int msg_id =
        esp_mqtt_client_subscribe(client, pair.second.topic_.c_str(), 0);
    ESP_LOGI(TAG, "Sent Subscribe Successful msg_id:%d topic:%s", msg_id,
             pair.second.topic_.c_str());
  }

  if (connect_function_) {
    connect_function_();
  }
}

void MQTTClient::EventDisconnected(esp_mqtt_client_handle_t client,
                                   const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

  if (connect_function_) {
    disconnect_function_();
  }
}

void MQTTClient::EventSubscribed(esp_mqtt_client_handle_t client,
                                 const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
}

void MQTTClient::EventUnsubscribed(esp_mqtt_client_handle_t client,
                                   const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
}

void MQTTClient::EventPublished(esp_mqtt_client_handle_t client,
                                const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
}

void MQTTClient::EventData(esp_mqtt_client_handle_t client,
                           const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_DATA");

  if (event->topic == nullptr || event->data == nullptr ||
      event->property == nullptr) {
    ESP_LOGW(TAG, "MQTT_EVENT_DATA is null");
    return;
  }

  std::string topic(event->topic, event->topic_len);
  std::string data(event->data, event->data_len);
  const uint16_t subscribe_id = event->property->subscribe_id;
  ESP_LOGI(TAG, "Recv topic:%s subid:%d data:%s", topic.c_str(), subscribe_id,
           data.c_str());

  auto topic_iter = subscribe_topics_.find(subscribe_id);
  if (topic_iter != subscribe_topics_.end()) {
    topic_iter->second.function_(data);
  } else {
    ESP_LOGI(TAG, "Not Found Function topic:%s subscribe_id:%d", topic.c_str(),
             subscribe_id);
  }
}

void MQTTClient::EventBeforeConnect(esp_mqtt_client_handle_t client,
                                    const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
}

void MQTTClient::EventDeleted(esp_mqtt_client_handle_t client,
                              const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_EVENT_DELETED");
}

void MQTTClient::EventUser(esp_mqtt_client_handle_t client,
                           const esp_mqtt_event_handle_t event) {
  ESP_LOGI(TAG, "MQTT_USER_EVENT");
}

void MQTTClient::Mqtt5EventHandler(void* handler_args, esp_event_base_t base,
                                   int32_t event_id, void* event_data) {
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32,
           base, event_id);

  const esp_mqtt_event_handle_t event =
      static_cast<esp_mqtt_event_handle_t>(event_data);
  esp_mqtt_client_handle_t client = event->client;

  ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32,
           esp_get_free_heap_size(), esp_get_minimum_free_heap_size());

  if (!handler_args) {
    return;
  }

  if (MQTT_EVENT_ERROR <= event_id && event_id <= MQTT_USER_EVENT) {
    EVENT_FUNCTIONS[event_id](*static_cast<MQTTClient*>(handler_args), client,
                              event);
  } else {
    ESP_LOGW(TAG, "Other event id:%d", event->event_id);
  }
}

} // IrrigationSystem

// EOF
