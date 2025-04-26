#pragma once

#include <string>

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace kaidi {

class KaidiComponent : public text_sensor::TextSensor, public Component, public uart::UARTDevice {
 public:
  enum KaidiError {
    KAIDI_ERROR_NONE = 0,
    KAIDI_ERROR_PACKET_START_CODE_MISMATCH,
    KAIDI_ERROR_BYTE2_MISMATCH,
    KAIDI_ERROR_PAYLOAD,
    KAIDI_ERROR_CHECKSUM,
    KAIDI_ERROR_PACKET_SIZE,
    KAIDI_ERROR_PACKET_END_CODE_MISSMATCH,
    KAIDI_ERROR_NONE_COMPLETED,
  };

  struct KaidiReading {
    uint16_t height;
  };
  // Nothing really public.

  // ========== INTERNAL METHODS ==========
  void setup() override;
  void loop() override;
  void dump_config() override;
  void set_kaidi_rx_state(bool enabled);

  void set_do_reset(bool do_reset) { this->do_reset_ = do_reset; }

 private:
  
  enum KaidiPacket {
    KAIDI_PACKET_START_BYTE = 0,
    KAIDI_PACKET_FIXED_BYTE = 1,
    KAIDI_PACKET_MODE = 2,
    KAIDI_PACKET_PAYLOAD = 3,
    KAIDI_PACKET_CHECKSUM = 5,
    KAIDI_PACKET_END_BYTE = 6,
    KAIDI_PACKET_SIZE = 7
  };
  

  bool do_reset_;

  KaidiError read_packet_();
  char* bytesToHexString(const uint8_t* byteArray, size_t length);
  uint8_t checksum_(uint8_t bytes[], uint8_t length);
  uint16_t bytes_to_height_(uint8_t bytes[]);
};

}  // namespace kaidi
}  // namespace esphome
