################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0/protocol/thread_2.0/app/thread/plugin/version-debug/version-debug.c 

OBJS += \
./version-debug/version-debug.o 

C_DEPS += \
./version-debug/version-debug.d 


# Each subdirectory must supply rules for building sources it contributes
version-debug/version-debug.o: C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0/protocol/thread_2.0/app/thread/plugin/version-debug/version-debug.c
	@echo 'Building file: $<'
	@echo 'Invoking: IAR C/C++ Compiler for ARM'
	iccarm "$<" -o "$@" --no_wrap_diagnostics -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/stack" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/app/rf4ce/include" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/app/util" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/Device/SiliconLabs/EFR32MG1P/Include" -IC:/Users/hites/SimplicityStudio/bb_workspace/server -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/app/thread/../.." -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/app/thread/../../stack" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/app/thread/../rf4ce/include" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//protocol/thread_2.0/app/thread/../util" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base/hal/../../Device/SiliconLabs/EFR32MG1P/Include" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//hardware/kit/common/bsp" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//hardware/kit/EFR32MG1_BRD4151A/config" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base/hal/micro/cortexm3/common" -I"C:\Users\hites\SimplicityStudio\bb_workspace\server\external-generated-files" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base/hal/micro/cortexm3/efm32" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0/" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base/hal" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base/hal//plugin" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base/hal//micro/cortexm3/efm32" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base/hal//micro/cortexm3/efm32/config" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base/hal//micro/cortexm3/efm32/efr32" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../CMSIS/Include" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/common/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/config" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/dmadrv/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/emcode" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/gpiointerrupt/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/rtcdrv/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/rtcdrv/test" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/sleep/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/spidrv/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/tempdrv/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/uartdrv/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emdrv/ustimer/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../emlib/inc" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../middleware/glib" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../middleware/glib/glib" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../radio/rail_lib/chip/efr32/rf/common/cortex" -I"C:/SiliconLabs/SimplicityStudio/v4/developer/stacks/thread/v2.0.0.0//platform/base//../radio/rail_lib/chip/efr32/rf/rfprotocol/ieee802154/cortex" -e --use_c++_inline --cpu Cortex-M4 --fpu VFPv4_sp --debug --dlib_config "C:/Program Files (x86)/IAR Systems/Embedded Workbench 7.5/arm/inc/c/DLib_Config_Normal.h" --endian little --cpu_mode thumb -Ohz --no_cse --no_unroll --no_inline --no_code_motion --no_tbaa --no_clustering --no_scheduling '-DBOARD_BRD4151A=1' '-DEMBER_STACK_IP=1' '-DMFGLIB=1' '-DHAVE_TLS_JPAKE=1' '-DAPP_BTL=1' '-DBOARD_HEADER="thread-board.h"' '-DCONFIGURATION_HEADER="thread-configuration.h"' '-DPLATFORM_HEADER="platform/base/hal/micro/cortexm3/compiler/iar.h"' '-DEFR32MG1P=1' '-DEFR32MG1P232F256GM48=1' '-DEMBER_AF_API_EMBER_TYPES="stack/include/ember-types.h"' '-DMBEDTLS_CONFIG_FILE="stack/ip/tls/mbedtls/config.h"' '-DDEBUG_LEVEL=FULL_DEBUG' '-DAPPLICATION_TOKEN_HEADER="thread-token.h"' '-DCORTEXM3=1' '-DCORTEXM3_EFM32_MICRO=1' '-DCORTEXM3_EFR32_MICRO=1' '-DCORTEXM3_EFR32=1' '-DPHY_EFR32=1' --diag_suppress Pa050 --diag_suppress Pa050 --dependencies=m version-debug/version-debug.d
	@echo 'Finished building: $<'
	@echo ' '


