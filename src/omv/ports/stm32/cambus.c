/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * SCCB (I2C like) driver.
 */
#include <stdbool.h>
#include <stddef.h>
#include "py/mphal.h"

#include "omv_boardconfig.h"
#include "cambus.h"
#include "common.h"

#define I2C_TIMEOUT             (1000)
#define I2C_SCAN_TIMEOUT        (100)

// If an I2C handle is already defined in MicroPython, reuse that handle
// to allow MicroPython to process I2C IRQs, otherwise define a new handle.
#if defined(I2C1)
#if defined(MICROPY_HW_I2C1_SCL)
extern I2C_HandleTypeDef I2CHandle1;
#else
static I2C_HandleTypeDef I2CHandle1;
#endif
#endif // I2C1

#if defined(I2C2)
#if defined(MICROPY_HW_I2C2_SCL)
extern I2C_HandleTypeDef I2CHandle2;
#else
static I2C_HandleTypeDef I2CHandle2;
#endif
#endif // I2C2

#if defined(I2C3)
#if defined(MICROPY_HW_I2C3_SCL)
extern I2C_HandleTypeDef I2CHandle3;
#else
static I2C_HandleTypeDef I2CHandle3;
#endif
#endif // I2C3

#if defined(I2C4)
#if defined(MICROPY_HW_I2C4_SCL)
extern I2C_HandleTypeDef I2CHandle4;
#else
static I2C_HandleTypeDef I2CHandle4;
#endif
#endif // I2C4

int cambus_init(cambus_t *bus, uint32_t bus_id, uint32_t speed)
{
    bus->id = bus_id;
    bus->speed = speed;
    bus->i2c = NULL;

    switch (bus_id) {
        #if defined(I2C1)
        case 1: {
            bus->i2c = &I2CHandle1;
            bus->i2c->Instance = I2C1;
            break;
        }
        #endif
        #if defined(I2C2)
        case 2: {
            bus->i2c = &I2CHandle2;
            bus->i2c->Instance = I2C2;
            break;
        }
        #endif
        #if defined(I2C3)
        case 3: {
            bus->i2c = &I2CHandle3;
            bus->i2c->Instance = I2C3;
            break;
        }
        #endif
        #if defined(I2C4)
        case 4: {
            bus->i2c = &I2CHandle4;
            bus->i2c->Instance = I2C4;
            break;
        }
        #endif
        default:
            return -1;
    }

    // Our code only knows about these two I2Cs instances.
    if (bus->i2c->Instance == FIR_I2C) {
        bus->port     = FIR_I2C_PORT;
        bus->scl_pin  = FIR_I2C_SCL_PIN;
        bus->sda_pin  = FIR_I2C_SDA_PIN;
    } else if (bus->i2c->Instance == ISC_I2C) {
        bus->port     = ISC_I2C_PORT;
        bus->scl_pin  = ISC_I2C_SCL_PIN;
        bus->sda_pin  = ISC_I2C_SDA_PIN;
    } else {
        bus->port     = NULL;
        bus->scl_pin  = 0;
        bus->sda_pin  = 0;
    }

    // Configure the I2C handle
    bus->i2c->Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    #if !defined(STM32F4)
    bus->i2c->Init.Timing          = speed;
    #else
    bus->i2c->Init.ClockSpeed      = speed;
    bus->i2c->Init.DutyCycle       = I2C_DUTYCYCLE_2;
    #endif
    bus->i2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
    bus->i2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
    bus->i2c->Init.NoStretchMode   = I2C_NOSTRETCH_DISABLED;
    bus->i2c->Init.OwnAddress1     = 0xFE;
    bus->i2c->Init.OwnAddress2     = 0xFE;
    #if !defined(STM32F4)
    bus->i2c->Init.OwnAddress2Masks = 0;
    #endif

    HAL_I2C_DeInit(bus->i2c);
    if (HAL_I2C_Init(bus->i2c) != HAL_OK) {
        bus->i2c = NULL;
        bus->port = NULL;
        return -1;
    }

    if (speed == CAMBUS_SPEED_FAST) {
        // Enable FAST mode plus.
        switch (bus_id) {
            #if defined(I2C_FASTMODEPLUS_I2C1)
            case 1:
                HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C1);
                break;
            #endif
            #if defined(I2C_FASTMODEPLUS_I2C2)
            case 2:
                HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C2);
                break;
            #endif
            #if defined(I2C_FASTMODEPLUS_I2C3)
            case 3:
                HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C3);
                break;
            #endif
            #if defined(I2C_FASTMODEPLUS_I2C4)
            case 4:
                HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C4);
                break;
            #endif
        }
    }
    return 0;
}

static int cambus_set_irq_state(cambus_t *bus, bool enabled)
{
    IRQn_Type ev_irqn;
    IRQn_Type er_irqn;
    switch (bus->id) {
        #if defined(I2C1)
        case 1: {
            ev_irqn = I2C1_EV_IRQn;
            er_irqn = I2C1_ER_IRQn;
            break;
        }
        #endif
        #if defined(I2C2)
        case 2: {
            ev_irqn = I2C2_EV_IRQn;
            er_irqn = I2C2_ER_IRQn;
            break;
        }
        #endif
        #if defined(I2C3)
        case 3: {
            ev_irqn = I2C3_EV_IRQn;
            er_irqn = I2C3_ER_IRQn;
            break;
        }
        #endif
        #if defined(I2C4)
        case 4: {
            ev_irqn = I2C4_EV_IRQn;
            er_irqn = I2C4_ER_IRQn;
            break;
        }
        #endif
        default:
            return -1;
    }

    if (enabled) {
        HAL_NVIC_EnableIRQ(ev_irqn);
        HAL_NVIC_EnableIRQ(er_irqn);
    } else {
        HAL_NVIC_DisableIRQ(ev_irqn);
        HAL_NVIC_DisableIRQ(er_irqn);
    }
    return 0;
}

int cambus_deinit(cambus_t *bus)
{
    if (bus->i2c && bus->i2c->Instance) {
        HAL_I2C_DeInit(bus->i2c);
        bus->i2c->Instance = NULL;
    }
    bus->i2c = NULL;
    bus->port = NULL;
    return 0;
}

int cambus_scan(cambus_t *bus)
{
    for (uint8_t addr=0x09; addr<=0x77; addr++) {
        if (HAL_I2C_IsDeviceReady(bus->i2c, addr << 1, 10, I2C_SCAN_TIMEOUT) == HAL_OK) {
            return (addr << 1);
        }
    }

    return 0;
}

int cambus_gencall(cambus_t *bus, uint8_t cmd)
{
    if (HAL_I2C_Master_Transmit(bus->i2c, 0x00, &cmd, 1, I2C_TIMEOUT) != HAL_OK) {
        return -1;
    }
    return 0;
}

int cambus_readb(cambus_t *bus, uint8_t slv_addr, uint8_t reg_addr, uint8_t *reg_data)
{
    int ret = 0;

    if((HAL_I2C_Master_Transmit(bus->i2c, slv_addr, &reg_addr, 1, I2C_TIMEOUT) != HAL_OK)
    || (HAL_I2C_Master_Receive (bus->i2c, slv_addr, reg_data, 1, I2C_TIMEOUT) != HAL_OK)) {
        ret = -1;
    }
    return ret;
}

int cambus_writeb(cambus_t *bus, uint8_t slv_addr, uint8_t reg_addr, uint8_t reg_data)
{
    int ret=0;
    uint8_t buf[] = {reg_addr, reg_data};

    if(HAL_I2C_Master_Transmit(bus->i2c, slv_addr, buf, 2, I2C_TIMEOUT) != HAL_OK) {
        ret = -1;
    }
    return ret;
}

int cambus_readb2(cambus_t *bus, uint8_t slv_addr, uint16_t reg_addr, uint8_t *reg_data)
{
    int ret=0;
    if (HAL_I2C_Mem_Read(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_16BIT, reg_data, 1, I2C_TIMEOUT) != HAL_OK) {
        ret = -1;
    }
    return ret;
}

int cambus_writeb2(cambus_t *bus, uint8_t slv_addr, uint16_t reg_addr, uint8_t reg_data)
{
    int ret=0;
    if (HAL_I2C_Mem_Write(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_16BIT, &reg_data, 1, I2C_TIMEOUT) != HAL_OK) {
        ret = -1;
    }
    return ret;
}

int cambus_readw(cambus_t *bus, uint8_t slv_addr, uint8_t reg_addr, uint16_t *reg_data)
{
    int ret=0;
    if (HAL_I2C_Mem_Read(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_8BIT, (uint8_t*) reg_data, 2, I2C_TIMEOUT) != HAL_OK) {
        ret = -1;
    }
    *reg_data = (*reg_data >> 8) | (*reg_data << 8);
    return ret;
}

int cambus_writew(cambus_t *bus, uint8_t slv_addr, uint8_t reg_addr, uint16_t reg_data)
{
    int ret=0;
    reg_data = (reg_data >> 8) | (reg_data << 8);
    if (HAL_I2C_Mem_Write(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_8BIT, (uint8_t*) &reg_data, 2, I2C_TIMEOUT) != HAL_OK) {
        ret = -1;
    }
    return ret;
}

int cambus_readw2(cambus_t *bus, uint8_t slv_addr, uint16_t reg_addr, uint16_t *reg_data)
{
    int ret=0;
    if (HAL_I2C_Mem_Read(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_16BIT, (uint8_t*) reg_data, 2, I2C_TIMEOUT) != HAL_OK) {
        ret = -1;
    }
    *reg_data = (*reg_data >> 8) | (*reg_data << 8);
    return ret;
}

int cambus_writew2(cambus_t *bus, uint8_t slv_addr, uint16_t reg_addr, uint16_t reg_data)
{
    int ret=0;
    reg_data = (reg_data >> 8) | (reg_data << 8);
    if (HAL_I2C_Mem_Write(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_16BIT, (uint8_t*) &reg_data, 2, I2C_TIMEOUT) != HAL_OK) {
        ret = -1;
    }
    return ret;
}

int cambus_read_bytes(cambus_t *bus, uint8_t slv_addr, uint8_t reg_addr, uint8_t *buf, int len)
{
    if (HAL_I2C_Mem_Read(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT) != HAL_OK) {
        return -1;
    }
    return 0;
}

int cambus_write_bytes(cambus_t *bus, uint8_t slv_addr, uint8_t reg_addr, uint8_t *buf, int len)
{
    if (HAL_I2C_Mem_Write(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_8BIT, buf, len, I2C_TIMEOUT) != HAL_OK) {
        return -1;
    }
    return 0;
}

int cambus_readw_bytes(cambus_t *bus, uint8_t slv_addr, uint16_t reg_addr, uint8_t *buf, int len)
{
    if (HAL_I2C_Mem_Read(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_16BIT, buf, len, I2C_TIMEOUT) != HAL_OK) {
        return -1;
    }
    return 0;
}

int cambus_writew_bytes(cambus_t *bus, uint8_t slv_addr, uint16_t reg_addr, uint8_t *buf, int len)
{
    if (HAL_I2C_Mem_Write(bus->i2c, slv_addr, reg_addr,
                I2C_MEMADD_SIZE_16BIT, buf, len, I2C_TIMEOUT) != HAL_OK) {
        return -1;
    }
    return 0;
}

int cambus_read_bytes_seq(cambus_t *bus, uint8_t slv_addr, uint8_t *buf, int len, bool nostop)
{
    int ret = 0;
    cambus_set_irq_state(bus, true);

    if (HAL_I2C_Master_Seq_Receive_IT(bus->i2c, slv_addr, buf, len,
                (nostop == true) ? I2C_FIRST_FRAME : I2C_FIRST_AND_LAST_FRAME) != HAL_OK) {
        ret = -1;
        goto i2c_error;
    }

    mp_uint_t tick_start = mp_hal_ticks_ms();
    while (HAL_I2C_GetState(bus->i2c) != HAL_I2C_STATE_READY) {
        if ((mp_hal_ticks_ms() - tick_start) >= I2C_TIMEOUT) {
            ret = -1;
        }
        __WFI();
    }

i2c_error:
    cambus_set_irq_state(bus, false);
    return ret;
}

int cambus_write_bytes_seq(cambus_t *bus, uint8_t slv_addr, uint8_t *buf, int len, bool nostop)
{
    int ret = 0;
    cambus_set_irq_state(bus, true);

    if (HAL_I2C_Master_Seq_Transmit_IT(bus->i2c, slv_addr, buf, len,
                (nostop == true) ? I2C_FIRST_FRAME : I2C_FIRST_AND_LAST_FRAME) != HAL_OK) {
        ret = -1;
        goto i2c_error;
    }

    mp_uint_t tick_start = mp_hal_ticks_ms();
    while (HAL_I2C_GetState(bus->i2c) != HAL_I2C_STATE_READY) {
        if ((mp_hal_ticks_ms() - tick_start) >= I2C_TIMEOUT) {
            ret = -1;
            goto i2c_error;
        }
        __WFI();
    }

i2c_error:
    cambus_set_irq_state(bus, false);
    return ret;
}

int cambus_pulse_scl(cambus_t *bus)
{
    if (bus->port) {
        // Configure SCL as GPIO
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.Pull  = GPIO_NOPULL;
        GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStructure.Pin   = bus->scl_pin;
        HAL_GPIO_Init(bus->port, &GPIO_InitStructure);

        // Pulse SCL to recover stuck device.
        for (int i=0; i<10000; i++) {
            HAL_GPIO_WritePin(bus->port, bus->scl_pin, GPIO_PIN_SET);
            mp_hal_delay_us(10);
            HAL_GPIO_WritePin(bus->port, bus->scl_pin, GPIO_PIN_RESET);
            mp_hal_delay_us(10);
        }

        // Clear ARLO flag if it's set.
        __HAL_I2C_CLEAR_FLAG(bus->i2c, I2C_FLAG_ARLO);
        debug_printf("reset stuck i2c device\n");
    }
    return 0;
}