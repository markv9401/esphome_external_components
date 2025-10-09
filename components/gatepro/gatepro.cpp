#include "esphome/core/log.h"
#include "gatepro.h"
#include <vector>

namespace esphome {
namespace gatepro {

////////////////////////////////////
static const char* TAG = "gatepro";

////////////////////////////////////////////
// Helper / misc functions
////////////////////////////////////////////
void GatePro::queue_gatepro_cmd(GateProCmd cmd) {
   this->tx_queue.push(GateProCmdMapping.at(cmd));
}

void GatePro::publish() {
   // publish on each tick
   // might have to comment out this one
   if (this->position_ == this->position) {
      return;
   }

   this->position_ = this->position;
   this->publish_state();
}

////////////////////////////////////////////
// GatePro logic functions
////////////////////////////////////////////
void GatePro::process() {
   if (!this->rx_queue.size()) {
      return;
   }
   std::string msg = this->rx_queue.front();
   this->rx_queue.pop();

   //ESP_LOGD(TAG, "UART RX: %s", (const char*)msg.c_str());
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

   // Read param example: ACK RP,1:1,0,0,1,2,2,0,0,0,3,0,0,3,0,0,0,0\r\n"
   if (msg.substr(0, 6) == "ACK RP") {
      this->parse_params(msg);
      return;
   }

   // ACK WP example: ACK WP,1\r\n
   if (msg.substr(0, 6) == "ACK WP") {
      ESP_LOGD(TAG, "Write params acknowledged");
      return;
   }

   // Event message from the motor
   // example: $V1PKF0,17,Closed;src=0001\r\n
   if (msg.substr(0, 7) == "$V1PKF0") {
      if (msg.substr(11, 7) == "Opening") {
      this->operation_finished = false;
      this->current_operation = cover::COVER_OPERATION_OPENING;
      this->last_operation_ = cover::COVER_OPERATION_OPENING;
      return;
      }
   if (msg.substr(11, 6) == "Opened") {
      this->operation_finished = true;
      this->target_position_ = 0.0f;
      this->current_operation = cover::COVER_OPERATION_IDLE;
      return;
   }
   if (msg.substr(11, 7) == "Closing") {
      this->operation_finished = false;
      this->current_operation = cover::COVER_OPERATION_CLOSING;
      this->last_operation_ = cover::COVER_OPERATION_CLOSING;
      return;
   }
   if (msg.substr(11, 6) == "Closed") {
      this->operation_finished = true;
      this->target_position_ = 0.0f;
      this->current_operation = cover::COVER_OPERATION_IDLE;
      return;
   }
   if (msg.substr(11, 7) == "Stopped") {
      this->target_position_ = 0.0f;
      this->current_operation = cover::COVER_OPERATION_IDLE;
      return;
   }
  }
}

////////////////////////////////////////////
// Cover component logic functions
////////////////////////////////////////////
void GatePro::control(const cover::CoverCall &call) {
   if (call.get_stop()) {
      this->start_direction_(cover::COVER_OPERATION_IDLE);
      return;
   }

   if (call.get_position().has_value()) {
      auto pos = *call.get_position();
      if (pos == this->position) {
         return;
      }
      auto op = pos < this->position ? cover::COVER_OPERATION_CLOSING : cover::COVER_OPERATION_OPENING;
      this->target_position_ = pos;
      this->start_direction_(op);
   }
}

void GatePro::start_direction_(cover::CoverOperation dir) {
   if (this->current_operation == dir ) {
      return;
   }

   switch (dir) {
      case cover::COVER_OPERATION_IDLE:
         if (this->operation_finished) {
            break;
         }
         this->queue_gatepro_cmd(GATEPRO_CMD_STOP);
         break;
      case cover::COVER_OPERATION_OPENING:
         this->queue_gatepro_cmd(GATEPRO_CMD_OPEN);
         break;
      case cover::COVER_OPERATION_CLOSING:
         this->queue_gatepro_cmd(GATEPRO_CMD_CLOSE);
         break;
      default:
         return;
   }
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

void GatePro::stop_at_target_position() {
   if (this->target_position_ &&
         this->target_position_ != cover::COVER_OPEN &&
         this->target_position_ != cover::COVER_CLOSED) {
      const float diff = abs(this->position - this->target_position_);
      if (diff < this->acceptable_diff) {
         this->make_call().set_command_stop().perform();
      }
   }
}

////////////////////////////////////////////
// UART operations
////////////////////////////////////////////
void GatePro::read_uart() {
   // check if anything on UART buffer
   int available = this->available();
   if (!available) {
      return;
   }
   
   // read the whole buffer into our own buffer
   // (if there's remainder from previous msgs, concatenate)
   uint8_t* bytes = new byte[available];
   this->read_array(bytes, available);
   this->msg_buff += this->convert(bytes, available);

   // find delimiter, thus a whole msg, send it to processor, then remove from buffer and keep remainder (if any)
   size_t pos = this->msg_buff.find(this->delimiter);
   if (pos != std::string::npos) {
      std::string sub = this->msg_buff.substr(0, pos + this->delimiter_length);
      this->rx_queue.push(sub);
      ESP_LOGD(TAG, "UART RX: %s", sub.c_str());
      this->msg_buff = this->msg_buff.substr(pos + this->delimiter_length, this->msg_buff.length() - pos);
   }
}

void GatePro::write_uart() {
   if (this->tx_queue.size()) {
      std::string tmp = this->tx_queue.front();
      tmp += this->tx_delimiter;
      const char* out = tmp.c_str();
      this->write_str(out);
      ESP_LOGD(TAG, "UART TX: %s", out);
      this->tx_queue.pop();
   }
}

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


////////////////////////////////////////////
// Paramater functions
////////////////////////////////////////////
void GatePro::set_speed_4() {
   ESP_LOGD(TAG, "SET SPEEEEEED FOO");
   this->queue_gatepro_cmd(GATEPRO_CMD_READ_PARAMS);

   this->paramTaskQueue.push(
      [this](){
         ESP_LOGD(TAG, "HALÓÓÓÓ ZSÓÓÓTI");
         this->params[4] = 4;
         this->write_params();
      });
}

void GatePro::parse_params(std::string msg) {
   this->params.clear();
   // example: ACK RP,1:1,0,0,1,2,2,0,0,0,3,0,0,3,0,0,0,0\r\n"
   //                   ^-9  
   msg = msg.substr(9, 33);
   size_t start = 0;
   size_t end;

   // efficiently split on ','
   while((end = msg.find(',', start)) != std::string::npos) {
      this->params.push_back(stoi(msg.substr(start, end - start)));
      start = end + 1;
   }
   this->params.push_back(stoi(msg.substr(start)));

   ESP_LOGD(TAG, "Parsed current params:", this->params.size());
   for (size_t i = 0; i < this->params.size(); ++i) {
      ESP_LOGD(TAG, "  [%zu] = %d", i, this->params[i]);
   }

   // write new params if any task is up
   while (!this->paramTaskQueue.empty()) {
      auto task = this->paramTaskQueue.front();
      this->paramTaskQueue.pop();
      task();
   }
}

void GatePro::write_params() {
   std::string msg = "WP,1:";
   for (size_t i = 0; i < this->params.size(); i++) {
      msg += to_string(this->params[i]);
      if (i != this->params.size() -1) {
         msg += ",";
      }
   }
   //msg += ";src=P00287D7";
   ESP_LOGD(TAG, "BUILT PARAMS: %s", msg.c_str());
   this->tx_queue.push(msg.c_str());
}

////////////////////////////////////////////
// Component functions
////////////////////////////////////////////
cover::CoverTraits GatePro::get_traits() {
   auto traits = cover::CoverTraits();
   traits.set_is_assumed_state(false);
   traits.set_supports_position(true);
   traits.set_supports_tilt(false);
   traits.set_supports_toggle(true);
   traits.set_supports_stop(true);
   return traits;
}

void GatePro::setup() {
   ESP_LOGD(TAG, "Setting up GatePro component..");
   this->last_operation_ = cover::COVER_OPERATION_CLOSING;
   this->current_operation = cover::COVER_OPERATION_IDLE;
   this->operation_finished = true;
   this->queue_gatepro_cmd(GATEPRO_CMD_READ_STATUS);
   this->target_position_ = 0.0f;
   this->queue_gatepro_cmd(GATEPRO_CMD_READ_PARAMS);

   // set up buttons
   if (btn_speed_4) {
      this->btn_speed_4->add_on_press_callback([this](){this->set_speed_4();});
   } 
}

void GatePro::update() {
   this->publish();
   this->stop_at_target_position();

   this->write_uart();

   if (this->current_operation != cover::COVER_OPERATION_IDLE) {
      this->queue_gatepro_cmd(GATEPRO_CMD_READ_STATUS);
   }

   this->correction_after_operation();
}

void GatePro::loop() {
   // keep reading uart for changes
   this->read_uart();
   this->process();
}

void GatePro::dump_config(){
   ESP_LOGCONFIG(TAG, "GatePro sensor dump config");
}

}  // namespace gatepro
}  // namespace esphome
