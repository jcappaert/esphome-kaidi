#include "kaidi.h"
#include "esphome/core/log.h"

#include <cinttypes>

namespace esphome {
namespace kaidi {

static const char *const TAG = "kaidi.sensor";
// static const uint8_t ASCII_CR = 0x0D;
// static const uint8_t ASCII_NBSP = 0xFF;
// static const int MAX_DATA_LENGTH_BYTES = 6;

static const uint8_t KAIDI_START_BYTE = 0x68;
static const uint8_t KAIDI_FIXED_BYTE = 0x01;
static const uint8_t KAIDI_END_BYTE = 0x16;
static const uint8_t KAIDI_MODE_RECEIVED = 0x00;

bool kaidi_rx_enabled = true;

void KaidiComponent::setup() { this->publish_state(""); }

void KaidiComponent::loop() {
  
  //this->flush();
  while ((this->available() >= KAIDI_PACKET_SIZE) && kaidi_rx_enabled) { 
    ESP_LOGV(TAG, "Reading packet, UART Available: %i", this->available());
    KaidiComponent::KaidiError error = this->read_packet_();
    if ((error != KAIDI_ERROR_NONE) && (error != KAIDI_ERROR_NONE_COMPLETED)) {
      ESP_LOGW(TAG, "Error: %d", error);
    }

    if (error == KAIDI_ERROR_NONE_COMPLETED) {
      // we've gotten a complete packet successfully
      set_kaidi_rx_state(false); 
    }

  }
}

KaidiComponent::KaidiError KaidiComponent::read_packet_() {
  uint8_t packet[KAIDI_PACKET_SIZE];
  packet[KAIDI_PACKET_START_BYTE] = this->read();

  // check for the first byte being the packet start code
  if (packet[KAIDI_PACKET_START_BYTE] != KAIDI_START_BYTE) {
    // just out of sync, ignore until we are synced up
    return KAIDI_ERROR_NONE;
  }

  packet[KAIDI_PACKET_FIXED_BYTE] = this->read();
  // check for the fixed byte
  if (packet[KAIDI_PACKET_FIXED_BYTE] != KAIDI_FIXED_BYTE) {
    return KAIDI_ERROR_BYTE2_MISMATCH;
  }

  // Read mode and payload bytes
  if (!this->read_array(&(packet[KAIDI_PACKET_MODE]), KAIDI_PACKET_CHECKSUM - KAIDI_PACKET_MODE)) {
    return KAIDI_ERROR_PAYLOAD;
  }

  // Read checksum byte and check checksum
  packet[KAIDI_PACKET_CHECKSUM] = this->read();
  uint8_t checksum_size = KAIDI_PACKET_CHECKSUM - KAIDI_PACKET_FIXED_BYTE;
  uint8_t checksum_bytes[checksum_size];
  memcpy(checksum_bytes, packet + KAIDI_PACKET_FIXED_BYTE, checksum_size);
  uint8_t checksum =  checksum_(checksum_bytes, checksum_size);
  // check for checksum
  if (packet[KAIDI_PACKET_CHECKSUM] != checksum) {
    ESP_LOGD(TAG, "Faulty checksum: %i, expected checksum %i", checksum, packet[KAIDI_PACKET_CHECKSUM]);
    return KAIDI_ERROR_CHECKSUM;
  }

  packet[KAIDI_PACKET_END_BYTE] = this->read();
  if (packet[KAIDI_PACKET_END_BYTE] != KAIDI_END_BYTE) {
    return KAIDI_ERROR_PACKET_END_CODE_MISSMATCH;
  }

  ESP_LOGV(TAG, "Packet: %s", bytesToHexString(packet, sizeof(packet)/sizeof(packet[0])));

  KaidiReading reading;

  // convert packet into the reading struct
  reading.height = this->bytes_to_height_(
    std::vector<uint8_t>(packet + KAIDI_PACKET_PAYLOAD, packet + KAIDI_PACKET_CHECKSUM).data());

  ESP_LOGD(TAG, "Height:    %u", reading.height);

  char buf[8];
  sprintf(buf, "%u", reading.height);
  this->publish_state(buf);
  if (this->do_reset_) {
    this->set_timeout(1000, [this]() { this->publish_state(""); });
  }

  return KAIDI_ERROR_NONE_COMPLETED;
}

uint16_t KaidiComponent::bytes_to_height_(uint8_t bytes[]) {
  uint16_t height = bytes[0] << 8 | bytes[1];
  return height;
}


void KaidiComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Kaidi Sensor:");
  LOG_TEXT_SENSOR("", "Tag", this);
  // As specified in the sensor's data sheet
  this->check_uart_settings(9600, 1, esphome::uart::UART_CONFIG_PARITY_NONE, 8);
}

uint8_t KaidiComponent::checksum_(uint8_t bytes[], uint8_t length) {
  uint16_t sum = 0; 
    for (size_t i = 0; i < length; ++i) {
        sum += bytes[i];
    }
  return static_cast<uint8_t>(sum);
}

char* KaidiComponent::bytesToHexString(const uint8_t* byteArray, size_t length) {
    if (byteArray == NULL || length == 0) {
        char* empty = (char*)malloc(3);
        if (empty == NULL) return NULL; // Memory allocation failed
        strcpy(empty, "{}");
        return empty;
    }

    // Estimate the required length: "{ ", "0xNN, ", " }", and null terminator
    size_t bufferSize = 2 + (length * 6) + 2; // Braces + bytes + spaces
    char* result = (char*)malloc(bufferSize);
    if (result == NULL) {
        return NULL; // Memory allocation failed
    }

    // Build the string
    char* ptr = result;
    ptr += sprintf(ptr, "{ ");
    for (size_t i = 0; i < length; ++i) {
        ptr += sprintf(ptr, "0x%02X", byteArray[i]);
        if (i < length - 1) {
            ptr += sprintf(ptr, ", ");
        }
    }
    sprintf(ptr, " }"); // Close the brace

    return result; // Caller must free this memory
}


void KaidiComponent::set_kaidi_rx_state(bool enabled) {
    kaidi_rx_enabled = enabled;
}


}  // namespace kaidi
}  // namespace esphome