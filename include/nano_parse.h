#ifndef __INCLUDE_NANO_PARSE_H__
#define __INCLUDE_NANO_PARSE_H__

//#ifdef ESP32
#include "nano_lib.h"
//#else
//#include "../components/nano_lib/include/nano_lib.h"
//#endif

int get_block_count();
int get_work(char *hash, char *work);
int get_head(nl_block_t *block);
int get_block(char *block_hash, nl_block_t *block);
int get_pending(char *account_address, char *pending_block);
int get_frontier(char *account_address, char *frontier_block_hash);
int process_block(nl_block_t *block);

#endif
