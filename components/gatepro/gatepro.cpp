// VERY GOOD STATE
#include "esphome/core/log.h"
#include "gatepro.h"

namespace esphome {
namespace gatepro {

////////////////////////////////////
static const char* TAG = "gatepro";

////////////////////////////////////
// Device logic below
///////////////////////////////////

// fn to abstract CoverOperations
void GatePro::queue_gatepro_cmd(GateProCmd cmd) {
  this->tx_queue.push(GateProCmdMapping.at(cmd));
}

// preprocessor (for the case if multiple messages read at once)
void GatePro::preprocess(std::string msg) {
    // check if there are multiple messages in one msg
    uint8_t delimiter_location = msg.find(this->delimiter);
    uint8_t msg_length = msg.length();
    if (msg_length > delimiter_location + this->delimiter_length) {
        std::string msg1 = msg.substr(0, delimiter_location + this->delimiter_length);
        //this->process(msg1);
        this->rx_queue.push(msg1);
        std::string msg2 = msg.substr(delimiter_location + this->delimiter_length);
        this->preprocess(msg2);
        return;
    }

    //this->process(msg);
    this->rx_queue.push(msg);
}

// main motor message processor
void GatePro::process(std::string msg) {
    ESP_LOGD(TAG, "UART RX: %s", (const char*)msg.c_str());

    // Answer to RS (read status)
    // example: ACK RS:00,80,C4,C6,3E,16,FF,FF,FF\r\n
    //                          ^- percentage in hex
    if (msg.substr(0, 6) == "ACK RS") {
        // status only matters when in motion (operation not finished) 
        if (this->operation_finished) {
          return;
        }
        msg = msg.substr(16, 2);
        int percentage = stoi(msg, 0, 16);
        // percentage correction with known offset, if necessary
        if (percentage > 100) {
          percentage -= this->known_percentage_offset;
        }
        this->position = (float)percentage / 100;

        return;
    }

    // Event message from the motor
    // example: $V1PKF0,17,Closed;src=0001\r\n
    if (msg.substr(0, 7) == "$V1PKF0") {
        if (msg.substr(11, 7) == "Opening") {
            this->operation_finished = false;
            // invoking in case gate is controlled from remote
            if (this->current_operation != cover::COVER_OPERATION_OPENING) {
              this->make_call().set_command_open().perform();
            }
            return;
        }
        if (msg.substr(11, 6) == "Opened") {
            this->operation_finished = true;
            // invoking in case gate is controlled from remote
            /*if (this->current_operation != cover::COVER_OPERATION_OPENING) {
              this->make_call().set_command_open().perform();
            }*/
            this->position = cover::COVER_OPEN;
            this->make_call().set_command_stop().perform();
            return;
        }
        if (msg.substr(11, 7) == "Closing") {
            this->operation_finished = false;
            // invoking in case gate is controlled from remote
            if (this->current_operation != cover::COVER_OPERATION_CLOSING) {
              this->make_call().set_command_close().perform();
            }
            return;
        }
        if (msg.substr(11, 6) == "Closed") {
            this->operation_finished = true;
            // invoking in case gate is controlled from remote
            /*if (this->current_operation != cover::COVER_OPERATION_CLOSING) {
              this->make_call().set_command_close().perform();
            }*/
            this->position = cover::COVER_CLOSED;
            this->make_call().set_command_stop().perform();
            return;
        }
        if (msg.substr(11, 7) == "Stopped") {
            // invoking in case gate is controlled from remote
            this->make_call().set_command_stop().perform();
            return;
        }
    }
}


// converter
std::string GatePro::convert(uint8_t* bytes, size_t len) {
  std::string res;
  char buf[5];
  for (size_t i = 0; i < len; i++) {
    if (bytes[i] == 7) {
      res += "\\a";
    } else if (bytes[i] == 8) {
      res += "\\b";
    } else if (bytes[i] == 9) {
      res += "\\t";
    } else if (bytes[i] == 10) {
      res += "\\n";
    } else if (bytes[i] == 11) {
      res += "\\v";
    } else if (bytes[i] == 12) {
      res += "\\f";
    } else if (bytes[i] == 13) {
      res += "\\r";
    } else if (bytes[i] == 27) {
      res += "\\e";
    } else if (bytes[i] == 34) {
      res += "\\\"";
    } else if (bytes[i] == 39) {
      res += "\\'";
    } else if (bytes[i] == 92) {
      res += "\\\\";
    } else if (bytes[i] < 32 || bytes[i] > 127) {
      sprintf(buf, "\\x%02X", bytes[i]);
      res += buf;
    } else {
      res += bytes[i];
    }
  }
  //ESP_LOGD(TAG, "%s", res.c_str());
  return res;
}

void GatePro::read_uart() {
    int available = this->available();
    if (!available) {
        return;
    }
    
    uint8_t* bytes = new byte[available];
    this->read_array(bytes, available);
    this->preprocess(this->convert(bytes, available));
}


////////////////////////////////////
// Abstract (cover) logic below
///////////////////////////////////

// set supported traits
cover::CoverTraits GatePro::get_traits() {
  auto traits = cover::CoverTraits();

  traits.set_is_assumed_state(false);
  traits.set_supports_position(true);
  traits.set_supports_tilt(false);
  traits.set_supports_toggle(true);
  traits.set_supports_stop(true);
  return traits;
}

void GatePro::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    this->start_direction_(cover::COVER_OPERATION_IDLE);
    //this->publish_state();
    return;
  }

  //
  if (call.get_position().has_value()) {
    auto pos = *call.get_position();
    if (pos == this->position) {
      //this->publish_state();
      return;
    } else {
      // sanitize position input - there has to be a min diff.
      if (abs(pos - this->position) < this->min_pos_diff) {
        return;
      }
      auto op = pos < this->position ? cover::COVER_OPERATION_CLOSING : cover::COVER_OPERATION_OPENING;
      //this->target_position_ = pos;
      this->start_direction_(op);
    }
  }
}

// cover logic
void GatePro::start_direction_(cover::CoverOperation dir) {
  if (this->current_operation == dir ) {
    return;
  }

  switch (dir) {
    case cover::COVER_OPERATION_IDLE:
      // if just setting logic to idling but opening / closing is finished
      if (this->operation_finished) {
        break;
      }
      this->queue_gatepro_cmd(GATEPRO_CMD_STOP);
      break;
    case cover::COVER_OPERATION_OPENING:
      this->last_operation_ = dir;
      this->queue_gatepro_cmd(GATEPRO_CMD_OPEN);
      break;
    case cover::COVER_OPERATION_CLOSING:
      this->last_operation_ = dir;
      this->queue_gatepro_cmd(GATEPRO_CMD_CLOSE);
      break;
    default:
      return;
  }

  this->current_operation = dir;
}

////////////////////////////////////
// Sensor logic below
///////////////////////////////////

void GatePro::setup() {
    ESP_LOGD(TAG, "Setting up GatePro component..");
    this->make_call().set_command_close().perform();
    this->make_call().set_command_stop().perform();
    this->operation_finished = true;
    this->queue_gatepro_cmd(GATEPRO_CMD_READ_STATUS);
    this->blocker = false;
}

void GatePro::correction_after_operation() {
    if (this->operation_finished) {
      if (this->current_operation == cover::COVER_OPERATION_IDLE &&
          this->last_operation_ == cover::COVER_OPERATION_CLOSING &&
          this->position != cover::COVER_CLOSED) {
        this->position = cover::COVER_CLOSED;
        return;
      }

      if (this->current_operation == cover::COVER_OPERATION_IDLE &&
          this->last_operation_ == cover::COVER_OPERATION_OPENING &&
          this->position != cover::COVER_OPEN) {
        this->position = cover::COVER_OPEN;
      }
    }
}

void GatePro::publish() {
    // publish on each tick
    /*if (this->position_ == this->position) {
      return;
    }*/

    this->position_ = this->position;
    this->publish_state();
}

void GatePro::update() {
    this->publish();
    
    /*if (this->blocker){
      ESP_LOGD(TAG, "TOO QUICK, SLOW DOWN!");
      return;
    }

    this->blocker = true;*/


    // if gate is not stationary, continuously read status
    if (this->current_operation != cover::COVER_OPERATION_IDLE) {
        this->queue_gatepro_cmd(GATEPRO_CMD_READ_STATUS);
    }

    // correction after an opening / closing operation finished
    this->correction_after_operation();

    // debug
    //this->debug();

    //this->blocker = false;
}


void GatePro::loop() {
  // keep reading uart for changes
  this->read_uart();
  if (this->rx_queue.size()) {
    this->process(this->rx_queue.front());
    this->rx_queue.pop();
  }

  // send first in queue UART cmd
  if (this->tx_queue.size()) {
    //this->write_str(this->tx_queue.front());
    std::string temp = tx.tx_queue.front();
    std::vector<uint8_t> cmdrn(temp.begin(), temp.end());
    this->write_array(cmdrn);
    //ESP_LOGD(TAG, "UART TX: %s", this->tx_queue.front());
    this->tx_queue.pop();
  }
}

void GatePro::dump_config(){
    ESP_LOGCONFIG(TAG, "GatePro sensor dump config");
}

void GatePro::debug() {
  this->publish_state();
  ESP_LOGD(TAG, "============================");

  ESP_LOGD(TAG, cover::cover_operation_to_str(this->current_operation));
  ESP_LOGD(TAG, cover::cover_operation_to_str(this->last_operation_));
  ESP_LOGD(TAG, (const char*)std::to_string(this->operation_finished).c_str());
  ESP_LOGD(TAG, (const char*)std::to_string(this->position).c_str());

  ESP_LOGD(TAG, "");
  ESP_LOGD(TAG, "");
}

}  // namespace gatepro
}  // namespace esphome