/**
 * @file  vl53l1_platform.h
 * @brief Those platform functions are platform dependent and have to be implemented by the user
 */

#ifndef _VL53L1_PLATFORM_H_
#define _VL53L1_PLATFORM_H_

#include "vl53l1_types.h"

//////////////////////////
// Own sensors definitions
// Warning: error may accure if num of devices differ between dts and NUM_OF_DEVICES
// Code adapted specifically for 53L1A1 module up to 3 sensors usage
/////////////////////////
#define ST_TOF_IOCTL_WFI 1
#define NUM_OF_DEVICES 1

#define CENTER_DEVICE 0 // id and minor number
#define LEFT_DEVICE 1
#define RIGHT_DEVICE 2

/////////////////////
// END of definitions
////////////////////

#ifdef __cplusplus
extern "C"
{
#endif

	/** @brief VL53L1_WrByte() definition.\n
	 * To be implemented by the developer
	 */
	int8_t VL53L1_WrByte(
		uint16_t dev,
		uint16_t index,
		uint8_t data);
	/** @brief VL53L1_WrWord() definition.\n
	 * To be implemented by the developer
	 */
	int8_t VL53L1_WrWord(
		uint16_t dev,
		uint16_t index,
		uint16_t data);
	/** @brief VL53L1_WrDWord() definition.\n
	 * To be implemented by the developer
	 */
	int8_t VL53L1_WrDWord(
		uint16_t dev,
		uint16_t index,
		uint32_t data);
	/** @brief VL53L1_RdByte() definition.\n
	 * To be implemented by the developer
	 */
	int8_t VL53L1_RdByte(
		uint16_t dev,
		uint16_t index,
		uint8_t *pdata);
	/** @brief VL53L1_RdWord() definition.\n
	 * To be implemented by the developer
	 */
	int8_t VL53L1_RdWord(
		uint16_t dev,
		uint16_t index,
		uint16_t *pdata);
	/** @brief VL53L1_RdDWord() definition.\n
	 * To be implemented by the developer
	 */
	int8_t VL53L1_RdDWord(
		uint16_t dev,
		uint16_t index,
		uint32_t *pdata);
	/** @brief VL53L1_WaitMs() definition.\n
	 * To be implemented by the developer
	 */
	int8_t VL53L1_WaitMs(
		uint16_t dev,
		int32_t wait_ms);

	int8_t VL53L1_WriteMulti(uint16_t dev,
							 uint16_t index, uint8_t *pdata, uint32_t count);

	int8_t VL53L1_ReadMulti(uint16_t dev,
							uint16_t index, uint8_t *pdata, uint32_t count);

	int8_t VL53L1X_UltraLite_Linux_Interrupt_Init(void);

	int8_t VL53L1X_UltraLite_WaitForInterrupt(uint16_t sensor_id, int IoctlWfiNumber);

	int8_t VL53L1X_UltraLite_Linux_I2C_Init(uint16_t dev,
											int i2c_adapter_nr, uint8_t i2c_Addr);

#ifdef __cplusplus
}
#endif

#endif
