import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, cover, select
from esphome.components.template import select as template_select
from esphome.const import CONF_ID, ICON_EMPTY, UNIT_EMPTY

DEPENDENCIES = ["uart", "cover"]

gatepro_ns = cg.esphome_ns.namespace("gatepro")
GatePro = gatepro_ns.class_(
    "GatePro", cover.Cover, cg.PollingComponent, uart.UARTDevice
)

CONF_SEL_SPEED = "sel_speed"


cover.COVER_OPERATIONS.update({
    "READ_STATUS": cover.CoverOperation.COVER_OPERATION_READ_STATUS,
})
validate_cover_operation = cv.enum(cover.COVER_OPERATIONS, upper=True)

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(GatePro),
        cv.GenerateID(CONF_SEL_SPEED): template_select.CONFIG_SCHEMA,
    }).extend(cv.COMPONENT_SCHEMA).extend(cv.polling_component_schema("60s")).extend(uart.UART_DEVICE_SCHEMA)



async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    #var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
    await uart.register_uart_device(var, config)

    sel_speed = await select.new_select(config[CONF_SEL_SPEED], options=["1", "4"])
    cg.add(var.set_sel_speed(sel_speed))