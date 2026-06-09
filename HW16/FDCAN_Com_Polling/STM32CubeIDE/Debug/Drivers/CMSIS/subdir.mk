################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/Users/weiqianpeng/Documents/ME433/ME_433/HW12/FDCAN_Com_Polling/Src/system_stm32c0xx.c 

OBJS += \
./Drivers/CMSIS/system_stm32c0xx.o 

C_DEPS += \
./Drivers/CMSIS/system_stm32c0xx.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/CMSIS/system_stm32c0xx.o: /Users/weiqianpeng/Documents/ME433/ME_433/HW12/FDCAN_Com_Polling/Src/system_stm32c0xx.c Drivers/CMSIS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32C092xx -DUSE_NUCLEO_64 -c -I../../Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32C0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/BSP/STM32C0xx_Nucleo -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-CMSIS

clean-Drivers-2f-CMSIS:
	-$(RM) ./Drivers/CMSIS/system_stm32c0xx.cyclo ./Drivers/CMSIS/system_stm32c0xx.d ./Drivers/CMSIS/system_stm32c0xx.o ./Drivers/CMSIS/system_stm32c0xx.su

.PHONY: clean-Drivers-2f-CMSIS

