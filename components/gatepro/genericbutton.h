#pragma once
#include "esphome.h"

class GenericButton : public esphome::button::Button {
 public:
  explicit GenericButton(std::function<void()> callback) {
    callback_ = callback;
  }

 protected:
  void press_action() override {
    if (callback_) callback_();
  }

 private:
  std::function<void()> callback_;
};
