#include <asm/gpio.h>
#include <common.h>
#include <asm/io.h>
#include "./smc_call/cortina_smc.h"


#define OTP_READ_CHIP_MODE     CA_SMC_CALL_OTP_READ_CHIP_MODE


int reset_button_gpio(void) 
{
	char name[] = CONFIG_SYS_BOARD;
	int ret=0;
	char rbuf[2] = {0};
	ret = OTP_READ_CHIP_MODE((unsigned long)&rbuf[0]);
	//printf("board name；%s chip_mode: 0x%02x 0x%02x\n",name,rbuf[0],rbuf[1]);
	if (!strncmp(name, "taurus", 6)) {
		if((rbuf[1] & 0x2) && ((rbuf[0] & 0x10) || (rbuf[0] & 0x40)  )){ 
			//9607DM,9617CM
			return 4*32+8;   	//GPIO4[8]
		}else{	
			//9607DS,9617C &
			return 8;	  		//GPIO[8]
		}
	}else if(!strncmp(name, "elnath", 6)) {
		if(! (rbuf[1] & 0x3) && (rbuf[0] & 0x40) ){
			//9607FQ
			return 8;			//GPIO[8]
		}else{	
			//9607FS,9607F &
			return 4*32+8;   	//GPIO4[8]
		}
	}else 
		return -1;
}


int get_reset_button(void)
{
	/* grab the pin before we tweak it */
	int ret;
	int gpio=reset_button_gpio();
	//printf("reset button gpio %d\n",gpio);
	ret = gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return 0;
	}
	gpio_direction_input(gpio);
	return !gpio_get_value(gpio);
}

