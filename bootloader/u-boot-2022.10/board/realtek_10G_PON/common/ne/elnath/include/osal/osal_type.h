/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2002 by Cortina Systems Incorporated.                 */
/***********************************************************************/
/*
 * osal_type.h
 *
 * Include file containing some basic and common data types and defines
 * used by driver.
 *
 * $Id$
*/
#ifndef __OSAL_TYPE_H__
#define __OSAL_TYPE_H__

#ifndef __ASSEMBLER__
#include <stdint.h>

typedef  unsigned long long     uint64 ;
typedef  long long              int64  ;
typedef  unsigned int           uint32 ;
typedef  unsigned int           uint ;
typedef  int                    int32  ;
typedef  unsigned short         uint16 ;
typedef  short                  int16  ;
typedef  unsigned char          uint8  ;
typedef  signed char            int8   ;
typedef  unsigned char          boolean;
///--typedef  unsigned int           size_t;

typedef  unsigned long          ca_uint;
typedef  long                   ca_int;
typedef  unsigned long          ca_uint_t;
typedef  long                   ca_int_t;

/*
 * Basic data types
 */
typedef  unsigned long long     ca_uint64;
typedef  long long              ca_int64;
typedef  unsigned int           ca_uint32;
typedef  int                    ca_int32;
typedef  unsigned short         ca_uint16;
typedef  short                  ca_int16;
typedef  unsigned char          ca_uint8;
typedef  signed char            ca_int8;
typedef  ca_uint8               ca_boolean;
typedef  ca_int32               ca_status ;
typedef  ca_uint32              ca_addr ;


typedef  ca_int8                ca_int8_t;
typedef  ca_int16               ca_int16_t ;
typedef  ca_int32               ca_int32_t ;
typedef  ca_uint8               ca_uint8_t;
typedef  ca_uint16              ca_uint16_t ;
typedef  ca_uint32              ca_uint32_t ;
typedef  ca_uint64              ca_uint64_t ;

typedef  ca_uint16              ca_dev_id_t ;
///--typedef  ca_uint32              ca_port_id_t ;
typedef  ca_uint32_t            ca_vlan_id_t;
typedef int                     ca_cos_t;
typedef int                     ca_queue_t;
typedef  unsigned int           ca_tunnel_id_t;
typedef  unsigned int           ca_rtp_id_t;
typedef  unsigned short         ca_session_id_t;
typedef  unsigned short         ca_ppp2_pro_t;
typedef  unsigned int           ca_ip_translate_id_t;

typedef unsigned long sector_t;
typedef	unsigned int	UINT32;
typedef unsigned short  UINT16;
typedef	unsigned char	UINT8;
typedef	unsigned long long		UINT64;

/* FIXME: re-visit CA_UNION and CA_DI - for now ported over from Cortina */
#ifndef  CA_UNION
#define  CA_UNION               union
#endif

#ifndef CA_DONT_USE_DESGNTD_INITLZR
#define  CA_DI(x) x
#else
#define  CA_DI(x)
#endif

#ifndef __LINE__
#define __LINE__ 0
#endif
#ifndef __FILE__
#define __FILE__ "unknown"
#endif

/* Conflict with Linux definition
#ifndef __FUNCTION__
#define __FUNCTION__ __FILE__
#endif
*/

/*
 * MAC address struct
 */
///--
#if 0
typedef struct {
  ca_uint8      byte5 ;
  ca_uint8      byte4 ;
  ca_uint8      byte3 ;
  ca_uint8      byte2 ;
  ca_uint8      byte1 ;
  ca_uint8      byte0 ;
} ca_mac_t ;
#endif

/*
 * Register data type
 */
typedef ca_uint32 ca_reg;

/*
 * Other typedef's
 */
///--
#if 0
typedef enum {
  CA_DISABLE   = 0,
  CA_ENABLE    = 1
} ca_ctl_t ;

typedef enum {
  CA_RESET_DEASSERT    = 0,
  CA_RESET_ASSERT      = 1,
  CA_RESET_TOGGLE      = 2
} ca_reset_action_t ;

typedef enum {
  CA_TX       = 0,
  CA_RX,
  CA_TX_AND_RX,
  CA_RX_AND_TX = CA_TX_AND_RX
} ca_dir_t ;

typedef enum {
  CA_OP_READ,
  CA_OP_WRITE
} ca_rw_t ;

typedef enum {
  CA_FALSE   = 0,
  CA_TRUE    = 1
} ca_boolean_t;

/*
 * Chip(Driver) Type
 */
typedef enum {
  CA_CHIP_UNKNOWN  = 0,
  CA_CHIP_MISANO,
  CA_CHIP_DAYTONA,
  CA_CHIP_ESTORIL,
  CA_CHIP_SUNI_TETRA
} ca_chip_type_t ;
#endif

/*
 * Other defines
 */
#define CA_DONE			1
#define CA_OK           0
#define CA_ERROR        -1
#define CA_ERR_ENTRY_NOT_FOUND	-2

/* error codes */
#define RE_OK                 0
#define RE_ERR                1
#define RE_ERR_CMD_NOT_FOUND  2
#define RE_ERR_CRC            3
#define RE_ERR_SW             4
#define RE_ERR_RANGE          5
#define RE_ERR_ALREADY_INITIALIZED 6
#define RE_ERR_IPC           7
#define RE_ERR_WRONG_CONTENT  8
#define RE_ERR_BUSY           9
#define RE_ERR_NO_RESOURCE   10
#define RE_ERR_LENGTH        11
#define RE_ERR_NO_PKT        12

///--
#if 0
typedef enum {
	CA_E_ERROR    = CA_ERROR,
	CA_E_OK       = CA_OK,
	CA_E_NONE     = CA_OK,
	CA_E_INIT,
	CA_E_DEV_ID,
	CA_E_PORT_ID,
	CA_E_RESOURCE,
	CA_E_PARAM,
	CA_E_DIR,
	CA_E_EXISTS,
	CA_E_NULL_PTR,
	CA_E_NOT_FOUND,
	CA_E_CONFLICT,
	CA_E_TIMEOUT,
	CA_E_INUSING,
	CA_E_NOT_SUPPORT,
	CA_E_MCAST_ADDR_EXISTS,
	CA_E_MCAST_ADDR_ADD_FAIL,
	CA_E_MCAST_ADDR_DELETE_FAIL,
	CA_E_MEM_ALLOC,
	CA_E_NEXTHOP_INVALID,
	CA_E_NEXTHOP_NOT_FOUND,
	CA_E_ROUTE_MAX_LIMIT,
	CA_E_ROUTE_NOT_FOUND,
  CA_E_INTERNAL,
  CA_E_BUSY,
	CA_E_FULL,

} ca_status_t;
#endif

typedef enum {
    CA_PORT_TYPE_GPON   = 0x1,
    CA_PORT_TYPE_EPON   = 0x2,
    CA_PORT_TYPE_ETHERNET = 0x3,
    CA_PORT_TYPE_CPU    = 0x4,
    CA_PORT_TYPE_SUBPORT  = 0x5,
    CA_PORT_TYPE_OFFLOAD  = 0x6,
    CA_PORT_TYPE_L2RP   = 0x7,
    CA_PORT_TYPE_TRUNK    = 0x8,
    CA_PORT_TYPE_OMCC   = 0x9,
    CA_PORT_TYPE_L3   = 0xa,
    CA_PORT_TYPE_vUNI   = 0xb,
    CA_PORT_TYPE_LLID   = 0xd,
    CA_PORT_TYPE_VIRTUAL  = 0xe,
    CA_PORT_TYPE_INVALID  = 0xFFFF,
} ca_port_type_t;

#define CA_PORT_TYPE_SHIFT   16
#define CA_PORT_TYPE_MASK    0xFFFF
#define CA_PORT_ID_MASK      0xFFFF
#define CA_PORT_ID(port_type, port_id)   ((((port_type)&CA_PORT_TYPE_MASK)<<CA_PORT_TYPE_SHIFT) | ((port_id)&CA_PORT_ID_MASK))
#define PORT_TYPE(ca_portid)             (((ca_portid) >> CA_PORT_TYPE_SHIFT) & CA_PORT_TYPE_MASK)
#define PORT_ID(ca_portid)               (ca_portid & CA_PORT_ID_MASK)

#define CA_ETH_ADDR_LEN     6
typedef ca_uint8_t ca_mac_addr_t[CA_ETH_ADDR_LEN];
typedef ca_uint32_t             ca_flow_id_t;
typedef ca_uint16_t             ca_device_id_t;

#if 0
#define REG_READ(r)                                   *(volatile unsigned int *) (((r>=0x90000000)&&(r<0xA0000000)) ? (r-0x20000000) : r)
#define REG_WRITE(r, v)                               *(volatile unsigned int*) (((r>=0x90000000)&&(r<0xA0000000)) ? (r-0x20000000) : r) =(v)
#endif

#ifndef TRUE
#define TRUE            1
#endif

#ifndef FALSE
#define FALSE           0
#endif

#ifndef NULL
#define NULL            0
#endif

#ifndef IN
#define IN
#define CA_IN

#endif

#ifndef OUT
#define OUT
#define CA_OUT
#endif

#ifndef CA_IN_OUT
#define CA_IN_OUT
#endif

#ifdef CA_DONT_USE_INLINE
#       define CA_INLINE        static
#else
#       define CA_INLINE        __inline__ static
#endif

#define CA_PORT_INDEX_INVALID             0xFF
#define CA_PORT_INDEX_SHIFT             0
#define CA_PORT_INDEX_MASK              0xFF

typedef enum {/*CODEGEN_IGNORE_TAG*/
    CA_PORT_ID_NI0 = 0,     /* 1 Gbps NI Port (Integrated GPHY) */
    CA_PORT_ID_NI1 = 1,     /* 1 Gbps NI Port (Integrated GPHY) */
    CA_PORT_ID_NI2 = 2,     /* 1 Gbps NI Port (Integrated GPHY) */
    CA_PORT_ID_NI3 = 3,     /* 1 Gbps NI Port (Integrated GPHY) */
    CA_PORT_ID_NI4 = 4,     /* 1 Gbps NI Port (RGMII)*/
#ifndef CONFIG_ARCH_CORTINA_G3LITE
    CA_PORT_ID_NI5 = 5,     /* 10 Gbps XFI NI Port (RXAUI/2.5GSGMII/RGMII) */
    CA_PORT_ID_NI6 = 6,     /* 10 Gbps XFI NI Port (LAN) */
    CA_PORT_ID_NI7 = 7,     /* 10 Gbps XFI NI Port (WAN) */
#endif/*CODEGEN_IGNORE_TAG*/
#ifdef CONFIG_ARCH_CORTINA_G3LITE/*CODEGEN_IGNORE_TAG*/
    CA_PORT_ID_NI5 = CA_PORT_INDEX_INVALID,
    CA_PORT_ID_NI6 = CA_PORT_INDEX_INVALID,
    CA_PORT_ID_NI7 = CA_PORT_INDEX_INVALID,
#endif/*CODEGEN_IGNORE_TAG*/
    CA_PORT_ID_DEEPQ0 = 0,
    CA_PORT_ID_DEEPQ1 = 1,
    CA_PORT_ID_DEEPQ2 = 2,
    CA_PORT_ID_DEEPQ3 = 3,
    CA_PORT_ID_DEEPQ4 = 4,
#ifndef CONFIG_ARCH_CORTINA_G3LITE
    CA_PORT_ID_DEEPQ5 = 5,
    CA_PORT_ID_DEEPQ6 = 6,
    CA_PORT_ID_DEEPQ7 = 7,
#endif/*CODEGEN_IGNORE_TAG*/
#ifdef CONFIG_ARCH_CORTINA_G3LITE/*CODEGEN_IGNORE_TAG*/
    CA_PORT_ID_DEEPQ5 = CA_PORT_INDEX_INVALID,
    CA_PORT_ID_DEEPQ6 = CA_PORT_INDEX_INVALID,
    CA_PORT_ID_DEEPQ7 = CA_PORT_INDEX_INVALID,
#endif/*CODEGEN_IGNORE_TAG*/
    CA_PORT_ID_CPU0 = 16,
    CA_PORT_ID_CPU1 = 17,
    CA_PORT_ID_CPU2 = 18,
    CA_PORT_ID_CPU3 = 19,
    CA_PORT_ID_CPU4 = 20,
    CA_PORT_ID_CPU5 = 21,
    CA_PORT_ID_CPU6 = 22,
    CA_PORT_ID_CPU7 = 23,

    CA_PORT_ID_L3_WAN = 24,        /* port on L3 (interface) */
    CA_PORT_ID_L3_LAN = 25,        /* port on L3 (interface) */

    CA_PORT_ID_OAM = 26,        /* oam port */
    CA_PORT_ID_MC = 27,         /* multicast port */
    CA_PORT_ID_CPU_Q = 29,      /* cpu port */
    CA_PORT_ID_SPCL = 30,       /* ldpid for special packet resolution */
    CA_PORT_ID_BLACKHOLE = 31,

    CA_PORT_ID_CPU_MQ_MIN = 32,
    CA_PORT_ID_CPU_MQ_MAX = 63,

} ca_port_index_t;

#define CA_MAX_GEM_PORTS        (64)
#define CA_GPON_MAX_TCONTS      (8)
#define CA_MAX_NUM_TCONT_QUEUE      (8)


typedef enum {
    CA_SPEED_10MBPS     = 0,
    CA_SPEED_100MBPS,
    CA_SPEED_1000MBPS,
    CA_SPEED_INVALID    = 0xFF,
} ca_speed_t;

typedef enum {
    CA_DUPLEX_HALF      = 0,
    CA_DUPLEX_FULL      = 1,
    CA_DUPLEX_INVALID   = 0xFF,
} ca_duplex_t;

typedef enum {
    AAL_PORT_UNI0       = 0x0,
    AAL_PORT_PON        = 0x1,
    AAL_PORT_RESERVE    = 0x2,
    AAL_PORT_BLACKHOLE  = 0x3,
    AAL_PORT_CPU        = 0x4,
    AAL_PORT_UNI1       = 0x5,
    AAL_PORT_DMA        = 0x6,
    AAL_PORT_FE     = 0x7,
    AAL_PORT_END        = 0x8,
    AAL_PORT_INVALID    = 0xff,
} aal_port_t;

typedef enum {
    AAL_FE_GE_ID        = 0,
    AAL_FE_PON_ID       = 1,
    AAL_FE_MA_ID        = 2,
    AAL_FE_INVALID      = 0xff,
} ca_aal_fe_instance_t;


#endif /* !__ASSEMBLER__ */

#endif /* __OSAL_TYPE_H__ */

