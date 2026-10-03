#ifndef MD_PRINT_QUEUE_TASK_UPDATA_H_
#define MD_PRINT_QUEUE_TASK_UPDATA_H_

#include "board_config.h"

#if(boardPRINT_IFACE)
#include "main.h"
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif

#endif  //boardPRINT_IFACE
#endif  //MD_PRINT_QUEUE_TASK_UPDATA_H_
