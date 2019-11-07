
DEFAULT_FOLDER = caravelle_ble/master

NRF_DEBUG = no

# it should exist either in <this_dir>/ld/
MCU_LDSCRIPT = nrf52832
MCU_SERIES = NRF52832
NRFSDK_ROOT := $(NRFSDK12_ROOT) #Path to nRF SDK v12.3.0
PRIV_KEY := priv.pem


MOUSEKEY_ENABLE = yes	# Mouse keys
EXTRAKEY_ENABLE = yes	# Audio control and System control
CONSOLE_ENABLE = yes	# Console for debug
COMMAND_ENABLE = yes    # Commands for debug and configuration
SLEEP_LED_ENABLE = yes  # Breathing sleep LED during USB suspend
NKRO_ENABLE = no	    # USB Nkey Rollover
CUSTOM_MATRIX = yes # This flag should be on for nrf52
RGBLIGHT_ENABLE = no

## chip/board settings
MCU_FAMILY = NRF52
# linker script to use
# Cortex version
# Teensy LC is cortex-m0; Teensy 3.x are cortex-m4
MCU  = cortex-m4
# ARM version, CORTEX-M0/M1 are 6, CORTEX-M3/M4/M7 are 7
ARMV = 7
# If you want to be able to jump to bootloader from firmware on STM32 MCUs,
# set the correct BOOTLOADER_ADDRESS. Either set it here, or define it in
# ./bootloader_defs.h or in ./boards/<FOO>/bootloader_defs.h (if you have
# a custom board definition that you plan to reuse).
# If you're not setting it here, leave it commented out.
# It is chip dependent, the correct number can be looked up here (page 175):
# http://www.st.com/web/en/resource/technical/document/application_note/CD00167594.pdf
# This also requires a patch to chibios:
#   <tmk_dir>/tmk_core/tool/chibios/ch-bootloader-jump.patch
#STM32_BOOTLOADER_ADDRESS = 0x1FFFC800

# Build Options
#   comment out to disable the options.
#
