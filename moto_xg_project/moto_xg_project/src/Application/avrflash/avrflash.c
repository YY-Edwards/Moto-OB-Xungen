/*
 * avrflash.c
 *
 * Created: 2018/12/3 16:41:10
 *  Author: Edwards
 */ 
#include "avrflash.h"
#include <string.h>


#pragma pack(1)
//! Structure type containing variables to store in NVRAM using a specific
//! memory map.
typedef const struct {
	uint8_t  var8;
	uint16_t var16;
	uint8_t  var8_3[3];
	uint32_t var32;
} nvram_data_t;
#pragma pack()


//! NVRAM data structure located in the flash array.
#if defined (__GNUC__)
__attribute__((__section__(".flash_nvram")))
#endif
static nvram_data_t flash_nvram_data
#if defined (__ICCAVR32__)
@ "FLASH_NVRAM"
#endif
;

//! NVRAM data structure located in the User page.
#if defined (__GNUC__)
__attribute__((__section__(".userpage")))
#elif defined(__ICCAVR32__)
__no_init
// This code is added so that this example can be programmed through
// batchisp and a bootloader. Else, IAR will set this variable as
// loadable and batchisp will err because this variable is out of the
// flash memory range (it's in the user page).
// GCC will init this variable at run time not during the programming
// of the application.
#endif
static nvram_data_t user_nvram_data
#if defined (__ICCAVR32__)
@ "USERDATA32_C"
#endif
;


void write_flash_in_multitask(volatile void *dst, const void *src, size_t nbytes)
{
	//Disable_interrupt_level(1);
	//vTaskSuspendAll();
	
	flashc_lock_page_region(1020, false);
	
	flashc_memcpy(dst, src, nbytes, true);
	//xTaskResumeAll();
	//Enable_interrupt_level(1);
	
	flashc_lock_page_region(1020, true);
	
	
}


static void flash_rw_example(const char *caption, nvram_data_t *nvram_data)
{
	
	 uint8_t write_data[128];
	for(int i = 0; i < 128; ++i)write_data[i] = i;
	
	uint16_t temp = 0xAB37;
	
	//static char str[80];
	//memset(str, 0x00, sizeof(str));
	
	//print_dbg("\r\nClearing NVRAM variables...");
	//flashc_memset((void *)nvram_data, 0x3B5A2B4A, 32, 256, true);
	
	
	//print_dbg("\r\nNVRAM variables cleared:\r\n");
	//print_nvram_variables(nvram_data);

	//print_dbg("\r\nWriting new values to NVRAM variables...");
	flashc_memcpy((void *)0x80800000,   write_data, 128,   true);
	//flashc_memcpy((void *)&nvram_data->var16,  &write_data, sizeof(nvram_data->var16),  true);
	//flashc_memcpy((void *)&nvram_data->var8_3, &write_data, sizeof(nvram_data->var8_3), true);
	//flashc_memcpy((void *)&nvram_data->var32,  &write_data, sizeof(nvram_data->var32),  true);
	//print_dbg("\r\nNVRAM variables written:\r\n");
	//print_nvram_variables(nvram_data);
	//flashc_memcpy((void *)str,  (void *)&nvram_data->var8, sizeof(nvram_data->var8),  false);
	
		
}

void avr_flash_test()
{
	flash_rw_example("user_page", &user_nvram_data);		
}