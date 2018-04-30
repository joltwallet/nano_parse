#ifndef __INCLUDE_NANO_PARSE_H__
#define __INCLUDE_NANO_PARSE_H__

#ifdef ESP32
#include "nano_lib.h"
#else
#include "../components/nano_lib/include/nano_lib.h"
#endif

int get_block_count();
int get_work(nl_block_t *block);
int get_head(nl_block_t *block);

#endif
