################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include version-debug/subdir.mk
-include udp-cli/subdir.mk
-include tftp-bootload-target/subdir.mk
-include stack/thread-stack/subdir.mk
-include slaac-client/subdir.mk
-include sim-eeprom1/EFR32/subdir.mk
-include serial/EFR32/subdir.mk
-include sensor-control/subdir.mk
-include scan-debug/subdir.mk
-include network-management-cli/subdir.mk
-include main/subdir.mk
-include led/EFR32/subdir.mk
-include idle-sleep/subdir.mk
-include icmp-debug/subdir.mk
-include icmp-cli/subdir.mk
-include i2c-driver/EFR32/subdir.mk
-include heartbeat/subdir.mk
-include hal-library/EFR32/subdir.mk
-include global-address-prefix-debug/subdir.mk
-include external-generated-files/subdir.mk
-include emlib/subdir.mk
-include emdrv/subdir.mk
-include diagnostic/EFR32/subdir.mk
-include dhcp-client/subdir.mk
-include debug-print/subdir.mk
-include command-interpreter2/subdir.mk
-include coap-dispatch/subdir.mk
-include coap-cli/subdir.mk
-include cli/subdir.mk
-include buzzer/EFR32/subdir.mk
-include button/EFR32/subdir.mk
-include antenna-stub/subdir.mk
-include address-configuration-debug/subdir.mk
-include adc/EFR32/subdir.mk
-include Si7021/EFR32/subdir.mk
-include B2Bprotocol/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(MSA_UPPER_DEPS)),)
-include $(MSA_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S79_DEPS)),)
-include $(S79_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(MSA_DEPS)),)
-include $(MSA_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S79_UPPER_DEPS)),)
-include $(S79_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: pre-build main-build

# Main-build Target
main-build: client.out

# Tool invocations
client.out: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: IAR Linker for ARM'
	ilinkarm $(OBJS) $(USER_OBJS) -o "client.out" --config "C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0/platform/base/hal/micro/cortexm3/efm32/iar-cfg.icf" --config_def APP_BTL=1 --no_wrap_diagnostics "C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/build/mfglib-library-cortexm3-iar-efr32mg1p-efr32/mfglib-library.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/build/thread-stack-cortexm3-iar-efr32mg1p-efr32/thread-stack.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/build/dhcp-library-cortexm3-iar-efr32mg1p-efr32/dhcp-library.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/build/sim-eeprom1-library-cortexm3-iar-efr32mg1p-efr32/sim-eeprom1-library.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/build/mbedtls-library-cortexm3-iar-efr32mg1p-efr32/mbedtls-library.a" "C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/build/hal-library-cortexm3-iar-efr32mg1p-efr32/hal-library.a" --entry halEntryPoint --map "C:/Users/subh6068/SimplicityStudio/v4_workspace/client/IAR ARM - Default/client.map" --log_file "C:/Users/subh6068/SimplicityStudio/v4_workspace/client/IAR ARM - Default/client.log" --log initialization,modules,sections,veneers --diag_suppress=Lp012 --redirect _Printf=_PrintfSmall --redirect _Scanf=_ScanfFull  
	ielftool --srec --verbose "client.out" "client.s37"  
	ielftool --ihex --verbose "client.out" "client.hex"  
	ielftool --bin --verbose "client.out" "client.bin"
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(ASM_UPPER_DEPS)$(MSA_UPPER_DEPS)$(EXECUTABLES)$(OBJS)$(ASM_DEPS)$(S79_DEPS)$(S_DEPS)$(S_UPPER_DEPS)$(MSA_DEPS)$(C_DEPS)$(S79_UPPER_DEPS) client.out
	-@echo ' '

pre-build:
	-cmd //c C:\Users\subh6068\SimplicityStudio\v4_workspace\client/client-prebuild.bat C:\Users\subh6068\SimplicityStudio\v4_workspace\client
	-@echo ' '

post-build:
	-cmd //c C:\Users\subh6068\SimplicityStudio\v4_workspace\client/client-postbuild.bat client
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: main-build pre-build post-build

-include ../makefile.targets
