# Component makefile for extras/flirlipton
#


INC_DIRS += $(ROOT)extras/flirlipton

# args for passing into compile rule generation
extras/flirlipton_INC_DIR =  $(ROOT)extras/flirlipton
extras/flirlipton_SRC_DIR =  $(ROOT)extras/flirlipton

$(eval $(call component_compile_rules,extras/flirlipton))
