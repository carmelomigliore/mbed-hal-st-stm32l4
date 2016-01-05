/* mbed Microcontroller Library
 *******************************************************************************
 * Copyright (c) 2015, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */
#include "mbed_assert.h"
#include "spi_api.h"

#if DEVICE_SPI

#include <math.h>
#include "cmsis.h"
#include "pinmap.h"
#include "mbed_error.h"
#include "PeripheralPins.h"

static SPI_HandleTypeDef SpiHandle;

static void init_spi(spi_t *obj)
{
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);

    __HAL_SPI_DISABLE(&SpiHandle);

    SpiHandle.Init.Mode              = SPI_MODE_MASTER;
    SpiHandle.Init.BaudRatePrescaler = obj->br_presc;
    SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
    SpiHandle.Init.CLKPhase          = obj->cpha;
    SpiHandle.Init.CLKPolarity       = obj->cpol;
    SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    SpiHandle.Init.CRCPolynomial     = 7;
    SpiHandle.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;
    SpiHandle.Init.DataSize          = obj->bits;
    SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
    SpiHandle.Init.NSS               = SPI_NSS_SOFT;
    SpiHandle.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;

    if (HAL_SPI_Init(&SpiHandle) != HAL_OK) {
        error("Cannot initialize SPI\n");
    }

    __HAL_SPI_ENABLE(&SpiHandle);
}

void spi_init(spi_t *obj, PinName mosi, PinName miso, PinName sclk)
{
    // Determine the SPI to use
    SPIName spi_mosi = (SPIName)pinmap_peripheral(mosi, PinMap_SPI_MOSI);
    SPIName spi_miso = (SPIName)pinmap_peripheral(miso, PinMap_SPI_MISO);
    SPIName spi_sclk = (SPIName)pinmap_peripheral(sclk, PinMap_SPI_SCLK);

    SPIName spi_data = (SPIName)pinmap_merge(spi_mosi, spi_miso);

    obj->spi = (SPIName)pinmap_merge(spi_data, spi_sclk);
    MBED_ASSERT(obj->spi != (SPIName)NC);

    // Enable SPI clock
    if (obj->spi == SPI_1) {
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
    if (obj->spi == SPI_2) {
        __HAL_RCC_SPI2_CLK_ENABLE();
    }
    if (obj->spi == SPI_3) {
        __HAL_RCC_SPI3_CLK_ENABLE();
    }

    // Configure the SPI pins
    pinmap_pinout(mosi, PinMap_SPI_MOSI);
    pinmap_pinout(miso, PinMap_SPI_MISO);
    pinmap_pinout(sclk, PinMap_SPI_SCLK);

    // Save new values
    obj->bits = SPI_DATASIZE_8BIT;
    obj->cpol = SPI_POLARITY_LOW;
    obj->cpha = SPI_PHASE_1EDGE;
    obj->br_presc = SPI_BAUDRATEPRESCALER_256;

    obj->pin_miso = miso;
    obj->pin_mosi = mosi;
    obj->pin_sclk = sclk;

    init_spi(obj);
}

void spi_free(spi_t *obj)
{
    // Reset SPI and disable clock
    if (obj->spi == SPI_1) {
        __HAL_RCC_SPI1_FORCE_RESET();
        __HAL_RCC_SPI1_RELEASE_RESET();
        __HAL_RCC_SPI1_CLK_DISABLE();
    }

    if (obj->spi == SPI_2) {
        __HAL_RCC_SPI2_FORCE_RESET();
        __HAL_RCC_SPI2_RELEASE_RESET();
        __HAL_RCC_SPI2_CLK_DISABLE();
    }

    if (obj->spi == SPI_3) {
        __HAL_RCC_SPI3_FORCE_RESET();
        __HAL_RCC_SPI3_RELEASE_RESET();
        __HAL_RCC_SPI3_CLK_DISABLE();
    }

    // Configure GPIO
    pin_function(obj->pin_miso, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, 0));
    pin_function(obj->pin_mosi, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, 0));
    pin_function(obj->pin_sclk, STM_PIN_DATA(STM_MODE_INPUT, GPIO_NOPULL, 0));
}

void spi_format(spi_t *obj, int bits, int mode, spi_bitorder_t order)
{
    // Save new values
    if (bits == 16) {
        obj->bits = SPI_DATASIZE_16BIT;
    } else {
        obj->bits = SPI_DATASIZE_8BIT;
    }

    switch (mode) {
        case 0:
            obj->cpol = SPI_POLARITY_LOW;
            obj->cpha = SPI_PHASE_1EDGE;
            break;
        case 1:
            obj->cpol = SPI_POLARITY_LOW;
            obj->cpha = SPI_PHASE_2EDGE;
            break;
        case 2:
            obj->cpol = SPI_POLARITY_HIGH;
            obj->cpha = SPI_PHASE_1EDGE;
            break;
        default:
            obj->cpol = SPI_POLARITY_HIGH;
            obj->cpha = SPI_PHASE_2EDGE;
            break;
    }

    if (order == SPI_MSB) {
        obj->order = SPI_FIRSTBIT_MSB;
    } else {
        obj->order = SPI_FIRSTBIT_LSB;
    }

    init_spi(obj);
}

void spi_frequency(spi_t *obj, int hz)
{
    // Values depend of PCLK1 and PCLK2: 80 MHz if MSI or HSI is used, 48 MHz if HSE is used
    if (SystemCoreClock == 80000000) { // MSI or HSI
        if (hz < 600000) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_256; // 313 kHz
        } else if ((hz >= 600000) && (hz < 1000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_128; // 625 kHz
        } else if ((hz >= 1000000) && (hz < 2000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_64; // 1.25 MHz (default)
        } else if ((hz >= 2000000) && (hz < 5000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_32; // 2.5 MHz
        } else if ((hz >= 5000000) && (hz < 10000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_16; // 5 MHz
        } else if ((hz >= 10000000) && (hz < 20000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_8; // 10 MHz
        } else if ((hz >= 20000000) && (hz < 40000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_4; // 20 MHz
        } else { // >= 40000000
            obj->br_presc = SPI_BAUDRATEPRESCALER_2; // 40 MHz
        }
    } else { // 48 MHz - HSE
        if (hz < 350000) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_256; // 188 kHz
        } else if ((hz >= 350000) && (hz < 750000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_128; // 375 kHz
        } else if ((hz >= 750000) && (hz < 1000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_64; // 750 kHz
        } else if ((hz >= 1000000) && (hz < 3000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_32; // 1.5 MHz (default)
        } else if ((hz >= 3000000) && (hz < 6000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_16; // 3 MHz
        } else if ((hz >= 6000000) && (hz < 12000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_8; // 6 MHz
        } else if ((hz >= 12000000) && (hz < 24000000)) {
            obj->br_presc = SPI_BAUDRATEPRESCALER_4; // 12 MHz
        } else { // >= 24000000
            obj->br_presc = SPI_BAUDRATEPRESCALER_2; // 24 MHz
        }
    }

    init_spi(obj);
}

static inline int ssp_readable(spi_t *obj)
{
    int status;
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);
    // Check if data is received
    status = ((__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_RXNE) != RESET) ? 1 : 0);
    return status;
}

static inline int ssp_writeable(spi_t *obj)
{
    int status;
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);
    // Check if data is transmitted
    status = ((__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_TXE) != RESET) ? 1 : 0);
    return status;
}

static inline void ssp_write(spi_t *obj, int value)
{
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    while (!ssp_writeable(obj));
    //spi->DR = (uint16_t)value;
    if (obj->bits == SPI_DATASIZE_8BIT) {
        // Force 8-bit access to the data register
        uint8_t *p_spi_dr = 0;
        p_spi_dr = (uint8_t *) & (spi->DR);
        *p_spi_dr = (uint8_t)value;
    } else { // SPI_DATASIZE_16BIT
        spi->DR = (uint16_t)value;
    }
}

static inline int ssp_read(spi_t *obj)
{
    SPI_TypeDef *spi = (SPI_TypeDef *)(obj->spi);
    while (!ssp_readable(obj));
    //return (int)spi->DR;
    if (obj->bits == SPI_DATASIZE_8BIT) {
        // Force 8-bit access to the data register
        uint8_t *p_spi_dr = 0;
        p_spi_dr = (uint8_t *) & (spi->DR);
        return (int)(*p_spi_dr);
    } else {
        return (int)spi->DR;
    }
}

static inline int ssp_busy(spi_t *obj)
{
    int status;
    SpiHandle.Instance = (SPI_TypeDef *)(obj->spi);
    status = ((__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_BSY) != RESET) ? 1 : 0);
    return status;
}

int spi_master_write(spi_t *obj, int value)
{
    ssp_write(obj, value);
    return ssp_read(obj);
}

int spi_busy(spi_t *obj)
{
    return ssp_busy(obj);
}

#endif
