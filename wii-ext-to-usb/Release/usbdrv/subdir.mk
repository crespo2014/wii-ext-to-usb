################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../usbdrv/oddebug.c \
../usbdrv/usbdrv.c 

S_UPPER_SRCS += \
../usbdrv/usbdrvasm.S 

ASM_SRCS += \
../usbdrv/usbdrvasm.asm 

OBJS += \
./usbdrv/oddebug.o \
./usbdrv/usbdrv.o \
./usbdrv/usbdrvasm.o 

C_DEPS += \
./usbdrv/oddebug.d \
./usbdrv/usbdrv.d 

S_UPPER_DEPS += \
./usbdrv/usbdrvasm.d 

ASM_DEPS += \
./usbdrv/usbdrvasm.d 


# Each subdirectory must supply rules for building sources it contributes
usbdrv/%.o: ../usbdrv/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -Os -fpack-struct -fshort-enums -mmcu=atmega168 -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

usbdrv/%.o: ../usbdrv/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Assembler'
	avr-gcc -x assembler-with-cpp -gstabs -mmcu=atmega168 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

usbdrv/%.o: ../usbdrv/%.asm
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Assembler'
	avr-gcc -x assembler-with-cpp -gstabs -mmcu=atmega168 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


