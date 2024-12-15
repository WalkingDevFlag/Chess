
// search_full.h

#ifndef SEARCH_FULL_H
#define SEARCH_FULL_H

// includes

#include "board.h"
#include "util.h"

// functions

extern void search_full_init (list_t * list, board_t * board, int ThreadId);
extern int  search_full_root (list_t * list, board_t * board, int a, int b, int depth, int search_type, int ThreadId);

//extern bool egbb_is_loaded;

#endif // !defined SEARCH_FULL_H

// end of search_full.h

