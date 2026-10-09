#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#include "board_config.h"

#if(boardDISPLAY)

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "main.h"

/* 跳过lv_conf.h，使用Kconfig配置 */
#ifndef LV_CONF_SKIP
#define LV_CONF_SKIP 1
#endif
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Initialize input device driver */
void lv_port_indev_init(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif  //boardDISPLAY

#endif  //LV_PORT_INDEV_H
