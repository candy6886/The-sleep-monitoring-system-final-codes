/*
 *  ======== empty.c ========
 */
#include <Board.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>

/* TI-RTOS Header files */
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>//modify
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
// #include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>

/* Board Header files */
#include "LIS2DS12.h"

#define TASKSTACKSIZE   1024
#define LIS2DS12_OBJ_ACC    0x0003  /* Object ACClerometer Result Register *///**********************

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

/* Pin driver handles */
static PIN_Handle ledPinHandle;

/* Global memory storage for a PIN_Config table */
static PIN_State ledPinState;
/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config ledPinTable[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

/* This macro can be used to switch on/off the evaluation with interrupts (for g sensor)*/
#define FIFO_SIZE   256
Type3Axis16bit_U Acceleration;
int Acceleration_G[FIFO_SIZE][3];
status_t response;

/*define the I2C ============================================================================*/
    uint8_t         txBuffer[32];//transmitter buffer
    uint8_t         rxBuffer[32];//receiver buffer
    I2C_Handle      handle;
    I2C_Params      params;
    I2C_Transaction i2cTrans;

static Bool SensorRWReg(uint8_t addr, uint8_t *pWriteBuf, uint8_t WriteBytes, uint8_t *pReadBuf, uint8_t ReadBytes)
{
    txBuffer[0] = addr;//address is register ID
    memcpy(txBuffer+1, pWriteBuf, WriteBytes);
    // Initialize master I2C transaction structure
    i2cTrans.writeCount   = 1+WriteBytes;//number of bytes to be written to the slave
    i2cTrans.writeBuf     = txBuffer;//buffer containing data to be written
    i2cTrans.readCount    = ReadBytes;//number of bytes to be read from the slave
    i2cTrans.readBuf      = pReadBuf;//buffer to which data is to read into
    i2cTrans.slaveAddress = LIS2DS12_I2C_ADDRESS;
    return I2C_transfer(handle, &i2cTrans);
}

/*define I2C reading functions & LIS2DS12 writing function============================================*/
u8_t I2C_ByteWrite(u8_t dev_addr, u8_t reg_addr, u8_t i2c_len, u8_t *i2c_data_buf)
{
    Bool ret = FALSE;
    ret = SensorRWReg(reg_addr, i2c_data_buf, i2c_len, NULL, 0);
    return ret;
}
u8_t I2C_ByteRead(u8_t dev_addr, u8_t reg_addr, u8_t i2c_len, u8_t *i2c_data_buf)
{
    Bool ret = SensorRWReg(reg_addr, NULL, 0, i2c_data_buf, i2c_len);
    return ret;
}

static u8_t gsensorI2C_Init()
{
         /* Create I2C for usage *****************/
      I2C_Params_init(&params);
      params.bitRate = I2C_400kHz;

       memset(txBuffer, 0, sizeof(txBuffer));
       memset(rxBuffer, 0, sizeof(rxBuffer));// just to make the txBuff and rxBuff

       handle = I2C_open(Board_I2C, &params);

       if (handle == NULL) {
           System_abort("Error Initializing I2C\n");
       }
       else {
            System_printf("I2C Initialized!\n");
       }

       /* Complete I2C creating for usage**********/
            Task_sleep(1000000 / Clock_tickPeriod);

        return SensorRWReg(LIS2DS12_WHO_AM_I_REG,NULL,0, rxBuffer, 1);
}



/*=========================================================================================================================
 * Callback to handle the 6D event.
 * It must be registered to be called at interrupt time.
 */
#define FIFO_SIZE   256
/* Private variables ----------------------------*/
Type3Axis16bit_U Acceleration;
LIS2DS12_DRDY_t     value_XL;
int Acceleration_G[FIFO_SIZE][3];

int i;
u8_t position;
u8_t old_position;
u8_t sleepstatus;

/*
 *  ======== gSensorFxn ================================================================================================
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the gSensor Task instance.
 */
//============================================================
void gSensorFxn(UArg arg0, UArg arg1)
{
      gsensorI2C_Init();
      /*Begin to write & read values**********************=============*/
      if (SensorRWReg(LIS2DS12_WHO_AM_I_REG,NULL,0, rxBuffer, 1))
           //important, when reading value must add it ahead.
      {
              System_printf("WHO_AM_I_REG 0x%02x\n", rxBuffer[0]);
              System_flush();

              /* Set ACC full scale @ 2g**************/
                                 LIS2DS12_W_FullScale(LIS2DS12_FS_2G);
                                 //if(response==MEMS_ERROR) while(1); //manage here communication error
                                 // BDU Enable
                                 LIS2DS12_W_BDU(LIS2DS12_BDU_ON);
                                 // Set ACC ODR  HR_14bit 200Hz
                                 LIS2DS12_W_ODR(LIS2DS12_ODR_HR_14bit_100Hz);
                                 //==================================================
                                 //definition for 6D Event
                                 /* Set interrupt latched mode */
                                 LIS2DS12_W_LIR(LIS2DS12_LIR_ON);

                                /*change from low to high when interrupt condition verified.*/
                                LIS2DS12_W_H_LACTIVE(LIS2DS12_H_LACTIVE_high);//default

                                /* Set 6D threshold  */
                                LIS2DS12_W_6D_THS(LIS2DS12_6D_THS_60_degrees);

                                /* 6D on INT1 */
                                LIS2DS12_W_INT1_6D(LIS2DS12_INT1_6D_ON);
                                 //===================================================
                                 //definition for wakeup-sleep even(inactive and active)
                                 /* Set sleep_on  */
                                 LIS2DS12_W_SLEEP_ON(LIS2DS12_SLEEP_ON_ON);

                                 /* Set Wakeup threshold  */
                                 LIS2DS12_W_WU_THS(2);//threshold is 2*FS/64=62.5mg(64=2^6,6-bit unsigned 1LSB)

                                 /* Set sleep duration  */
                                 LIS2DS12_W_SleepDuration(2);//1 LSB=1TODR

                                 LIS2DS12_W_WakeUpDuration(2);

                                 /* Wakeup on INT1 */
                                LIS2DS12_W_INT1_WU(LIS2DS12_INT1_WU_ON);

                                 /* Set high pass filter */
                                 LIS2DS12_W_FDS_SLOPE(LIS2DS12_FDS_SLOPE_OFF);

               while(1)
               {

                   /* Event will be handled in driver callback */
                                         //get 6D Position================================================
                                         LIS2DS12_6D_IA_DUP2_t positionStatus;
                                         LIS2DS12_R_6D_IA_DUP2(&positionStatus);
                                         if (positionStatus == LIS2DS12_6D_IA_DUP2_EV_ON) {
                                             response=LIS2DS12_Get_6D_Event(&position);
                                             if(response==1 && (old_position!=position)){//&& (old_position!=position)
                                                   switch(position){
                                                   case LIS2DS12_YL_EV_ON: System_printf("position = UP_LS O(^_^)O %d\n\r\0",position); break;
                                                   case LIS2DS12_XL_EV_ON: System_printf("position = UP_RS O(^_^)O %d\n\r\0",position); break;
                                                   case LIS2DS12_XH_EV_ON: System_printf("position = DW_LS O(^_^)O %d\n\r\0",position); break;
                                                   case LIS2DS12_YH_EV_ON: System_printf("position = DW_RS  O(^_^)O %d\n\r\0",position); break;
                                                   case LIS2DS12_ZH_EV_ON: System_printf("position = UPFACE O(^_^)O %d\n\r\0",position); break;
                                                   case LIS2DS12_ZL_EV_ON: System_printf("position = DOWNFACE O(^_^)O %d\n\r\0",position); break;
                                                   default: System_printf("position = unknown O(-_-)O%d\n\r\0",position); break;
                                                   }
                                               System_flush();
                                               old_position = position;
                                             }//IF 6D_Event
                                             /* Clear 6D_IA event*/
                                             position = LIS2DS12_6D_IA_EV_DUP2_OFF;
                                          }//If LIS2DS12_6D_IA_EV_DUP2

                                          //WAKUP EVENT================================================
                                          LIS2DS12_WU_IA_DUP2_t WakeupStatus;
                                          /* handle event *//* Clear WU_IA event*/
                                          LIS2DS12_R_WU_IA_DUP2(&WakeupStatus);
                                          if (WakeupStatus == LIS2DS12_WU_IA_DUP2_EV_ON)
                                          {
                                                System_printf("WAKE_UP ALREADY !!\n");
                                                System_flush();
                                                /* Clear WU_IA event*/
                                                WakeupStatus = LIS2DS12_WU_IA_DUP2_EV_OFF;
                                          }//if LIS2DS12_WU_IA_DUP2_EV_ON

                                          /*
                                          * Read ACC output only if new ACC value is available=================*/
                                          LIS2DS12_R_DRDY(&value_XL);
                                          if (LIS2DS12_DRDY_EV_ON == value_XL)
                                          {
                                                LIS2DS12_Get_Acceleration(Acceleration_G[0]);
                                                System_printf("Get Acceleration: %d %d %d\n", Acceleration_G[0][0],
                                                              Acceleration_G[0][1], Acceleration_G[0][2]);//PIN_getInputValue(PIN_ID(21)
                                                System_flush();
                                          }//if DRDY*/
               }//while===============================

      }//I2C communication
      else {
          System_printf("I2C Bus fault\n");
      }

      /* initialized I2C */
      I2C_close(handle);
      System_printf("I2C closed!\n");
      System_flush();
}//g_sensor fxn


/*
 *  ======== main =================================================================================================
 */
int main(void)
{
    Task_Params taskParams;

    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initI2C();
    // Board_initSPI();
    // Board_initUART();
    // Board_initWatchdog();

    /* Call board init functions */
    //PIN_init(BoardGpioInitTable);
    /* declare the disallow standby constraint while GP timer is in use!!!!!(not sure here) */

    /* Construct g_sensor Task  thread */
    Task_Params_init(&taskParams);
    //taskParams.arg0 = 1000000 / Clock_tickPeriod;**************
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)gSensorFxn, &taskParams, NULL);

    /* Open LED pins*/
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if(!ledPinHandle) {
        System_abort("Error initializing board LED pins\n");
    }
    PIN_setOutputValue(ledPinHandle, Board_LED1, 1);

    System_printf("Starting the I2C example\nSystem provider is set to SysMin. "
                  "Halt the target to view any SysMin contents in ROV.\n");
    /* SysMin will only print to the console when you call flush or exit */


    /* Start BIOS */
    BIOS_start();

    return (0);
}
