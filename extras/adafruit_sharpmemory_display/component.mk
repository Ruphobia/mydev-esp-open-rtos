# Component makefile for extras/adafruit_sharpmemory_display
#


INC_DIRS += $(ROOT)extras/adafruit_sharpmemory_display

# args for passing into compile rule generation
extras/adafruit_sharpmemory_display_INC_DIR =  $(ROOT)extras/adafruit_sharpmemory_display
extras/adafruit_sharpmemory_display_SRC_DIR =  $(ROOT)extras/adafruit_sharpmemory_display

$(eval $(call component_compile_rules,extras/adafruit_sharpmemory_display))
