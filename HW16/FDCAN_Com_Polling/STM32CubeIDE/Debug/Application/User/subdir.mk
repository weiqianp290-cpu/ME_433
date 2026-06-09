################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/Users/weiqianpeng/Documents/ME433/ME_433/HW12/FDCAN_Com_Polling/Src/main.c \
/Users/weiqianpeng/Documents/ME433/ME_433/HW12/FDCAN_Com_Polling/Src/stm32c0xx_hal_msp.c \
/Users/weiqianpeng/Documents/ME433/ME_433/HW12/FDCAN_Com_Polling/Src/stm32c0xx_it.c \
../Application/User/syscalls.c \
../Application/User/sysmem.c 

OBJS += \
./Application/User/main.o \
./Application/User/stm32c0xx_hal_msp.o \
./Application/User/stm32c0xx_it.o \
./Application/User/syscalls.o \
./Application/User/sysmem.o 

C_DEPS += \
./Application/User/main.d \
./Application/User/stm32c0xx_hal_msp.d \
./Application/User/stm32c0xx_it.d \
./Application/User/syscalls.d \
./Application/User/sysmem.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/main.o: /Users/weiqianpeng/Documents/ME433/ME_433/HW12/FDCAN_Com_Polling/Src/main.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32C092xx -DUSE_NUCLEO_64 -c -I../../Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32C0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/BSP/STM32C0xx_Nucleo -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/stm32c0xx_hal_msp.o: /Users/weiqianpeng/Documents/ME433/ME_433/HW12/FDCAN_Com_Polling/Src/stm32c0xx_hal_msp.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32C092xx -DUSE_NUCLEO_64 -c -I../../Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32C0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/BSP/STM32C0xx_Nucleo -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/stm32c0xx_it.o: /Users/weiqianpeng/Documents/ME433/ME_433/HW12/FDCAN_Com_Polling/Src/stm32c0xx_it.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32C092xx -DUSE_NUCLEO_64 -c -I../../Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32C0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/BSP/STM32C0xx_Nucleo -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/%.o Application/User/%.su Application/User/%.cyclo: ../Application/User/%.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32C092xx -DUSE_NUCLEO_64 -c -I../../Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc -I../../Drivers/STM32C0xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32C0xx/Include -I../../Drivers/CMSIS/Include -I../../Drivers/BSP/STM32C0xx_Nucleo -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Application-2f-User

clean-Application-2f-User:
	-$(RM) ./Application/User/main.cyclo ./Application/User/main.d ./Application/User/main.o ./Application/User/main.su ./Application/User/stm32c0xx_hal_msp.cyclo ./Application/User/stm32c0xx_hal_msp.d ./Application/User/stm32c0xx_hal_msp.o ./Application/User/stm32c0xx_hal_msp.su ./Application/User/stm32c0xx_it.cyclo ./Application/User/stm32c0xx_it.d ./Application/User/stm32c0xx_it.o ./Application/User/stm32c0xx_it.su ./Application/User/syscalls.cyclo ./Application/User/syscalls.d ./Application/User/syscalls.o ./Application/User/syscalls.su ./Application/User/sysmem.cyclo ./Application/User/sysmem.d ./Application/User/sysmem.o ./Application/User/sysmem.su

.PHONY: clean-Application-2f-User

