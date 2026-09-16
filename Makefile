PROJECT := stm32
APP     ?= b1

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

BUILD_DIR := Build
BIN_DIR   := Bin
LINKER    := Linker/STM32F103C8.ld

C_SOURCES := Src/$(APP).c Src/system_stm32f1xx.c
STARTUP   := Lib/STM32CubeF1/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s
OBJECTS   := $(patsubst Src/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES)) \
             $(BUILD_DIR)/startup_stm32f103xb.o
DEPS      := $(patsubst Src/%.c,$(BUILD_DIR)/%.d,$(C_SOURCES))

CPUFLAGS := -mcpu=cortex-m3 -mthumb
CMSIS_DIR := Lib/STM32CubeF1/Drivers/CMSIS
INCLUDES  := -I$(CMSIS_DIR)/Core/Include \
             -I$(CMSIS_DIR)/Device/ST/STM32F1xx/Include
CFLAGS   := $(CPUFLAGS) -DSTM32F103xB $(INCLUDES) \
            -std=gnu11 -O2 -g3 -Wall -Wextra -ffreestanding \
            -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS  := $(CPUFLAGS) -T$(LINKER) -nostdlib \
            -Wl,--gc-sections -Wl,-Map=$(BIN_DIR)/$(PROJECT).map

ELF := $(BIN_DIR)/$(PROJECT).elf
BIN := $(BIN_DIR)/$(PROJECT).bin
HEX := $(BIN_DIR)/$(PROJECT).hex

.PHONY: all flash flash-stlink clean

all: $(ELF) $(BIN) $(HEX)
	$(SIZE) $(ELF)

$(ELF): $(OBJECTS) $(LINKER) | $(BIN_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/%.o: Src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup_stm32f103xb.o: $(STARTUP) | $(BUILD_DIR)
	$(CC) $(CPUFLAGS) -c $< -o $@

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# Flash with OpenOCD and an ST-Link probe.
flash: $(ELF)
	openocd -f interface/stlink.cfg \
		-c "set CPUTAPID 0x2ba01477" \
		-f target/stm32f1x.cfg \
		-c "program $(ELF) verify reset exit"
# Alternative for systems that have the stlink-tools package installed.
flash-stlink: $(BIN)
	st-flash --reset write $(BIN) 0x08000000

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d
	rm -f $(BIN_DIR)/$(PROJECT).elf $(BIN_DIR)/$(PROJECT).bin \
	      $(BIN_DIR)/$(PROJECT).hex $(BIN_DIR)/$(PROJECT).map

-include $(DEPS)
