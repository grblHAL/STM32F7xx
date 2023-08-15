#
# Template for Web Builder, STM32F7xx
#

[platformio]
include_dir = Inc
src_dir = Src

[common]
build_flags =
  -I .
  -D L1_CACHE_ENABLE=1
  -D OVERRIDE_MY_MACHINE
  -I FatFs
  -I FatFs/STM
  -I Drivers/FATFS/Target
  -I Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
  -I Middlewares/ST/STM32_USB_Device_Library/Core/Inc
  -I USB_DEVICE/Target
  -I USB_DEVICE/App
  -D _USE_IOCTL=1
  -D _USE_WRITE=1
  -D _VOLUMES=1
# Floating point support for printf, required for WebUI v3
  -Wl,-u,_printf_float
lib_deps =
  bluetooth
  grbl
  eeprom
  fans
  keypad
  laser
  motors
  odometer
  openpnp
  sdcard
  spindle
  FatFs
  Drivers/FATFS/App
  Drivers/FATFS/Target
  USB_DEVICE/App
  USB_DEVICE/Target
  Middlewares/ST/STM32_USB_Device_Library/Class
  Middlewares/ST/STM32_USB_Device_Library/Core
lib_extra_dirs =
  .
%lib_extra_dirs%

[eth_networking]
build_flags =
  -I LWIP/App
  -I LWIP/Target
  -I Middlewares/Third_Party/LwIP/src/include
  -I Middlewares/Third_Party/LwIP/system
  -I Middlewares/Third_Party/LwIP/src/include/netif
  -I Middlewares/Third_Party/LwIP/src/include/lwip
  -I Drivers/BSP/Components/lan8742
  # Floating point support for printf, required for WebUI v3
  -Wl,-u,_printf_float
lib_deps =
   networking
   webui
   LWIP/App
   LWIP/Target
   Middlewares/Third_Party/LwIP
   Drivers/BSP/Components/lan8742
lib_extra_dirs =

[env]
platform = ststm32
framework = stm32cube
# Do not produce .a files for lib deps (which would prevent them from overriding weak symbols)
lib_archive = no
lib_ldf_mode = off

[env:%env_name%]
board = %board%
board_build.ldscript = %ldscript%
build_flags = ${common.build_flags}
%build_flags%
lib_deps = ${common.lib_deps}
%lib_deps%
lib_extra_dirs = ${common.lib_extra_dirs}
