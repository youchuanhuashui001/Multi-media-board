#ifndef __COMMON_H_
#define __COMMON_H_

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl.h"
#include "lvgl/demos/lv_demos.h"

#include "src/lib/driver_backends.h"
#include "src/lib/simulator_util.h"
#include "src/lib/simulator_settings.h"

#include "view_manager.h"

#ifdef VIEW_DEBUG_ON
#define view_printf(fmt, args...)   printf("VIEW_DEBUG: " fmt, ## args)
#else
#define view_printf(fmt, args...) do {} while (0)
#endif

#ifdef MAIN_VIEW_DEBUG_ON
#define main_view_printf(fmt, args...)   printf("MAIN_VIEW_DEBUG: " fmt, ## args)
#else
#define main_view_printf(fmt, args...) do {} while (0)
#endif


#endif