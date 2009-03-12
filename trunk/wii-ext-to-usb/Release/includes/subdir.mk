################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../includes/buffer.c \
../includes/rprintf.c \
../includes/timerx8.c \
../includes/uart.c 

OBJS += \
./includes/buffer.o \
./includes/rprintf.o \
./includes/timerx8.o \
./includes/uart.o 

C_DEPS += \
./includes/buffer.d \
./includes/rprintf.d \
./includes/timerx8.d \
./includes/uart.d 


# Each subdirectory must supply rules for building sources it contributes
includes/%.o: ../includes/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -Os -fpack-struct -fshort-enums -mmcu=atmega168 -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


