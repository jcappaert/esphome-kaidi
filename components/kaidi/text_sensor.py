import esphome.codegen as cg
from esphome.components import text_sensor, number, uart
import esphome.config_validation as cv
from esphome.const import ICON_RULER

CODEOWNERS = ["@jcappaert"]
DEPENDENCIES = ["uart"]
CONF_RESET = "reset"

kaidi_ns = cg.esphome_ns.namespace("kaidi")
KaidiComponent = kaidi_ns.class_(
    "KaidiComponent", text_sensor.TextSensor, cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(
        KaidiComponent,
        icon=ICON_RULER,
    )
    .extend({cv.Optional(CONF_RESET, default=False): cv.boolean})
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)
    cg.add(var.set_do_reset(config[CONF_RESET]))
    await uart.register_uart_device(var, config)
