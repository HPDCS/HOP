/**
 * Hardware Online Profiler (HOP) module
 */

#ifndef HOP_MOD_H
#define HOP_MOD_H

// #define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define HOP_MODULE_NAME		KBUILD_MODNAME
#define HOP_DEVICE_NAME		"ctl"

#define HOP_MAJOR		0
#define HOP_MIN_DEVICES		0
#define HOP_MAX_DEVICES		32	// Max Available CPU

void threads_monitor_hook(void);

#endif /* HOP_MOD_H */
