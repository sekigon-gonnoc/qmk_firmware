
LED_BACK_ENABLE = yes        # LED backlight (Enable WS2812 RGB underlight.)
LED_ANIMATIONS = yes        # LED animations
OLED_ENABLE = yes            # OLED_ENABLE
LOCAL_GLCDFONT = no         # use each keymaps "helixfont.h" insted of "common/glcdfont.c"


# it should exist either in <this_dir>/ld/
MCU_LDSCRIPT = nrf52840
MCU_SERIES = NRF52840
NRFSDK_ROOT := $(NRFSDK15_ROOT) #Path to nRF SDK v15.0.0

# project specific files
SRC =	alpha/matrix.c
SRC += ssd1306.c

# NRF_SEPARATE = master
#CFLAGS += -DNRF_XTAL_32MHZ

ifeq ($(strip $(LED_BACK_ENABLE)), yes)
  RGBLIGHT_ENABLE = yes
  OPT_DEFS += -DRGBLED_BACK
else
  RGBLIGHT_ENABLE = no
endif

ifeq ($(strip $(LED_ANIMATIONS)), yes)
    OPT_DEFS += -DRGBLIGHT_ANIMATIONS
endif

ifeq ($(strip $(LOCAL_GLCDFONT)), yes)
    OPT_DEFS += -DLOCAL_GLCDFONT
endif

ifeq ($(strip $(OLED_ENABLE)), yes)
    OPT_DEFS += -DOLED_ENABLE
endif

# Do not enable SLEEP_LED_ENABLE. it uses the same timer as BACKLIGHT_ENABLE
SLEEP_LED_ENABLE = no    # Breathing sleep LED during USB suspend
