# Cerraduras (solenoides) y microswitches de un gabinete de lockers.
# Salida por GPIO (un MOSFET por puerta, ESP32-C3 SuperMini) o por un 74HC595 (8 salidas con 3 pines).
idf_component_register(SRCS "puertas.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_gpio
                       PRIV_REQUIRES esp_timer esp_rom)
