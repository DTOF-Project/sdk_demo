CC := aarch64-buildroot-linux-gnu-gcc

CFLAGS := -fPIC -O2 -Wall -I./sdk
LDFLAGS := -shared
TARGET := libdtof.so

SRCS := sdk/src/calibration/dtof_bin_width_calibrate.c \
		sdk/src/calibration/dtof_calibration_cg.c \
		sdk/src/calibration/dtof_calibration_common.c \
		sdk/src/calibration/dtof_calibration_reference_spad.c \
		sdk/src/calibration/dtof_refbinoffset.c \
		sdk/src/dev/dtof_dev_api.c \
		sdk/src/dev/dtof_confidence.c \
		sdk/src/dev/dtof_hal.c \
		sdk/src/dev/dtof_swap_peak.c

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)