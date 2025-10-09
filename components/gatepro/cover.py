import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, cover, button, number, text_sensor
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
CONF_SPEED_SLIDER = "set_speed"
CONF_DECEL_DIST_SLIDER = "set_decel_dist"
CONF_DECEL_SPEED_SLIDER = "set_decel_speed"
CONF_DEVINFO = "txt_devinfo"
CONF_LEARN_STATUS = "txt_learn_status"

CONFIG_SCHEMA = cover.cover_schema(GatePro).extend(
    {
        cv.GenerateID(): cv.declare_id(GatePro),
        cv.Optional(CONF_LEARN): cv.use_id(button.Button),
        cv.Optional(CONF_PARAMS_OD): cv.use_id(button.Button),
        cv.Optional(CONF_SPEED_SLIDER): cv.use_id(number.Number),
        cv.Optional(CONF_DECEL_DIST_SLIDER): cv.use_id(number.Number),
        cv.Optional(CONF_DECEL_SPEED_SLIDER): cv.use_id(number.Number),
        cv.Optional(CONF_DEVINFO): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_LEARN_STATUS): cv.use_id(text_sensor.TextSensor),
    }).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s")).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    #var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    await uart.register_uart_device(var, config)

    if CONF_LEARN in config:
        btn = await cg.get_variable(config[CONF_LEARN])
        cg.add(var.set_btn_learn(btn))
    if CONF_PARAMS_OD in config:
        btn = await cg.get_variable(config[CONF_PARAMS_OD])
        cg.add(var.set_btn_params_od(btn))
    if CONF_SPEED_SLIDER in config: 
      slider = await cg.get_variable(config[CONF_SPEED_SLIDER])
      cg.add(var.set_speed_slider(slider))
    if CONF_DECEL_DIST_SLIDER in config: 
      slider = await cg.get_variable(config[CONF_DECEL_DIST_SLIDER])
      cg.add(var.set_decel_dist_slider(slider))
    if CONF_DECEL_SPEED_SLIDER in config: 
      slider = await cg.get_variable(config[CONF_DECEL_SPEED_SLIDER])
      cg.add(var.set_decel_speed_slider(slider))
    if CONF_DEVINFO in config:
      txt = await cg.get_variable(config[CONF_DEVINFO])
      cg.add(var.set_txt_devinfo(txt))
    if CONF_LEARN_STATUS in config:
      txt = await cg.get_variable(config[CONF_LEARN_STATUS])
      cg.add(var.set_txt_learn_status(txt))