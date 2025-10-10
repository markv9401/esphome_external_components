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
#include "esphome/components/switch/switch.h"
#include "constants.h"

namespace esphome {
namespace gatepro {

class GatePro : public cover::Cover, public PollingComponent, public uart::UARTDevice {
   public:
      // auto-learn btn
      esphome::button::Button *btn_learn;
      void set_btn_learn(esphome::button::Button *btn) { btn_learn = btn; }
      // get params od btn
      esphome::button::Button *btn_params_od;
      void set_btn_params_od(esphome::button::Button *btn) { btn_params_od = btn; }
      // remote learn btn
      esphome::button::Button *btn_remote_learn;
      void set_btn_remote_learn(esphome::button::Button *btn) { btn_remote_learn = btn; }

      // devinfo
      text_sensor::TextSensor *txt_devinfo{nullptr};
      void set_txt_devinfo(esphome::text_sensor::TextSensor *txt) { txt_devinfo = txt; }
      // learn status
      text_sensor::TextSensor *txt_learn_status{nullptr};
      void set_txt_learn_status(esphome::text_sensor::TextSensor *txt) { txt_learn_status = txt; }

      // Param setter
      void set_param(int idx, int val);
      // Sliders
      struct SliderWithIdx{
         u_int idx;
         number::Number *slider;
         SliderWithIdx(u_int idx, number::Number *slider) : idx(idx), slider(slider) {};
      };
      std::vector<SliderWithIdx> sliders_with_indices;
      void set_slider(u_int param_idx, number::Number *slider) {
         this->sliders_with_indices.push_back(SliderWithIdx(param_idx, slider));
      }
      // Switches
      struct SwitchWithIdx{
         u_int idx;
         switch_::Switch *switch_;
         SwitchWithIdx(u_int idx, switch_::Switch *switch_) : idx(idx), switch_(switch_) {};
      };
      std::vector<SwitchWithIdx> switches_with_indices;
      void set_switch(u_int param_idx, switch_::Switch *switch_) {
         this->switches_with_indices.push_back(SwitchWithIdx(param_idx, switch_));
      }
      // Buttons
      struct BtnWithCmd{
         GateProCmd cmd;
         button::Button *btn;
         BtnWithCmd(GateProCmd cmd, button::Button *btn) : cmd(cmd), btn(btn) {};
      };
      std::vector<BtnWithCmd> btns_with_cmds;
      void set_btn(std::string cmd, button::Button *btn) {
         this->btns_with_cmds.push_back(BtnWithCmd(find_cmd_by_string(cmd), btn));
      }

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
      void publish_params();
      void write_params();
      std::queue<std::function<void()>> paramTaskQueue;
      std::string devinfo = "N/A";
      GateProCmd find_cmd_by_string(const std::string &input) {
         for (const auto &[cmd, str] : GateProCmdMapping) {
            if (input == str) {
               return cmd;
            }
         }
         return GATEPRO_CMD_NONE;
      }


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
