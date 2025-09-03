/**
 * @file LTR-329.h
 * @brief Header file for the LTR-329 ambient light sensor driver.
 * @author Kent Hong
 *
 * This file contains definitions and function prototypes for initializing
 * and interacting with the LTR-329 sensor over I2C.
 *
 * @note Ensure that the I2C peripheral is properly configured before using this driver.
 */

#ifndef INC_LTR_329_H_
#define INC_LTR_329_H_

#include <stdint.h>
#include "stm32L4xx_hal.h" // Adjust this include based on your STM32 series

/** @brief Register Defines for LTR-329 Ambient Light Sensor */
#define LTR_329_I2C_ADDR (0x29 << 1) // LTR-329 I2C address shifted for HAL
#define LTR_329_PART_ID_ADDR 0x86    // Device ID
#define LTR_329_ALS_CONTR 0x80       // ALS Control Register
#define LTR_329_ALS_MEAS_RATE 0x85   // ALS Measurement Rate Register
#define LTR_329_MANUFAC_ID 0x87		 // Manufacturer ID Register
#define LTR_329_ALS_DATA_CH1_0 0x88  // ALS Data Channel 1 Low Byte
#define LTR_329_ALS_DATA_CH1_1 0x89  // ALS Data Channel 1 High Byte
#define LTR_329_ALS_DATA_CH0_0 0x8A  // ALS Data Channel 0 Low Byte
#define LTR_329_ALS_DATA_CH0_1 0x8B  // ALS Data Channel 0 High Byte
#define LTR_329_ALS_STATUS 0x8C      // ALS Status Register

#define LTR_329_PART_ID 0xA0 // LTR-329 Part ID Default Value (0xA0 -> 1010)

/** @brief Struct to store LTR-329 variables */
typedef struct {
	uint16_t c0Data;           // Variable to store C0 channel data
	uint16_t c1Data;           // Variable to store C0 and C1 channel data
	uint8_t alsGainDataBinary; // Variable to store sensor binary gain data
	uint8_t alsGainData;       // Variable to store gain setting
	uint8_t alsIntDataBinary;  // Variable to store sensor binary integration time data
	uint16_t alsIntData;       // Variable to store integration time setting
	float alsLuxData;          // Variable to store calculated lux value
} LTR329_t;

/** @brief Function Prototypes for LTR-329 ALS */
uint8_t LTR_329_Init(I2C_HandleTypeDef *hi2c);
uint8_t LTR_329_Reset(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef LTR_329_RegWrite(uint8_t regAddr, uint8_t regData);
HAL_StatusTypeDef LTR_329_RegRead(uint8_t regAddr, uint8_t *regData);
uint16_t LTR_329_Read_C1(I2C_HandleTypeDef *hi2c);
uint16_t LTR_329_Read_C0(I2C_HandleTypeDef *hi2c);
uint8_t LTR_329_Read_ALS_Gain(I2C_HandleTypeDef *hi2c);
uint8_t LTR_329_Read_ALS_Int(I2C_HandleTypeDef *hi2c);
float LTR_329_Calculate_Lux(uint16_t c0Data, uint16_t c1Data, uint8_t alsGainData, uint8_t alsIntData);

#endif /* INC_LTR_329_H_ */
