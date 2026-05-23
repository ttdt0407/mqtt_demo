TARGET ?= camera_monitor
BUILD_DIR	?= build
TOOLCHAIN_PATH = /home/ttdt/Projects/FTEL/av100-camera/AV100_SDK/AnyCloud39AV100_SDK_V1.11/tools/arm-anycloud-linux-uclibcgnueabi-v7.3.0/bin
CROSS_COMPILER = arm-anycloud_v7.3.0-linux-uclibcgnueabi-

CC 	= $(TOOLCHAIN_PATH)/$(CROSS_COMPILER)gcc

SRCS 		= src/main.c src/mongoose.c src/cpu_status.c src/system_log.c
OBJS		= $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS))

INC			= -Iinc -Ilib/include
CFLAGS 	= -Os -Wall -Wextra -Werror -g $(INC)
CFLAGS 	+= -DMBEDTLS_CONFIG_FILE=\"config_ftel_camera.h\"
CFLAGS 	+= -DMG_ENABLE_MQTT=1 -DMG_ENABLE_IPV6=0 -DMG_TLS=MG_TLS_MBED

MBEDTLS_LIB = 	lib/libmbedtls.a \
			  	lib/libmbedx509.a \
			  	lib/libmbedcrypto.a
LDFLAGS = -lpthread -lm

.PHONY: all clean

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@$(CC) $(OBJS) -o $(BUILD_DIR)/$(TARGET) $(MBEDTLS_LIB) $(LDFLAGS)
	@echo "LD     	 $(notdir $@)"

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "CC       $(notdir $<)"
clean:
	@if [ -d $(BUILD_DIR) ]; then \
		rm -rfv $(BUILD_DIR) | sed -n "s/^removed '.*[/]\([^']*\)'/RM       \1/p"; \
	else \
		echo "Nothing to clean."; \
	fi