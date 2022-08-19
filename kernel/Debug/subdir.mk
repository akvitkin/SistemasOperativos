################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../instrucciones_handler.c \
../kernel.c \
../planificador.c \
../utils.c 

OBJS += \
./instrucciones_handler.o \
./kernel.o \
./planificador.o \
./utils.o 

C_DEPS += \
./instrucciones_handler.d \
./kernel.d \
./planificador.d \
./utils.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


