#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

template <typename T>
class MessageQueue {
 public:
  MessageQueue() : queue_(nullptr) {}

  virtual ~MessageQueue() { Destroy(); }

  bool Create(const int queueSize = 1) {
    if (queue_) {
      return true;
    }
    queue_ = xQueueCreate(queueSize, sizeof(T));
    if (!queue_) {
      return false;
    }
    return true;
  }

  void Destroy() {
    if (queue_) {
      xQueueReset(queue_);
      vQueueDelete(queue_);
      queue_ = nullptr;
    }
  }

  bool ReceiveWait(T *const receive_data, const int max_wait_millisecond) {
    if (!queue_) {
      return false;
    }
    return xQueueReceive(queue_, receive_data,
                         pdMS_TO_TICKS(max_wait_millisecond));
  }

  bool ReceiveNonBlock(T *const receive_data) {
    if (!queue_) {
      return false;
    }
    return xQueueReceive(queue_, receive_data, 0);
  }

  bool ReceiveBlock(T *const receive_data) {
    if (!queue_) {
      return false;
    }
    return xQueueReceive(queue_, receive_data, portMAX_DELAY);
  }

  bool SendFromISR(const T &data) {
    if (!queue_) {
      return false;
    }
    BaseType_t high_task_awoken = pdFALSE;
    xQueueSendFromISR(queue_, &data, &high_task_awoken);
    return (high_task_awoken == pdTRUE);
  }

 private:
  QueueHandle_t queue_;
};

#endif  // MESSAGE_QUEUE_H_
