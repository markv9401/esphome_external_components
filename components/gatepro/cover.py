import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, cover, button, number
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

#CONF_SPEED_1 = "set_speed_1"

CONF_SPEED_SLIDER = "set_speed"
CONF_DECEL_DIST_SLIDER = "set_decel_dist"
CONF_DECEL_SPEED_SLIDER = "set_decel_speed"

CONFIG_SCHEMA = cover.cover_schema(GatePro).extend(
    {
        cv.GenerateID(): cv.declare_id(GatePro),
        #cv.Optional(CONF_SPEED_1): cv.use_id(button.Button),
        cv.Optional(CONF_SPEED_SLIDER): cv.use_id(number.Number),
        cv.Optional(CONF_DECEL_DIST_SLIDER): cv.use_id(number.Number),
        cv.Optional(CONF_DECEL_SPEED_SLIDER): cv.use_id(number.Number),
    }).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s")).extend(uart.UART_DEVICE_SCHEMA)



async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    #var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    await uart.register_uart_device(var, config)

    #if CONF_SPEED_1 in config:
    #    btn = await cg.get_variable(config[CONF_SPEED_1])
    #    cg.add(var.set_btn_set_speed_1(btn))
    if CONF_SPEED_SLIDER in config: 
      slider = await cg.get_variable(config[CONF_SPEED_SLIDER])
      cg.add(var.set_speed_slider(slider))
    if CONF_DECEL_DIST_SLIDER in config: 
      slider = await cg.get_variable(config[CONF_DECEL_DIST_SLIDER])
      cg.add(var.set_decel_dist_slider(slider))
    if CONF_DECEL_SPEED_SLIDER in config: 
      slider = await cg.get_variable(config[CONF_DECEL_SPEED_SLIDER])
      cg.add(var.set_decel_speed_slider(slider))

   op_speed_slider = cg.new_Pvariable("set_operational_speed")
   cg.add(op_speed_slider.set_name("Operational speed"))
   cg.add(op_speed_slider.set_min_value(1))
   cg.add(op_speed_slider.set_max_value(4))
   cg.add(op_speed_slider.set_step(1))
   cg.add(op_speed_slider.set_optimistic(True))
   cg.add(var.set_op_speed_slider(op_speed_slider))
   cg.add_var(op_speed_slider)
