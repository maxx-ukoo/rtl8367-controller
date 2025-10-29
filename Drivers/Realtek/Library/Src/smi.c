/*
 * Copyright c                  Realtek Semiconductor Corporation, 2006
 * All rights reserved.
 *
 * Program : Control smi connected RTL8366
 * Abstract :
 * Author : Yu-Mei Pan (ympan@realtek.com.cn)
 *  $Id: smi.c,v 1.2 2008-04-10 03:04:19 shiehyy Exp $
 */
#include "rtk_types.h"
#include "smi.h"
#include "rtk_error.h"
#include "main.h"
//#include "Arduino.h" /* Erase this line to use outside of Arduino env */

/*******************************************************************************/
/*  I2C porting                                                                */
/*******************************************************************************/
/* Define the GPIO ID for SCK & SDA */
//rtk_uint8 RTK_I2C_SCK_PIN = 1; /* GPIO used for SMI Clock Generation */
//rtk_uint8 RTK_I2C_SDA_PIN = 2; /* GPIO used for SMI Data signal */
//rtk_uint8 RTK_I2C_DELAY = 1;   /* Delay time for I2C */

#define CLK_DURATION delay_us(RTK_I2C_DELAY) /* Change this line to use outside of Arduino env */

extern GPIO_TypeDef* rtkI2C_SCKport;
extern GPIO_TypeDef* rtkI2C_SDAport;
extern uint32_t rtkI2C_SCKpin;
extern uint32_t rtkI2C_SDApin;

static uint32_t getCurrentDevicePin(uint16_t pin) {
	if (RTK_I2C_SCK_PIN == pin) {
		return rtkI2C_SCKpin;
	} else {
		return rtkI2C_SDApin;
	}
}

static GPIO_TypeDef* getCurrentDevicePort(uint16_t pin) {
    if (RTK_I2C_SCK_PIN == pin) {
		return rtkI2C_SCKport;
	} else {
		return rtkI2C_SDAport;
	}
}

void _pinMode(uint16_t pin, uint32_t mode)
{
	uint32_t realPin = getCurrentDevicePin(pin);
	GPIO_TypeDef* realPort = getCurrentDevicePort(pin);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Mode = mode;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = realPin;
	HAL_GPIO_Init(realPort, &GPIO_InitStruct);
}

void digitalWrite(uint16_t pin, GPIO_PinState value)
{
	uint32_t realPin = getCurrentDevicePin(pin);
	GPIO_TypeDef* realPort = getCurrentDevicePort(pin);
	HAL_GPIO_WritePin(realPort, realPin, value);
}

static uint8_t digitalRead(uint16_t pin)
{
	GPIO_PinState state;
	uint32_t realPin = getCurrentDevicePin(pin);
	GPIO_TypeDef* realPort = getCurrentDevicePort(pin);
	state = HAL_GPIO_ReadPin(realPort, realPin);
	if (state == GPIO_PIN_RESET) {
		return 0;
	} else {
		return 1;
	}
}

static void _smi_start(void)
{
    /* change GPIO pin to Output only */
	_pinMode(RTK_I2C_SCK_PIN, GPIO_MODE_OUTPUT_PP); /* Change this line to use outside of Arduino env */
	_pinMode(RTK_I2C_SDA_PIN, GPIO_MODE_OUTPUT_PP); /* Change this line to use outside of Arduino env */

    /* Initial state: SCK: 0, SDA: 1 */
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */
    digitalWrite(RTK_I2C_SDA_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;

    /* CLK 1: 0 -> 1, 1 -> 0 */
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;

    /* CLK 2: */
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SDA_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SDA_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
}

static void _smi_writeBit(rtk_uint16 signal, rtk_uint32 bitLen)
{
    /* change GPIO pin to Output only */
    _pinMode(RTK_I2C_SDA_PIN, GPIO_MODE_OUTPUT_PP); /* Change this line to use outside of Arduino env */

    for (; bitLen > 0; bitLen--)
    {
        CLK_DURATION;

        /* prepare data */
        if (signal & (1 << (bitLen - 1)))
            digitalWrite(RTK_I2C_SDA_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
        else
            digitalWrite(RTK_I2C_SDA_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */

        CLK_DURATION;

        /* clocking */
        digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
        CLK_DURATION;
        digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */
    }
}

static void _smi_readBit(rtk_uint32 bitLen, rtk_uint32 *rData)
{
    rtk_uint32 u = 0;

    /* change GPIO pin to Input only */
    _pinMode(RTK_I2C_SDA_PIN, GPIO_MODE_INPUT); /* Change this line to use outside of Arduino env */


    for (*rData = 0; bitLen > 0; bitLen--)
    {
        CLK_DURATION;

        /* clocking */
        digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
        CLK_DURATION;
        u = digitalRead(RTK_I2C_SDA_PIN); /* Change this line to use outside of Arduino env */
        digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */

        *rData |= (u << (bitLen - 1));
    }
}

static void _smi_stop(void)
{
    /* change GPIO pin to Output only */
	_pinMode(RTK_I2C_SDA_PIN, GPIO_MODE_OUTPUT_PP); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SDA_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SDA_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */

    /* add a click */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_RESET); /* Change this line to use outside of Arduino env */
    CLK_DURATION;
    digitalWrite(RTK_I2C_SCK_PIN, GPIO_PIN_SET); /* Change this line to use outside of Arduino env */

    /* change GPIO pin to Input only */
    _pinMode(RTK_I2C_SDA_PIN, GPIO_MODE_INPUT); /* Change this line to use outside of Arduino env */
    _pinMode(RTK_I2C_SCK_PIN, GPIO_MODE_INPUT); /* Change this line to use outside of Arduino env */
}

rtk_int32 smi_read(rtk_uint32 mAddrs, rtk_uint32 *rData)
{
    rtk_uint32 rawData = 0, ACK;
    rtk_uint8 con;
    rtk_uint32 ret = RT_ERR_OK;

    if (mAddrs > 0xFFFF)
        return RT_ERR_INPUT;

    if (rData == NULL)
        return RT_ERR_NULL_POINTER;

    _smi_start(); /* Start SMI */

    _smi_writeBit(0x0b, 4); /* CTRL code: 4'b1011 for RTL8370 */

    _smi_writeBit(0x4, 3); /* CTRL code: 3'b100 */

    _smi_writeBit(0x1, 1); /* 1: issue READ command */

    con = 0;
    do
    {
        con++;
        _smi_readBit(1, &ACK); /* ACK for issuing READ command*/
    } while ((ACK != 0) && (con < ack_timer));

    if (ACK != 0)
        ret = RT_ERR_FAILED;

    _smi_writeBit((mAddrs & 0xff), 8); /* Set reg_addr[7:0] */

    con = 0;
    do
    {
        con++;
        _smi_readBit(1, &ACK); /* ACK for setting reg_addr[7:0] */
    } while ((ACK != 0) && (con < ack_timer));

    if (ACK != 0)
        ret = RT_ERR_FAILED;

    _smi_writeBit((mAddrs >> 8), 8); /* Set reg_addr[15:8] */

    con = 0;
    do
    {
        con++;
        _smi_readBit(1, &ACK); /* ACK by RTL8369 */
    } while ((ACK != 0) && (con < ack_timer));
    if (ACK != 0)
        ret = RT_ERR_FAILED;

    _smi_readBit(8, &rawData); /* Read DATA [7:0] */
    *rData = rawData & 0xff;

    _smi_writeBit(0x00, 1); /* ACK by CPU */

    _smi_readBit(8, &rawData); /* Read DATA [15: 8] */

    _smi_writeBit(0x01, 1); /* ACK by CPU */
    *rData |= (rawData << 8);

    _smi_stop();

    return ret;
}

rtk_int32 smi_write(rtk_uint32 mAddrs, rtk_uint32 rData)
{
    rtk_int8 con;
    rtk_uint32 ACK;
    rtk_uint32 ret = RT_ERR_OK;

    if (mAddrs > 0xFFFF)
        return RT_ERR_INPUT;

    if (rData > 0xFFFF)
        return RT_ERR_INPUT;

    _smi_start(); /* Start SMI */

    _smi_writeBit(0x0b, 4); /* CTRL code: 4'b1011 for RTL8370*/

    _smi_writeBit(0x4, 3); /* CTRL code: 3'b100 */

    _smi_writeBit(0x0, 1); /* 0: issue WRITE command */

    con = 0;
    do
    {
        con++;
        _smi_readBit(1, &ACK); /* ACK for issuing WRITE command*/
    } while ((ACK != 0) && (con < ack_timer));
    if (ACK != 0)
        ret = RT_ERR_FAILED;

    _smi_writeBit((mAddrs & 0xff), 8); /* Set reg_addr[7:0] */

    con = 0;
    do
    {
        con++;
        _smi_readBit(1, &ACK); /* ACK for setting reg_addr[7:0] */
    } while ((ACK != 0) && (con < ack_timer));
    if (ACK != 0)
        ret = RT_ERR_FAILED;

    _smi_writeBit((mAddrs >> 8), 8); /* Set reg_addr[15:8] */

    con = 0;
    do
    {
        con++;
        _smi_readBit(1, &ACK); /* ACK for setting reg_addr[15:8] */
    } while ((ACK != 0) && (con < ack_timer));
    if (ACK != 0)
        ret = RT_ERR_FAILED;

    _smi_writeBit(rData & 0xff, 8); /* Write Data [7:0] out */

    con = 0;
    do
    {
        con++;
        _smi_readBit(1, &ACK); /* ACK for writting data [7:0] */
    } while ((ACK != 0) && (con < ack_timer));
    if (ACK != 0)
        ret = RT_ERR_FAILED;

    _smi_writeBit(rData >> 8, 8); /* Write Data [15:8] out */

    con = 0;
    do
    {
        con++;
        _smi_readBit(1, &ACK); /* ACK for writting data [15:8] */
    } while ((ACK != 0) && (con < ack_timer));
    if (ACK != 0)
        ret = RT_ERR_FAILED;

    _smi_stop();

    return ret;
}
