// ESP32 Irrigation System
// (C)2024 bekki.jp

// Include ----------------------
#include "water_flow_sensor.h"

#include <functional>
#include <string>

#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "message_queue.h"
#include "gpio_control.h"
#include "logger.h"
#include "util.h"

namespace {
  constexpr float WATER_FLOW_COUNT_TO_CUBIC_CENTIMETRES = 1000.0f / 11.0f;
}

namespace IrrigationSystem {

WaterFlowSensor::WaterFlowSensor()
    : Task(TASK_NAME, PRIORITY, CORE_ID),
      gptimer_(),
      queue_(),
      pcnt_unit_(nullptr),
      sensor_hz_(0),
      total_count_(0) {}

void WaterFlowSensor::Initialize() {
  ESP_LOGI(TAG, "Initialize WaterFlowSensor");
 
  // Create MessageQueue
  if (!queue_.Create(1)) {
    ESP_LOGE(TAG, "Creating queue failed");
    return;
  }

  // Create Timer
  gptimer_.Create(1000000u, &WaterFlowSensor::TimerCallback,
                  &queue_);  // 1MHz, 1 tick=1ns

  // Pulse Counter 
  pcnt_unit_config_t pcnt_unit_config = {
      .low_limit = INT16_MIN,
      .high_limit = INT16_MAX,
      .intr_priority = 0,
      .flags =
          {
              .accum_count = 1,
          },
  };
  ESP_ERROR_CHECK(pcnt_new_unit(&pcnt_unit_config, &pcnt_unit_));

  // GPIO ポート設定 (EdgeはGPIO / Levelは仮想GPIO)
  ESP_LOGI(TAG, "install pcnt channels");
  pcnt_chan_config_t pcnt_channel_config = {
      .edge_gpio_num = static_cast<gpio_num_t>(CONFIG_WATER_FLOW_INPUT_GPIO_NO),
      .level_gpio_num = GPIO_NUM_NC,
      .flags =
          {
              .invert_edge_input = 0,
              .invert_level_input = 0,
              .virt_edge_io_level = 0,
              .virt_level_io_level = 0,
              .io_loop_back = 0,
          },
  };
  pcnt_channel_handle_t pcnt_channel_handle = NULL;
  ESP_ERROR_CHECK(
      pcnt_new_channel(pcnt_unit_, &pcnt_channel_config, &pcnt_channel_handle));

  // グリッチフィルターを設定
  ESP_LOGI(TAG, "set glitch filter");
  pcnt_glitch_filter_config_t pcnt_filter_config = {
      .max_glitch_ns = 1000,  // ns
  };
  ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit_, &pcnt_filter_config));

  // 立ち下がりエッジ検知時のインクリメントを設定
  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
      pcnt_channel_handle, PCNT_CHANNEL_EDGE_ACTION_HOLD,
      PCNT_CHANNEL_EDGE_ACTION_INCREASE));

  ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit_));
  ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit_));

  ESP_LOGI(TAG, "Finish initialized Water Flow Sensor");
}

void WaterFlowSensor::Update() {
  ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit_));

  // TimerStart
  gptimer_.Start(1000000ull);

  int last_pc = 0;
  int32_t receive_data = 0;
  while (true) {
    if (queue_.ReceiveWait(&receive_data, 1100)) {
      int pulse_count = 0;
      ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit_, &pulse_count));
      const int hz =
          pulse_count - last_pc + (pulse_count < last_pc ? INT16_MAX : 0);

      //ESP_LOGI(TAG, "hz: %d lpc:%d pc:%d", hz, last_pc, pulse_count);
      last_pc = pulse_count;

      sensor_hz_ = hz;
      total_count_ += hz;
    } else {
      ESP_LOGW(TAG, "Missed one count event");
    }
  }
}

void WaterFlowSensor::StartMeasurement()
{
  GPIO::SetLevel(CONFIG_WATER_FLOW_OUTPUT_GPIO_NO, 1);
  total_count_ = 0;
}

int32_t WaterFlowSensor::FinishMeasurement()
{
  GPIO::SetLevel(CONFIG_WATER_FLOW_OUTPUT_GPIO_NO, 0);
  return total_count_;
}

int32_t WaterFlowSensor::GetSensorHz() const
{
  return sensor_hz_;
}

bool IRAM_ATTR WaterFlowSensor::TimerCallback(gptimer_handle_t timer,
                                        const gptimer_alarm_event_data_t *edata,
                                        void *user_data) {
  MessageQueue<int32_t> *const queue =
      static_cast<MessageQueue<int32_t> *>(user_data);
  return queue->SendFromISR(0);
}

float WaterFlowSensor::CountToCubicCentimetres(const int32_t count) {
  return static_cast<float>(count) * WATER_FLOW_COUNT_TO_CUBIC_CENTIMETRES;
}

}  // namespace IrrigationSystem

// EOF
