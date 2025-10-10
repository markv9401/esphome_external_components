import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, cover, button, number, text_sensor, switch
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY, CONF_NAME

DEPENDENCIES = ["uart", "cover", "button"]

gatepro_ns = cg.esphome_ns.namespace("gatepro")
GatePro = gatepro_ns.class_(
    "GatePro", cover.Cover, cg.PollingComponent, uart.UARTDevice
)

CONF_OPERATIONAL_SPEED = "operational_speed"

cover.COVER_OPERATIONS.update({
    "READ_STATUS": cover.CoverOperation.COVER_OPERATION_READ_STATUS,
})
validate_cover_operation = cv.enum(cover.COVER_OPERATIONS, upper=True)


CONF_LEARN = "set_learn"
CONF_PARAMS_OD = "get_params"
CONF_REMOTE_LEARN = "remote_learn"
CONF_SPEED_SLIDER = "set_speed"
CONF_DECEL_DIST_SLIDER = "set_decel_dist"
CONF_DECEL_SPEED_SLIDER = "set_decel_speed"
CONF_MAX_AMP = "set_max_amp"
CONF_AUTO_CLOSE = "set_auto_close"
CONF_DEVINFO = "txt_devinfo"
CONF_LEARN_STATUS = "txt_learn_status"
CONF_PERMALOCK = "sw_permalock"
CONF_INFRA1 = "sw_infra1"
CONF_INFRA2 = "sw_infra2"


SET_NUMBER_SCHEMA = cv.Schema({
   cv.Required("number"): cv.use_id(number.Number),
   cv.Required("param"): cv.int_,
})

SET_SWITCH_SCHEMA = cv.Schema({
   cv.Required("switch"): cv.use_id(switch.Switch),
   cv.Required("param"): cv.int_,
})

CONFIG_SCHEMA = cover.cover_schema(GatePro).extend(
    {
        # buttons
        cv.GenerateID(): cv.declare_id(GatePro),
        cv.Optional(CONF_LEARN): cv.use_id(button.Button),
        cv.Optional(CONF_PARAMS_OD): cv.use_id(button.Button),
        cv.Optional(CONF_REMOTE_LEARN): cv.use_id(button.Button),
        # numbers
        cv.Optional(CONF_AUTO_CLOSE): SET_NUMBER_SCHEMA,
        cv.Optional(CONF_SPEED_SLIDER): SET_NUMBER_SCHEMA,
        cv.Optional(CONF_DECEL_DIST_SLIDER): SET_NUMBER_SCHEMA,
        cv.Optional(CONF_DECEL_SPEED_SLIDER): SET_NUMBER_SCHEMA,
        cv.Optional(CONF_MAX_AMP): SET_NUMBER_SCHEMA,
        # text sensors
        cv.Optional(CONF_DEVINFO): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_LEARN_STATUS): cv.use_id(text_sensor.TextSensor),
        # switches
        cv.Optional(CONF_PERMALOCK): SET_SWITCH_SCHEMA,
        cv.Optional(CONF_INFRA1): SET_SWITCH_SCHEMA,
        cv.Optional(CONF_INFRA2): SET_SWITCH_SCHEMA,
    }).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s")).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    #var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    await uart.register_uart_device(var, config)

    # buttons
    if CONF_LEARN in config:
        btn = await cg.get_variable(config[CONF_LEARN])
        cg.add(var.set_btn_learn(btn))
    if CONF_PARAMS_OD in config:
        btn = await cg.get_variable(config[CONF_PARAMS_OD])
        cg.add(var.set_btn_params_od(btn))
    if CONF_REMOTE_LEARN in config:
        btn = await cg.get_variable(config[CONF_REMOTE_LEARN])
        cg.add(var.set_btn_remote_learn(btn))
    # numbers
    if CONF_SPEED_SLIDER in config: 
      cfg = config[CONF_SPEED_SLIDER]
      slider = await cg.get_variable(cfg["number"])
      cg.add(var.set_slider(cfg["param"], slider))
    if CONF_DECEL_DIST_SLIDER in config: 
      cfg = config[CONF_DECEL_DIST_SLIDER]
      slider = await cg.get_variable(cfg["number"])
      cg.add(var.set_slider(cfg["param"], slider))
    if CONF_DECEL_SPEED_SLIDER in config: 
      cfg = config[CONF_DECEL_SPEED_SLIDER]
      slider = await cg.get_variable(cfg["number"])
      cg.add(var.set_slider(cfg["param"], slider))
    if CONF_MAX_AMP in config: 
      cfg = config[CONF_MAX_AMP]
      slider = await cg.get_variable(cfg["number"])
      cg.add(var.set_slider(cfg["param"], slider))
    if CONF_AUTO_CLOSE in config: 
      cfg = config[CONF_AUTO_CLOSE]
      slider = await cg.get_variable(cfg["number"])
      cg.add(var.set_slider(cfg["param"], slider))
    # text sensors
    if CONF_DEVINFO in config:
      txt = await cg.get_variable(config[CONF_DEVINFO])
      cg.add(var.set_txt_devinfo(txt))
    if CONF_LEARN_STATUS in config:
      txt = await cg.get_variable(config[CONF_LEARN_STATUS])
      cg.add(var.set_txt_learn_status(txt))
    # switches
    if CONF_PERMALOCK in config:
      cfg = config[CONF_PERMALOCK]
      switch = await cg.get_variable(cfg["switch"])
      cg.add(var.set_switch(cfg["param"], switch))
    if CONF_INFRA1 in config:
      cfg = config[CONF_INFRA1]
      switch = await cg.get_variable(cfg["switch"])
      cg.add(var.set_switch(cfg["param"], switch))
    if CONF_INFRA2 in config:
      cfg = config[CONF_INFRA2]
      switch = await cg.get_variable(cfg["switch"])
      cg.add(var.set_switch(cfg["param"], switch))