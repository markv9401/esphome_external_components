import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, cover, button
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY

DEPENDENCIES = ["uart", "cover"]

gatepro_ns = cg.esphome_ns.namespace("gatepro")
GatePro = gatepro_ns.class_(
    "GatePro", cover.Cover, cg.PollingComponent, uart.UARTDevice
)

CONF_OPERATIONAL_SPEED = "operational_speed"

cover.COVER_OPERATIONS.update({
    "READ_STATUS": cover.CoverOperation.COVER_OPERATION_READ_STATUS,
})
validate_cover_operation = cv.enum(cover.COVER_OPERATIONS, upper=True)


CONF_SPEED_4 = "speed_4"
CONFIG_SCHEMA = cover.cover_schema(GatePro).extend(
    {
        cv.GenerateID(): cv.declare_id(GatePro),
        cv.Optional(CONF_SPEED_4): button.BUTTON_SCHEMA.extend({
            cv.GenerateId(): cv.declare_id(button.Button),
            cv.Required(CONF_NAME): cv.string,
        }),
    }).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s")).extend(uart.UART_DEVICE_SCHEMA)



async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    #var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    await uart.register_uart_device(var, config)

    if CONF_SPEED_4 in config:
        btn_config = config[CONF_SPEED_4]
        btn_set_speed_4 = await button.new_button(config[CONF_SPEED_4], var)
        cg.add(var.set_btn_set_speed_4(btn_set_speed_4))
