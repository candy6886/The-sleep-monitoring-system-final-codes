################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../main.cfg 

CMD_SRCS += \
../CC2650_LAUNCHXL.cmd 

C_SRCS += \
../CC2650_LAUNCHXL.c \
../GSENSOR_SYSTEM\ main_TI.c \
../LIS2DS12.c \
../ccfg.c 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt 

GEN_MISC_DIRS += \
./configPkg/ 

C_DEPS += \
./CC2650_LAUNCHXL.d \
./GSENSOR_SYSTEM\ main_TI.d \
./LIS2DS12.d \
./ccfg.d 

GEN_OPTS += \
./configPkg/compiler.opt 

OBJS += \
./CC2650_LAUNCHXL.obj \
./GSENSOR_SYSTEM\ main_TI.obj \
./LIS2DS12.obj \
./ccfg.obj 

GEN_MISC_DIRS__QUOTED += \
"configPkg\" 

OBJS__QUOTED += \
"CC2650_LAUNCHXL.obj" \
"GSENSOR_SYSTEM main_TI.obj" \
"LIS2DS12.obj" \
"ccfg.obj" 

C_DEPS__QUOTED += \
"CC2650_LAUNCHXL.d" \
"GSENSOR_SYSTEM main_TI.d" \
"LIS2DS12.d" \
"ccfg.d" 

GEN_FILES__QUOTED += \
"configPkg\linker.cmd" \
"configPkg\compiler.opt" 

C_SRCS__QUOTED += \
"../CC2650_LAUNCHXL.c" \
"../GSENSOR_SYSTEM main_TI.c" \
"../LIS2DS12.c" \
"../ccfg.c" 


