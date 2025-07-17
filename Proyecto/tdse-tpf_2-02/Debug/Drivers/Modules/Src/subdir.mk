################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/Modules/Src/i2c_lcd.c \
../Drivers/Modules/Src/keypad_4x4.c 

OBJS += \
./Drivers/Modules/Src/i2c_lcd.o \
./Drivers/Modules/Src/keypad_4x4.o 

C_DEPS += \
./Drivers/Modules/Src/i2c_lcd.d \
./Drivers/Modules/Src/keypad_4x4.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Modules/Src/%.o Drivers/Modules/Src/%.su Drivers/Modules/Src/%.cyclo: ../Drivers/Modules/Src/%.c Drivers/Modules/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../app/inc -I../Drivers/Modules/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-Modules-2f-Src

clean-Drivers-2f-Modules-2f-Src:
	-$(RM) ./Drivers/Modules/Src/i2c_lcd.cyclo ./Drivers/Modules/Src/i2c_lcd.d ./Drivers/Modules/Src/i2c_lcd.o ./Drivers/Modules/Src/i2c_lcd.su ./Drivers/Modules/Src/keypad_4x4.cyclo ./Drivers/Modules/Src/keypad_4x4.d ./Drivers/Modules/Src/keypad_4x4.o ./Drivers/Modules/Src/keypad_4x4.su

.PHONY: clean-Drivers-2f-Modules-2f-Src

