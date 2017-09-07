/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*********************************************************************
 * INCLUDES
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

/* Board Header files */
#include "LIS2DS12.h"
//#define xdc_runtime_Log_DISABLE_ALL 1  // Add to disable logs from this file
#include <xdc/runtime/Log.h>
// BLuetooth Developer Studio

#include "GSensor_Service.h"

#include "project_zero.h"
#include "peripheral.h"
#include "emptyTask.h"
#include "custom_fmt.h"

/* This macro can be used to switch on/off the evaluation with interrupts (for g sensor)*/
#define FIFO_SIZE   256
Type3Axis16bit_U Acceleration;
int Acceleration_G[FIFO_SIZE][3];
status_t response;
//for g sensor=======

//extern Semaphore_Handle gSensorSem;
//extern void user_GsensorChangeCB(app_msg_types_t paramID);
//extern void user_Gsensor_CharValueChangeEvt(uint8_t paramID);

/*define the I2C ============================================================================*/
uint8_t         txBuffer[32];//transmitter buffer
uint8_t         rxBuffer[32];//receiver buffer
I2C_Handle      handle;
I2C_Params      params;
I2C_Transaction i2cTrans;
Semaphore_Handle gSensorSem;

static Bool HalSensorRWReg(uint8_t addr, uint8_t *pWriteBuf, uint8_t WriteBytes, uint8_t *pReadBuf, uint8_t ReadBytes)
{
    txBuffer[0] = addr;//address is register ID
    memcpy(txBuffer+1, pWriteBuf, WriteBytes);

    // Initialize master I2C transaction structure
    //txBuffer[0] = LIS2DS12_OBJ_ACC;
    i2cTrans.writeCount   = 1+WriteBytes;
    i2cTrans.writeBuf     = txBuffer;
    i2cTrans.readCount    = ReadBytes;
    i2cTrans.readBuf      = pReadBuf;
    i2cTrans.slaveAddress = LIS2DS12_I2C_ADDRESS;

    //transferDone = false;

    return I2C_transfer(handle, &i2cTrans);
}

static u8_t gsensorI2C_Init()
{
    // Do I2C transfer (in callback mode)
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

    return HalSensorRWReg(LIS2DS12_WHO_AM_I_REG,NULL,0, rxBuffer, 1);
}


/*define I2C reading functions & LIS2DS12 writing function============================================*/
int I2C_ByteWrite(u8_t dev_addr, u8_t reg_addr, u8_t i2c_len, u8_t *i2c_data_buf)
{
    Bool ret = FALSE;
    //memcpy(txBuffer, i2c_data_buf, i2c_len);
    ret = HalSensorRWReg(reg_addr, i2c_data_buf, i2c_len, NULL, 0);
    return ret;
}
int I2C_ByteRead(u8_t dev_addr, u8_t reg_addr, u8_t i2c_len, u8_t *i2c_data_buf)
{
    Bool ret = HalSensorRWReg(reg_addr, NULL, 0, i2c_data_buf, i2c_len);
    //memcpy(i2c_data_buf, rxBuffer, i2c_len);
    return ret;
}


/*=========================================================================================================================
 * Callback to handle the 6D event.
 * It must be registered to be called at interrupt time.
 */
//static u32_t LIS2DS12_sample_calls = 0;
u8_t position;
u8_t old_position;

void LIS2DS12_Events(void)
{
    char pLine[20];

    //uint16 notify_Handle;
    //GAPRole_GetParameter( GAPROLE_CONNHANDLE, &notify_Handle);        //Get Connection Handle
    /* Event will be handled in driver callback */
    //get 6D Position================================================
    LIS2DS12_6D_IA_DUP2_t positionStatus;
    LIS2DS12_R_6D_IA_DUP2(&positionStatus);
    if(GSENSOR_SERVICE_SERV_UUID){
        if (positionStatus == LIS2DS12_6D_IA_DUP2_EV_ON) {
            response = LIS2DS12_Get_6D_Event(&position);
            if (response == 1& (old_position!=position)) {    //&& (old_position!=position)
                  switch (position) {
                     case LIS2DS12_YL_EV_ON:
                        itoaAppendStr(pLine, position, "UP LS");
                        break;
                    case LIS2DS12_XL_EV_ON:
                        itoaAppendStr(pLine, position, "UP RS");
                        break;
                    case LIS2DS12_XH_EV_ON:
                        itoaAppendStr(pLine, position, "DW LS");
                        break;
                    case LIS2DS12_YH_EV_ON:
                        itoaAppendStr(pLine, position, "DW RS");
                        break;
                    case LIS2DS12_ZH_EV_ON:
                        itoaAppendStr(pLine, position, "UPFACE");
                        break;
                    case LIS2DS12_ZL_EV_ON:
                        itoaAppendStr(pLine, position, "DOWNFACE");
                        break;
                    default:
                        itoaAppendStr(pLine, position, "unknown");
                        break;
                     /*user_enqueueCharDataMsg(APP_MSG_UPDATE_CHARVAL, 0,
                                                                     GSENSOR_SERVICE_SERV_UUID, GS_POSITION_ID,
                                                                    (uint8_t *)pLine, strlen(pLine));*/
                    }
                  user_enqueueCharDataMsg(APP_MSG_UPDATE_CHARVAL, 0,
                                                                   GSENSOR_SERVICE_SERV_UUID, GS_POSITION_ID,
                                                                    (uint8_t *)pLine, strlen(pLine));

                    LIS2DS12_Get_Acceleration(Acceleration_G[0]);

                    itoaAppendStr(pLine, Acceleration_G[0][0], "X mg");
                    user_enqueueCharDataMsg(APP_MSG_UPDATE_CHARVAL, 0,
                                                                GSENSOR_SERVICE_SERV_UUID, GS_ORIENTION_AXIS_ID,
                                                                (uint8_t *)pLine, strlen(pLine));
                   itoaAppendStr(pLine, Acceleration_G[0][1], "Y mg");
                   user_enqueueCharDataMsg(APP_MSG_UPDATE_CHARVAL, 0,
                                                               GSENSOR_SERVICE_SERV_UUID, GS_ORIENTION_AXIS_ID,
                                                               (uint8_t *)pLine, strlen(pLine));
                   itoaAppendStr(pLine, Acceleration_G[0][2], "Z mg");
                    //sprintf(pLine,"%d%d %s ",Acceleration_G[0][1],  "Y mg", Acceleration_G[0][2], "Z mg" );
                    user_enqueueCharDataMsg(APP_MSG_UPDATE_CHARVAL, 0,
                                            GSENSOR_SERVICE_SERV_UUID, GS_ORIENTION_AXIS_ID,
                                            (uint8_t *)pLine, strlen(pLine));

                old_position = position;
            }//IF 6D_Event position changed
            /* handle event*/
            position = LIS2DS12_6D_IA_EV_DUP2_OFF;
        }  //If LIS2DS12_6D_IA_EV_DUP2

        /*
         * ***********************************************WAKUP EVENT*************************************************************
         **/

        LIS2DS12_WU_IA_DUP2_t WakeupStatus;
        /* Clear WU_IA event*/
        LIS2DS12_R_WU_IA_DUP2(&WakeupStatus);
        if (WakeupStatus == LIS2DS12_WU_IA_DUP2_EV_ON) {
                itoaAppendStr(pLine,NULL, "WAKE_UP ALREADY !");
                user_enqueueCharDataMsg(APP_MSG_UPDATE_CHARVAL, 0,
                                        GSENSOR_SERVICE_SERV_UUID, GS_WAKE_UP_STATE_ID,
                                        (uint8_t *)pLine, strlen(pLine));
            /* handle event */
            WakeupStatus = LIS2DS12_WU_IA_DUP2_EV_OFF;
        }    //if LIS2DS12_WU_IA_DUP2_EV_ON

    }//for GSENSOR_SERVICE uuid
}

/*
 *  ======== gSensorFxn ================================================================================================
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the gSensor Task instance.
 */
//============================================================
extern void  gSensorFxn(void)//changed
{
    gsensorI2C_Init();
    /*Begin to write & read values**********************=============*/
    if (HalSensorRWReg(LIS2DS12_WHO_AM_I_REG,NULL,0, rxBuffer, 1))
        //important, when reading value must add it ahead.
    {
        System_printf("WHO_AM_I_REG 0x%02x\n", rxBuffer[0]);
        System_flush();

        /* Set ACC full scale @ 2g**************/
        LIS2DS12_W_FullScale(LIS2DS12_FS_2G);
        //if(response==MEMS_ERROR) while(1); //manage here communication error
        // BDU Enable
        LIS2DS12_W_BDU(LIS2DS12_BDU_ON);
        // Set ACC ODR  HR_14bit 25Hz
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

        //PIN_setInterrupt(pinHandle, Board_INT1 | PIN_IRQ_NEGEDGE);[*can put here or in main function]

        /*originally, here are initial g_sensor functions,==================
              but now they were moved to callback fxn=============================*/


        //MCU keep standby status normally until an interrupt come*******
        //Power_sleep(PowerCC26XX_STANDBY);
        //for (;;);
        /* while(1)//not sure the while loop need to add or not!@@@@@@try!!!!!!
                {
                    wait for semaphore to be posted by pinInterruptHandler
                    Semaphore_pend(sem, BIOS_WAIT_FOREVER);
                    //MCU wake up when the interrupt arrive*******
                    //Power_sleep(PowerCC26XX_AWAKE_STANDBY);
                     Do work */
                    LIS2DS12_Events();
                //}//while===============================

    }//I2C communication
    else {
        System_printf("I2C Bus fault\n");
    }


    /* initialized I2C */
    I2C_close(handle);
    System_printf("I2C closed!\n");
    System_flush();
}//g_sensor fxn

//=================================================================================================================
void pinInterruptHandler(UArg a0)//hardware call back
{
    Log_info0("Sensor interrupt.");
    user_enqueueRawAppMsg(APP_MSG_TASK_ALERT, NULL, 0);
}
