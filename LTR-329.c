/**
 * @file LTR-329.c
 * @brief Implementation of the LTR-329 Ambient Light Sensor driver.
 * @author Kent Hong
 *
 * This file contains functions to initialize the LTR-329 sensor,
 * reset its registers, and read/write data via I2C.
 *
 * @note Ensure that the I2C peripheral is properly configured before using this driver.
 */

#include "LTR-329.h"


/** @brief Default register values for the LTR-329 sensor. */
uint8_t LTR_329_REG_CONFIG_SETTINGS[8] = {
	0x00, // ALS_CONTR: Default settings
	0x03, // ALS_MEAS_RATE: Default settings
	0xA0, // PART_ID: Default settings
	0x05, // MANUFAC_ID: Default settings
	0x00, // ALS_DATA_CH1_0: Default settings
	0x00, // ALS_DATA_CH1_1: Default settings
	0x00, // ALS_DATA_CH0_0: Default settings
	0x00, // ALS_DATA_CH0_1: Default settings
	0x00  // ALS_STATUS: Default settings
};

/** @brief Arrays to store binary mapping to readable values for ALS_CONTR and ALS_MEAS_RATE registers */
const uint8_t gainMap[] = {1, 2, 4, 8, 48, 96}; // Gain mapping in pg. 13 of LTR-329 datasheet
const uint16_t intTimeMap[] = {100, 50, 200, 400, 150, 250, 300, 350}; // Integration time mapping in pg. 14 of LTR-329 datasheet


/********************************************************
 * @brief Initialize LTR-329 Sensor and I2C Connection  *
 * @param hi2c: Pointer to the I2C handle               *
 * @return 0 if successful, error code otherwise        *
 ********************************************************/
void LTR_329_Init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart, LTR329_t *ltr329) {

	HAL_StatusTypeDef i2cStatus = HAL_OK; // Variable to store I2C status

	/* Hard reset of LTR-329 */
	LTR_329_Reset(hi2c, huart, ltr329);

	/* Make sure I2C is working properly */
	if (i2cStatus != HAL_OK) {
		sprintf(ltr329->buffer, "I2C Init Error: %d\r\n", i2cStatus);
		HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}

	/* Check Device ID */
	uint8_t deviceID = 0x00; // Empty variable to store ID read from LTR_329_PART_ID Register
	i2cStatus = LTR_329_RegRead(hi2c, LTR_329_PART_ID_ADDR, &deviceID);
	if ((i2cStatus != HAL_OK) || ((deviceID & 0xF0) != LTR_329_PART_ID)) {
		sprintf(ltr329->buffer, "I2C Read Error: %d\r\n", i2cStatus);
		HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}

	/* Switch from Stand-by mode to Active mode in LTR_329_ALS_CONTR Register */
	i2cStatus = LTR_329_RegWrite(hi2c, LTR_329_ALS_CONTR, 0x01);
	if (i2cStatus != HAL_OK) {
		sprintf(ltr329->buffer, "I2C Write Error: %d\r\n", i2cStatus);
		HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}
}

/** @brief Reset all registers of the LTR-329 sensor to their default values. */
void LTR_329_Reset(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart, LTR329_t *ltr329) {

	HAL_StatusTypeDef i2cStatus;

	/* SW Reset for LTR_329_ALS_CONTR Register */
	i2cStatus = LTR_329_RegWrite(hi2c, LTR_329_ALS_CONTR, 0x02);
	if (i2cStatus != HAL_OK) {
		sprintf(ltr329->buffer, "I2C Write Error: %d\r\n", i2cStatus);
		HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}

	HAL_Delay(25); // Wait 25ms
}


/***************************************************************
 * @brief Write a byte to a specific register of LTR-329 	   *
 * @param regAddr: Register address to write to         	   *
 * @param regData: Data to write to the register        	   *
 * @return HAL status code                                     *
 *                                                             *
 * Refer to the HAL User Manual, page 497 for function details *
 * @cite UM1884                                                *
 ***************************************************************/
HAL_StatusTypeDef LTR_329_RegWrite(I2C_HandleTypeDef *hi2c, uint8_t regAddr, uint8_t regData) {
	return HAL_I2C_Mem_Write(hi2c, LTR_329_I2C_ADDR, regAddr, I2C_MEMADD_SIZE_8BIT, &regData, 1, HAL_MAX_DELAY);
}


/***************************************************************
 * @brief Read a byte from a specific register of LTR-329      *
 * @param regAddr: Register address to read from               *
 * @param regData: Pointer to store the read data              *
 * @return HAL status code                                     *
 *                                                             *
 * Refer to the HAL User Manual, page 497 for function details *
 * @cite UM1884                                                *
 ***************************************************************/
HAL_StatusTypeDef LTR_329_RegRead(I2C_HandleTypeDef *hi2c, uint8_t regAddr, uint8_t *regData) {
	return HAL_I2C_Mem_Read(hi2c, LTR_329_I2C_ADDR, regAddr, I2C_MEMADD_SIZE_8BIT, regData, 1, HAL_MAX_DELAY);
}


/*************************************************************************************
 * @brief Read the CH1, CH0, Gain, and Integration Time data from the LTR-329 sensor *
 * @param hi2c: Pointer to the I2C handle                                            *
 * @param ltr329: Pointer to the LTR329_t struct                                     *
 * @return 16-bit value of C1 channel data                                           *
 *                                                                                   *
 * This function reads the necessary data from the LTR-329 to calculate lux.         *
 ************************************************************************************/
void LTR_329_Read_All(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart, LTR329_t *ltr329) {

	HAL_StatusTypeDef i2cStatus;

	uint8_t c1RawData1, c1RawData2, c0RawData1, c0RawData2, gainRawData, intTimeRawData;

	/* Read C0 channel data */
	i2cStatus = LTR_329_RegRead(hi2c, LTR_329_ALS_DATA_CH1_0, &c1RawData1);
	if (i2cStatus != HAL_OK) {
		sprintf(ltr329->buffer, "I2C Read Error: %d\r\n", c1RawData1);
		HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}

	i2cStatus = LTR_329_RegRead(hi2c, LTR_329_ALS_DATA_CH1_1, &c1RawData2);
	if (i2cStatus != HAL_OK) {
		sprintf(ltr329->buffer, "I2C Read Error: %d\r\n", c1RawData2);
		HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}

	ltr329->c1Data = (c1RawData2 << 8) | c1RawData1; // Combine the two bytes into a 16-bit value

	/* Read C1 channel data */
	i2cStatus = LTR_329_RegRead(hi2c, LTR_329_ALS_DATA_CH0_0, &c0RawData1);
	if (i2cStatus != HAL_OK) {
		sprintf(ltr329->buffer, "I2C Read Error: %d\r\n", c0RawData1);
		HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}

	i2cStatus = LTR_329_RegRead(hi2c, LTR_329_ALS_DATA_CH0_1, &c0RawData2);
	if (i2cStatus != HAL_OK) {
		sprintf(ltr329->buffer, "I2C Read Error: %d\r\n", c0RawData2);
		HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}

	ltr329->c0Data = (c0RawData2 << 8) | c0RawData1; // Combine the two bytes into a 16-bit value

	/* Read ALS Gain */
	i2cStatus = LTR_329_RegRead(hi2c, LTR_329_ALS_CONTR, &gainRawData);
	if (i2cStatus != HAL_OK) {
			sprintf(ltr329->buffer, "I2C Read Error: %d\r\n", gainRawData);
			HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
		}

	/* Map the binary gain data to actual gain values */
	switch (gainRawData) {
		case 0: // b'000
			ltr329->alsGainData = gainMap[0]; // 1x gain
			break;
		case 1: // b'001
			ltr329->alsGainData = gainMap[1]; // 2x gain
			break;
		case 2: // b'010
			ltr329->alsGainData = gainMap[2]; // 4x gain
			break;
		case 3: // b'011
			ltr329->alsGainData = gainMap[3]; // 8x gain
			break;
		case 6: // b'110
			ltr329->alsGainData = gainMap[4]; // 48x gain
			break;
		case 7: // b'111
			ltr329->alsGainData = gainMap[5]; // 96x gain
			break;
		default: // Invalid gain setting
			ltr329->alsGainData = 0;
			sprintf(ltr329->buffer, "Invalid ALS Gain Data: %u\r\n", gainRawData);
			HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
			break;
	}

	/* Read ALS integration time */
	i2cStatus = LTR_329_RegRead(hi2c, LTR_329_ALS_STATUS, &intTimeRawData);
	if (i2cStatus != HAL_OK) {
		 sprintf(ltr329->buffer, "I2C Read Error: %d\r\n", intTimeRawData);
		 HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}

	/* Map the binary integration time data to actual integration time values */
	switch (intTimeRawData) {
		case 0: // b'000
			ltr329->alsIntData = intTimeMap[0]; // 100 ms integration time
			break;
		case 1: // b'001
			ltr329->alsIntData = intTimeMap[1]; // 50 ms integration time
			break;
		case 2: // b'010
			ltr329->alsIntData = intTimeMap[2]; // 200 ms integration time
			break;
		case 3: // b'011
			ltr329->alsIntData = intTimeMap[3]; // 400 ms integration time
			break;
		case 4: // b'100
			ltr329->alsIntData = intTimeMap[4]; // 150 ms integration time
			break;
		case 5: // b'101
			ltr329->alsIntData = intTimeMap[5]; // 250 ms integration time
		  	break;
		case 6: // b'110
			ltr329->alsIntData = intTimeMap[6]; // 300 ms integration time
			break;
		case 7: // b'111
			ltr329->alsIntData = intTimeMap[7]; // 350 ms integration time
			break;
		default: // Invalid integration time setting
			ltr329->alsIntData = 0;
			sprintf(ltr329->buffer, "Invalid ALS Integration Time Data: %u\r\n", intTimeRawData);
			HAL_UART_Transmit(huart, (uint8_t*)ltr329->buffer, strlen(ltr329->buffer), HAL_MAX_DELAY);
	}
}


/****************************************************************************************
 * @brief Calculate the lux value based on C0 data, C1 data, gain, and integration time *
 * @param c0Data: C0 channel data                                                       *
 * @param c1Data: C1 channel data                                                       *
 * @param alsGainData: ALS gain                                                 *
 * @param alsIntData: ALS integration time                                              *
 * @return Calculated lux value                                                         *
 *                                                                                      *                                                     *
 * Formula for Lux provided in Appendix A, pg. 3                                        *
 * @cite Appendix A                                                                     *
 ***************************************************************************************/
void LTR_329_Calculate_Lux(LTR329_t *ltr329) {

	// Validate input data to prevent division by zero
	if (ltr329->alsGainData == 0 || ltr329->alsIntData == 0 || (ltr329->c0Data + ltr329->c1Data) == 0) {
		ltr329->alsLuxData = 0.0f; // Return 0 if any of the parameters are invalid
	}

	// Calculate the ratio of infrared to infrared + visible light data
	float ratio = (float)ltr329->c1Data / (float)(ltr329->c0Data + ltr329->c1Data);

	// Adjusted calculation for ratio below 0.45
	if (ratio < 0.45f) {
		ltr329->alsLuxData = (float)(1.7743 * ltr329->c0Data + 1.1059 * ltr329->c1Data) / ltr329->alsGainData / ltr329->alsIntData;
	}

	// Adjusted calculation for ratio between 0.45 and 0.64
	else if (ratio < 0.64f && ratio >= 0.45f) {
		ltr329->alsLuxData = (float)(4.2785 * ltr329->c0Data - 1.9548 * ltr329->c1Data) / ltr329->alsGainData / ltr329->alsIntData;
	}

	// Adjusted calculation for ratio above 0.64 and below 0.85
	else if (ratio < 0.85f && ratio >= 0.64f) {
		ltr329->alsLuxData = (float)(0.5926 * ltr329->c0Data + 0.1185 * ltr329->c1Data) / ltr329->alsGainData / ltr329->alsIntData;
	}

	// Return 0 if the ratio is above 0.85, indicating no valid lux calculation
	else {
		ltr329->alsLuxData = 0.0f;
	}
}

