################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/EECS3215_Project.c \
../source/PIDController.c \
../source/semihost_hardfault.c 

OBJS += \
./source/EECS3215_Project.o \
./source/PIDController.o \
./source/semihost_hardfault.o 

C_DEPS += \
./source/EECS3215_Project.d \
./source/PIDController.d \
./source/semihost_hardfault.d 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_LPC802M001JDH20 -DCPU_LPC802M001JDH20_cm0plus -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DDEBUG_SEMIHOSTING -I"/Users/mkazi/repos/MCUXpresso/EECS3215_Project/board" -I"/Users/mkazi/repos/MCUXpresso/EECS3215_Project/source" -I"/Users/mkazi/repos/MCUXpresso/EECS3215_Project" -I"/Users/mkazi/repos/MCUXpresso/EECS3215_Project/drivers" -I"/Users/mkazi/repos/MCUXpresso/EECS3215_Project/device" -I"/Users/mkazi/repos/MCUXpresso/EECS3215_Project/CMSIS" -I"/Users/mkazi/repos/MCUXpresso/EECS3215_Project/component/uart" -I"/Users/mkazi/repos/MCUXpresso/EECS3215_Project/utilities" -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


