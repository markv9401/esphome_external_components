#pragma once

#include <map>
#include <vector>
#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace gatepro {

enum GateProCmd : uint8_t {
   GATEPRO_CMD_OPEN,
   GATEPRO_CMD_CLOSE,
   GATEPRO_CMD_STOP,
   GATEPRO_CMD_READ_STATUS,
   GATEPRO_CMD_READ_PARAMS,
   GATEPRO_CMD_WRITE_PARAMS,
   GATEPRO_CMD_LEARN,
   GATEPRO_CMD_DEVINFO,
   GATEPRO_CMD_READ_LEARN_STATUS,
};  

const std::map<GateProCmd, const char*> GateProCmdMapping = {
   {GATEPRO_CMD_OPEN, "FULL OPEN;src=P00287D7"},
   {GATEPRO_CMD_CLOSE, "FULL CLOSE;src=P00287D7"},
   {GATEPRO_CMD_STOP, "STOP;src=P00287D7"},
   {GATEPRO_CMD_READ_STATUS, "RS;src=P00287D7"},
   {GATEPRO_CMD_READ_PARAMS, "RP,1:;src=P00287D7"},
   {GATEPRO_CMD_WRITE_PARAMS, "WP,1:"},
   {GATEPRO_CMD_LEARN, "AUTO LEARN;src=P00287D7"},
   {GATEPRO_CMD_DEVINFO, "READ DEVINFO;src=P00287D7"},
   {GATEPRO_CMD_READ_LEARN_STATUS, "READ LEARN STATUS;src=P00287D7"}
};

class GatePro : public cover::Cover, public PollingComponent, public uart::UARTDevice {
   public:
      // auto-learn btn
      esphome::button::Button *btn_learn;
      void set_btn_learn(esphome::button::Button *btn) { btn_learn = btn; }

      // get params od btn
      esphome::button::Button *btn_params_od;
      void set_btn_params_od(esphome::button::Button *btn) { btn_params_od = btn; }

      // devinfo
      text_sensor::TextSensor *txt_devinfo{nullptr};
      void set_txt_devinfo(esphome::text_sensor::TextSensor *txt) { txt_devinfo = txt; }

      // learn status
      text_sensor::TextSensor *txt_learn_status{nullptr};
      void set_txt_learn_status(esphome::text_sensor::TextSensor *txt) { txt_learn_status = txt; }

      // generic re-used param setter
      void set_param(int idx, int val);

      // speed control
      number::Number *speed_slider{nullptr};
      void set_speed_slider(number::Number *slider) { speed_slider = slider; }
      // deceleration distance slider
      number::Number *decel_dist_slider{nullptr};
      void set_decel_dist_slider(number::Number *slider) { decel_dist_slider = slider; }
      // deceleration speed slider
      number::Number *decel_speed_slider{nullptr};
      void set_decel_speed_slider(number::Number *slider) { decel_speed_slider = slider; }

      void setup() override;
      void update() override;
      void loop() override;
      void dump_config() override;
      cover::CoverTraits get_traits() override;

   protected:
      // param logic
      std::vector<int> params;
      char params_cmd[50];
      void parse_params(std::string msg);
      bool param_no_pub = false;
      void write_params();
      std::queue<std::function<void()>> paramTaskQueue;
      std::string devinfo = "N/A";

      // abstract (cover) logic
      void control(const cover::CoverCall &call) override;
      void start_direction_(cover::CoverOperation dir);

      // device logic
      std::string convert(uint8_t*, size_t);
      void process();
      void queue_gatepro_cmd(GateProCmd cmd);
      void read_uart();
      void write_uart();
      void debug();
      std::queue<const char*> tx_queue;
      std::queue<std::string> rx_queue;

      // sensor logic
      void correction_after_operation();
      cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};
      void publish();
      void stop_at_target_position();
      // how many 'ticks' to update after position hasn't changed
      const int after_tick_max = 10;
      int after_tick = after_tick_max;

      // UART parser constants
      const std::string delimiter = "\\r\\n";
      const uint8_t delimiter_length = delimiter.length();
      const std::string tx_delimiter = "\r\n";
      std::string msg_buff;

      // black magic shit..
      const int known_percentage_offset = 128;
      // maximum acceptable difference in %
      const float acceptable_diff = 0.05f;
      float target_position_;
      float position_;
      bool operation_finished;
      cover::CoverCall* last_call_;
};

}  // namespace gatepro
}  // namespace esphome
