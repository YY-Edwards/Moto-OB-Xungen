/*
 * xgflash.c
 *
 * Created: 2017/11/8 16:49:42
 *  Author: Edwards
 */ 

#include "xgflash.h"

/*! \brief This is an example demonstrating flash read / write data accesses
 *         using the FLASHC driver.
 *
 * \param caption     Caption to print before running the example.
 * \param nvram_data  Pointer to the NVRAM data structure to use in the example.
 */
/*
void flash_rw_example(const char *caption, nvram_data_t *nvram_data)
{
	static const uint8_t write_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

	Disable_interrupt_level(1);
	flashc_memset((void *)nvram_data, 0x00, 8, sizeof(*nvram_data), true);
	Enable_interrupt_level(1);
	
	Disable_interrupt_level(1);
	flashc_memcpy((void *)&nvram_data->var8,   &write_data, sizeof(nvram_data->var8),   true);
	Enable_interrupt_level(1);
	
	Disable_interrupt_level(1);
	flashc_memcpy((void *)&nvram_data->var16,  &write_data, sizeof(nvram_data->var16),  true);
	Enable_interrupt_level(1);
	flashc_memcpy((void *)&nvram_data->var8_3, &write_data, sizeof(nvram_data->var8_3), true);
	flashc_memcpy((void *)&nvram_data->var32,  &write_data, sizeof(nvram_data->var32),  true);
	
}

*/
void xg_flashc_init(void)
{
	char ch =0;
	nvram_data_t  *flash_nvram_data ;
	flash_nvram_data = 0x80003200;//LABEL_ADDRESS;
	nvram_data_t temp_data;
	static const uint8_t write_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
	
	//flashc_lock_all_regions(false);
	
	//flashc_memset((void *)0x80013200, 0x0000, 8, 6, true);
	
	//flashc_memcpy((void *)0x8000CBDA,  &write_data, 6,  true);
	
	//flash_rw_example(ch, flash_nvram_data);
	//flash_rw_example(ch, &temp_data);
	
}