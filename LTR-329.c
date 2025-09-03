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


/** @brief I2C handle for communication with the LTR-329 sensor. */
static I2C_HandleTypeDef *LTR_329_I2C_HANDLE;


/********************************************************
 * @brief Initialize LTR-329 Sensor and I2C Connection  *
 * @param hi2c: Pointer to the I2C handle               *
 * @return 0 if successful, error code otherwise        *
 ********************************************************/
uint8_t LTR_329_Init(I2C_HandleTypeDef *hi2c) {

	LTR_329_I2C_HANDLE = hi2c;

	HAL_StatusTypeDef i2cStatus = HAL_OK; // Variable to store I2C status

	uint8_t resetStatus;

	/* Hard reset of LTR-329 */
	resetStatus = LTR_329_Reset(LTR_329_I2C_HANDLE);
	if (resetStatus != 0) {
		return resetStatus; // Return error code if reset fails
	}


	/* Make sure I2C is working properly */
	if (i2cStatus != HAL_OK) {
		return 1;
	}

	/* Check Device ID */
	uint8_t deviceID = 0x00; // Empty variable to store ID read from LTR_329_PART_ID Register
	i2cStatus = LTR_329_RegRead(LTR_329_PART_ID_ADDR, &deviceID);
	if ((i2cStatus != HAL_OK) || ((deviceID & 0xF0) != LTR_329_PART_ID)) {
		return 2;
	}

	/* Switch from Stand-by mode to Active mode in LTR_329_ALS_CONTR Register */
	i2cStatus = LTR_329_RegWrite(LTR_329_ALS_CONTR, 0x01);
	if (i2cStatus != HAL_OK) {
		return 1;
	}

	/* Return 0 if successfully initialized */
	return 0;
}

/** @brief Reset all registers of the LTR-329 sensor to their default values. */
uint8_t LTR_329_Reset(I2C_HandleTypeDef *hi2c) {

	LTR_329_I2C_HANDLE = hi2c; // Reset the I2C handle
	HAL_StatusTypeDef i2cStatus;

	/* SW Reset for LTR_329_ALS_CONTR Register */
	i2cStatus = LTR_329_RegWrite(LTR_329_ALS_CONTR, 0x02);
	if (i2cStatus != HAL_OK) {
		return 1;
	}

	HAL_Delay(25); // Wait 25ms

	return 0; // Return 0 if reset is successful
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
HAL_StatusTypeDef LTR_329_RegWrite(uint8_t regAddr, uint8_t regData) {
	return HAL_I2C_Mem_Write(LTR_329_I2C_HANDLE, LTR_329_I2C_ADDR, regAddr, I2C_MEMADD_SIZE_8BIT, &regData, 1, HAL_MAX_DELAY);
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
HAL_StatusTypeDef LTR_329_RegRead(uint8_t regAddr, uint8_t *regData) {
	return HAL_I2C_Mem_Read(LTR_329_I2C_HANDLE, LTR_329_I2C_ADDR, regAddr, I2C_MEMADD_SIZE_8BIT, regData, 1, HAL_MAX_DELAY);
}


/***************************************************************
 * @brief Read the C1 channel data from the LTR-329 sensor     *
 * @param hi2c: Pointer to the I2C handle                       *
 * @return 16-bit value of C1 channel data                      *
 *                                                             *
 * This function reads the C1 channel data from the LTR-329    *
 * sensor by reading two bytes from the ALS_DATA_CH1 registers. *
 ***************************************************************/
uint16_t LTR_329_Read_C1(I2C_HandleTypeDef *hi2c) {

	LTR_329_I2C_HANDLE = hi2c;
	HAL_StatusTypeDef i2cStatus;

	uint8_t data1;
	uint8_t data2;

	i2cStatus = LTR_329_RegRead(LTR_329_ALS_DATA_CH1_0, &data1);
	if (i2cStatus != HAL_OK) {
		return 1; // Return 1 if read fails
	}

	i2cStatus = LTR_329_RegRead(LTR_329_ALS_DATA_CH1_1, &data2);
	if (i2cStatus != HAL_OK) {
		return 1; // Return 1 if read fails
	}

	return (data2 << 8) | data1; // Combine the two bytes into a 16-bit value
}


/***************************************************************
 * @brief Read the C0 channel data from the LTR-329 sensor     *
 * @param hi2c: Pointer to the I2C handle                       *
 * @return 16-bit value of C0 channel data                      *
 *                                                             *
 * This function reads the C0 channel data from the LTR-329    *
 * sensor by reading two bytes from the ALS_DATA_CH0 registers. *
 ***************************************************************/
uint16_t LTR_329_Read_C0(I2C_HandleTypeDef *hi2c) {

	LTR_329_I2C_HANDLE = hi2c;
	HAL_StatusTypeDef i2cStatus;

	uint8_t data3;
	uint8_t data4;

	i2cStatus = LTR_329_RegRead(LTR_329_ALS_DATA_CH0_0, &data3);
	if (i2cStatus != HAL_OK) {
		return 1; // Return 1 if read fails
	}

	i2cStatus = LTR_329_RegRead(LTR_329_ALS_DATA_CH0_1, &data4);
	if (i2cStatus != HAL_OK) {
		return 1; // Return 1 if read fails
	}

	return (data4 << 8) | data3; // Combine the two bytes into a 16-bit value
}


/***************************************************************
 * @brief Read the ALS gain setting from the LTR-329 sensor    *
 * @param hi2c: Pointer to the I2C handle                       *
 * @return ALS gain value                                       *
 *                                                             *
 * This function reads the ALS gain setting from the LTR-329   *
 * sensor by reading the ALS_CONTR register.                    *
 ***************************************************************/
uint8_t LTR_329_Read_ALS_Gain(I2C_HandleTypeDef *hi2c) {

	LTR_329_I2C_HANDLE = hi2c;
	HAL_StatusTypeDef i2cStatus;

	uint8_t alsGainRawData; // Variable to store the ALS gain data

	i2cStatus = LTR_329_RegRead(LTR_329_ALS_CONTR, &alsGainRawData);
	if (i2cStatus != HAL_OK) {
		return 1; // Return 1 if read fails
	}

	// 00011100 -> 0x1C
	return (alsGainRawData & 0x1C) >> 2; // Return the ALS gain value in binary (2nd to 4th bits of the register)
}


/*****************************************************************
 * @brief Read the ALS interrupt status from the LTR-329 sensor  *
 * @param hi2c: Pointer to the I2C handle                        *
 * @return ALS interrupt status value                            *
 *                                                               *
 * This function reads the ALS integration time from the LTR-329 *
 * sensor by reading the ALS_MEAS_RATE register.                 *
 ****************************************************************/
uint8_t LTR_329_Read_ALS_Int(I2C_HandleTypeDef *hi2c) {

	LTR_329_I2C_HANDLE = hi2c;
	HAL_StatusTypeDef i2cStatus;

	uint8_t alsIntRawData; // Variable to store the ALS integration time data

	i2cStatus = LTR_329_RegRead(LTR_329_ALS_STATUS, &alsIntRawData);
	if (i2cStatus != HAL_OK) {
		return 1; // Return 1 if read fails
	}

	// 00111000 -> 0x38
	return (alsIntRawData & 0x38) >> 3; // Return the ALS integration time in binary (3rd to 5th bits of the register)
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
float LTR_329_Calculate_Lux(uint16_t c0Data, uint16_t c1Data, uint8_t alsGainData, uint8_t alsIntData) {

	// Validate input data to prevent division by zero
	if (alsGainData == 0 || alsIntData == 0 || (c0Data + c1Data) == 0) {
		return 0.0f; // Return 0 if any of the parameters are invalid
	}

	// Calculate the ratio of infrared to infrared + visible light data
	float ratio = (float)c1Data / (float)(c0Data + c1Data);

	// Adjusted calculation for ratio below 0.45
	if (ratio < 0.45f) {
		return (float)(1.7743 * c0Data + 1.1059 * c1Data) / alsGainData / alsIntData;
	}

	// Adjusted calculation for ratio between 0.45 and 0.64
	else if (ratio < 0.64f && ratio >= 0.45f) {
		return (float)(4.2785 * c0Data - 1.9548 * c1Data) / alsGainData / alsIntData;
	}

	// Adjusted calculation for ratio above 0.64 and below 0.85
	else if (ratio < 0.85f && ratio >= 0.64f) {
		return (float)(0.5926 * c0Data + 0.1185 * c1Data) / alsGainData / alsIntData;
	}

	// Return 0 if the ratio is above 0.85, indicating no valid lux calculation
	else {
		return 0.0f;
	}
}

