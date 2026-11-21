#ifndef __BOARD_ID_H__
#define __BOARD_ID_H__
/*
	bit 0~7 --- WIFI
	bit 8~15 --- SLIC
	bit 16~23 --- GPIO
	bit 24~31 --- reserved
*/
enum BOARD_WIFI_TYPE_DEFINE{
	BOARD_WIFI_NONE = 0,
	BOARD_WIFI_WIFI5 = 1, //bit 0
	BOARD_WIFI_WIFI6 = 2, //bit 1
};

enum BOARD_GPIO_TYPE_DEFINE{
	BOARD_GPIO_DEF=0,
	BOARD_GPIO_TYPE1,
};

#define BOARD_WIFI_TYPE(x) ((x)&0xff)
#define BOARD_SLIC_TYPE(x) (((x)>>8)&0xff)
#define BOARD_GPIO_TYPE(x) (((x)>>16)&0xff)

#ifdef __KERNEL__
extern unsigned int boot_board_id;
#endif

#endif //__BOARD_ID_H__
