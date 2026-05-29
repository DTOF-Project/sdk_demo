CC := aarch64-buildroot-linux-gnu-gcc

OUTPUT_DIR := sdk/src/lib

CFLAGS := -fPIC -O2 -Wall -I./sdk
LDFLAGS := -shared

LIB_NAME := libdtof_ft.so
VERSION := 1.0.0
SONAME := $(LIB_NAME).1
TARGET := $(LIB_NAME).$(VERSION)

SRCS := sdk/src/calibration/dtof_bin_width_calibrate.c \
        sdk/src/calibration/dtof_calibration_cg.c \
        sdk/src/calibration/dtof_calibration_common.c \
        sdk/src/calibration/dtof_calibration_reference_spad.c \
        sdk/src/calibration/dtof_refbinoffset.c \
        sdk/src/dev/dtof_dev_api.c \
        sdk/src/dev/dtof_confidence.c \
        sdk/src/dev/dtof_hal.c \
        sdk/src/dev/dtof_swap_peak.c

.PHONY: all clean

all: $(OUTPUT_DIR)/$(TARGET)

$(OUTPUT_DIR)/$(TARGET): $(SRCS)
	@mkdir -p $(OUTPUT_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-soname,$(SONAME) -o $@ $^
	cd $(OUTPUT_DIR) && ln -sfn $(TARGET) $(SONAME) && ln -sfn $(SONAME) $(LIB_NAME)

clean:
	rm -f $(OUTPUT_DIR)/$(TARGET)
	rm -f $(OUTPUT_DIR)/$(SONAME)
	rm -f $(OUTPUT_DIR)/$(LIB_NAME)