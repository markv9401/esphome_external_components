# esphome_external_components
Central repo for my ESPHome external components to be used by the ESPHome server

# the components

## GatePro gate opener
GatePro and most likely some other brands are using TMT CHOW and a wifi box with an ESP32 to control the gate operation. Flashing the said ESP32 (or replacing it) with this ESPHome firmware will allow seemless integration.
#### Example usage
```
uart:
  baud_rate: 9600
  tx_pin: GPIO17
  rx_pin: GPIO16

external_components:
  - source:
      type: git
      url: https://github.com/markv9401/esphome-external-component-uart-reader
      ref: main
    components: [ gatepro ]
    refresh: 0s

cover:
  - platform: gatepro
    name: "Driveway gate"
    device_class: gate
    update_interval: 1s
```

## Gree / Syen HVAC systems
Gree and Syen (and most likely numerous others) are using a really similar UART communication on the WiFi box. This implementation will allow great integration into HA.
#### Example usage
```
external_components:
  - source:
      type: git
      url: https://github.com/markv9401/esphome_gree_hvac
      ref: dev
    components: [ gree ]
    refresh: 0s

uart:
  id: ac_uart
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 4800
  data_bits: 8
  parity: EVEN
  stop_bits: 1

climate:
  - platform: gree
    name: GreeHVAC_Bedroom
    supported_presets:
      - "NONE"
      - "BOOST"
      - "SLEEP"
    supported_swing_modes:
      - "VERTICAL"
      - "HORIZONTAL"
      - "BOTH"
      - "OFF"
```
_based on bekmansurov/esphome_gree_hvac_
