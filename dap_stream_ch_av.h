#ifndef _CH_MCOD_H_
#define _CH_MCOD_H_

/// Multimedia Content-On-Dement - MCOD module


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// seconds



#include "media_mcod.h"

extern int ch_mcod_init();
extern void ch_mcod_deinit();
extern void ch_mcod_check_and_step(ch_mcod_t * mcod);
extern void ch_mcod_set_state(ch_mcod_t * mcod, ch_mcod_state_t st);
extern const char* ch_mcod_state_str(ch_mcod_state_t st);
#endif
