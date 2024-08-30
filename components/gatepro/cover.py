import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, cover, button
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY

DEPENDENCIES = ["uart", "cover"]

gatepro_ns = cg.esphome_ns.namespace("gatepro")
GatePro = gatepro_ns.class_(
    "GatePro", cover.Cover, cg.PollingComponent, uart.UARTDevice
)

#CONF_POSITION = "position"
#CONF_ISOPEN = "isopen"

CONF_LEARN_STATUS = "learn_status"

cover.COVER_OPERATIONS.update({
    "READ_STATUS": cover.CoverOperation.COVER_OPERATION_READ_STATUS,
})
validate_cover_operation = cv.enum(cover.COVER_OPERATIONS, upper=True)

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(GatePro),
        #cv.GenerateID(CONF_ISOPEN): sensor.sensor_schema(),
        #cv.GenerateID(CONF_POSITION): sensor.sensor_schema(),
        cv.GenerateID(CONF_LEARN_STATUS): button.button_schema(GatePro),
    }).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s")).extend(uart.UART_DEVICE_SCHEMA)



async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    #var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    await uart.register_uart_device(var, config)

    btn_learn_status = await button.new_button(config[CONF_LEARN_STATUS])
    cg.add(var.set_learn_status_btn(btn_learn_status))
    
    #sensor_isopen = await sensor.new_sensor(config[CONF_ISOPEN])
    #cg.add(var.set_isopen_sensor(sensor_isopen))
    #sensor_position = await sensor.new_sensor(config[CONF_POSITION])
    #cg.add(var.set_position_sensor(sensor_position))
