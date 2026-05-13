################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Application/Startup/startup_stm32c092rctx.s 

OBJS += \
./Application/Startup/startup_stm32c092rctx.o 

S_DEPS += \
./Application/Startup/startup_stm32c092rctx.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Startup/%.o: ../Application/Startup/%.s Application/Startup/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m0plus -g3 -DDEBUG -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@" "$<"

clean: clean-Application-2f-Startup

clean-Application-2f-Startup:
	-$(RM) ./Application/Startup/startup_stm32c092rctx.d ./Application/Startup/startup_stm32c092rctx.o

.PHONY: clean-Application-2f-Startup

