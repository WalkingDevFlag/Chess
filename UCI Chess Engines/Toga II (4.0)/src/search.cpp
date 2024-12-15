
// search.cpp

// includes

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#endif


#include <csetjmp>
#include <cstring>

#include "attack.h"
#include "board.h"
#include "book.h"
#include "colour.h"
#include "list.h"
#include "material.h"
#include "move.h"
#include "move_do.h"
#include "move_gen.h"
#include "option.h"
#include "pawn.h"
#include "protocol.h"
#include "pv.h"
#include "search.h"
#include "search_full.h"
#include "sort.h"
#include "trans.h"
#include "util.h"
#include "value.h"

// constants

static const bool UseCpuTime = false; // false
static const bool UseEvent = true; // true

static const bool UseShortSearch = true;
static const int ShortSearchDepth = 1;

static const bool DispBest = true; // true
static const bool DispDepthStart = true; // true
static const bool DispDepthEnd = true; // true
static const bool DispRoot = true; // true
static const bool DispStat = true; // true

static const bool UseEasy = true; // singular move
static const int EasyThreshold = 150;
static const double EasyRatio = 0.20;

static const bool UseEarly = true; // early iteration end
static const double EarlyRatio = 0.60;

static const bool UseBad = true;
static const int BadThreshold = 50; // 50
static const bool UseExtension = true;

// variables

static search_multipv_t save_multipv[MultiPVMax];
#ifdef _WIN32
static HANDLE threat_handle[MaxThreads];
#else
static pthread_t threat_handle[MaxThreads];
static my_sem_t thread_runnable[MaxThreads];
#endif

int NumberThreads = 1;

//bool trans_endgame;
//CRITICAL_SECTION CriticalSection; 

search_input_t SearchInput[1];
search_info_t SearchInfo[MaxThreads][1];
search_root_t SearchRoot[MaxThreads][1];
search_current_t SearchCurrent[MaxThreads][1];
search_best_t SearchBest[MaxThreads][MultiPVMax];

// prototypes

static void search_send_stat (int ThreadId);
#ifdef _WIN32
unsigned __stdcall search_thread (void *param);
#else
void * search_thread (void *param);
#endif

// functions

// depth_is_ok()

bool depth_is_ok(int depth) {

   return depth > -128 && depth < DepthMax;
}

// height_is_ok()

bool height_is_ok(int height) {

   return height >= 0 && height < HeightMax;
}

// search_clear()

void search_clear() {

	int ThreadId;

   // SearchInput

   SearchInput->infinite = false;
   SearchInput->depth_is_limited = false;
   SearchInput->depth_limit = 0;
   SearchInput->time_is_limited = false;
   SearchInput->time_limit_1 = 0.0;
   SearchInput->time_limit_2 = 0.0;

   // SearchInfo

	for (ThreadId = 0; ThreadId < NumberThreads; ThreadId++){
		SearchInfo[ThreadId]->can_stop = false;
		SearchInfo[ThreadId]->stop = false;
		SearchInfo[ThreadId]->stopped = false;
		SearchInfo[ThreadId]->exited = false;
		SearchInfo[ThreadId]->check_nb = 10000; // was 100000
		SearchInfo[ThreadId]->check_inc = 10000; // was 100000
		SearchInfo[ThreadId]->last_time = 0.0;

		// SearchBest

		SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].move = MoveNone;
		SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].value = 0;
		SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].flags = SearchUnknown;
		SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].depth = 0;
		PV_CLEAR(SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].pv);

		// SearchRoot

		SearchRoot[ThreadId]->depth = 0;
		SearchRoot[ThreadId]->move = MoveNone;
		SearchRoot[ThreadId]->move_pos = 0;
		SearchRoot[ThreadId]->move_nb = 0;
		SearchRoot[ThreadId]->last_value = 0;
		SearchRoot[ThreadId]->bad_1 = false;
		SearchRoot[ThreadId]->bad_2 = false;
		SearchRoot[ThreadId]->change = false;
		SearchRoot[ThreadId]->easy = false;
		SearchRoot[ThreadId]->flag = false;

		// SearchCurrent

		SearchCurrent[ThreadId]->max_depth = 0;
		SearchCurrent[ThreadId]->node_nb = 0;
		SearchCurrent[ThreadId]->time = 0.0;
		SearchCurrent[ThreadId]->speed = 0.0;
		SearchCurrent[ThreadId]->cpu = 0.0;
	}
}
#ifdef _WIN32
void start_suspend_threads(){

   static int ThreadIds[MaxThreads];
   static unsigned internalThreadIds[MaxThreads];
   int i;

   // start and suspend threads

   for (i = 1; i < NumberThreads; i++){
	    ThreadIds[i-1] = i;
		threat_handle[i-1] = (HANDLE) _beginthreadex( NULL, 0, &search_thread, &ThreadIds[i-1], CREATE_SUSPENDED, &internalThreadIds[i-1]);
   }

}
void resume_threads(){

   int i;

   // resume threads

   for (i = 1; i < NumberThreads; i++){
	    ResumeThread(threat_handle[i-1]);
   }
}
#else
void start_suspend_threads(){

   static int ThreadIds[MaxThreads];
   int i;

   // start and suspend threads

   for (i = 1; i < NumberThreads; i++){
	    ThreadIds[i-1] = i;
        my_sem_init(&(thread_runnable[i-1]),0);
	    pthread_create(&(threat_handle[i-1]),NULL,
			   search_thread,&(ThreadIds[i-1]));
   }

}
void resume_threads(){

   int i;
   // resume threads
   for (i = 1; i < NumberThreads; i++){
       my_sem_post(&(thread_runnable[i-1]));
   }
}
#endif

// exit_threads() 

void exit_threads() {
  int ThreadId;
  bool all_exited=false;
  SearchInput->exit_engine=true; // hack
  resume_threads();
  while (!all_exited){
    all_exited = true;
    for (ThreadId = 1; ThreadId < NumberThreads; ThreadId++){
      if (!SearchInfo[ThreadId]->exited)
	all_exited = false;
    }
  }
  SearchInput->exit_engine=false; // hack
}


// search()

void search() {

   int move;
   int i;
   bool all_stopped;
   int ThreadId; 
           
   for (i = 0; i < MultiPVMax; i++){
	  save_multipv[i].mate = 0;
	  save_multipv[i].depth = 0;
	  save_multipv[i].max_depth = 0;
	  save_multipv[i].value = 0;
	  save_multipv[i].time = 0;
	  save_multipv[i].node_nb = 0;
	  strcpy(save_multipv[i].pv_string,""); 
   }
  
   SearchInput->multipv = option_get_int("MultiPV")-1;

	for (ThreadId = 0; ThreadId < NumberThreads; ThreadId++){
		SearchCurrent[ThreadId]->multipv = 0;
	}
   
   
   ASSERT(board_is_ok(SearchInput->board));

   // opening book

   if (option_get_bool("OwnBook") && !SearchInput->infinite) {

      move = book_move(SearchInput->board);

      if (move != MoveNone) {

         // play book move

         SearchBest[0][SearchCurrent[ThreadId]->multipv].move = move;
         SearchBest[0][SearchCurrent[ThreadId]->multipv].value = 1;
         SearchBest[0][SearchCurrent[ThreadId]->multipv].flags = SearchExact;
         SearchBest[0][SearchCurrent[ThreadId]->multipv].depth = 1;
         SearchBest[0][SearchCurrent[ThreadId]->multipv].pv[0] = move;
         SearchBest[0][SearchCurrent[ThreadId]->multipv].pv[1] = MoveNone;

         search_update_best(0);

         return;
      }
   }

   // SearchInput

   gen_legal_moves(SearchInput->list,SearchInput->board);

   if (LIST_SIZE(SearchInput->list) < SearchInput->multipv+1){ 
	  SearchInput->multipv = LIST_SIZE(SearchInput->list)-1;
   }

   if (LIST_SIZE(SearchInput->list) <= 1) {
      SearchInput->depth_is_limited = true;
      SearchInput->depth_limit = 4; // was 1
   }

   trans_inc_date(Trans);

   // resume threads

   resume_threads();

   //SearchInfo_smp->stop = true;
   search_smp(0);
   for (ThreadId = 1; ThreadId < NumberThreads; ThreadId++){ // stop threads
		SearchInfo[ThreadId]->stop = true;
   }

   all_stopped = false;
   while (!all_stopped){
	   all_stopped = true;
	   for (ThreadId = 1; ThreadId < NumberThreads; ThreadId++){
			if (!SearchInfo[ThreadId]->stopped)
				all_stopped = false;
	   }
   }
}

#ifdef _WIN32
unsigned __stdcall search_thread (void *param) {

	int ThreadId = *((int*)param);
	SearchInfo[ThreadId]->exited= false;

	while (!SearchInput->exit_engine){
		search_smp(ThreadId);
		SearchInfo[ThreadId]->stopped = true;
		SuspendThread(threat_handle[ThreadId-1]);
	}
	SearchInfo[ThreadId]->exited= true;
	_endthreadex( 0 );
	return 0;
}
#else
void * search_thread (void *param) {

	int ThreadId = *((int*)param);
	SearchInfo[ThreadId]->exited= false;
	my_sem_wait(&(thread_runnable[ThreadId-1]));
	while (!SearchInput->exit_engine){
	  search_smp(ThreadId);
	  SearchInfo[ThreadId]->stopped = true;
	  my_sem_wait(&(thread_runnable[ThreadId-1]));
	}
	SearchInfo[ThreadId]->exited= true;
	return NULL;
}
#endif

// search_smp()

void search_smp(int ThreadId) {

   int depth;
   int i;
   int delta, alpha, beta;
   int last_move;
   bool search_ready;
   sint64 node_nb;
   double speed;
        
   // SearchInfo

   if (setjmp(SearchInfo[ThreadId]->buf) != 0) {
      ASSERT(SearchInfo[ThreadId]->can_stop);
      ASSERT(SearchBest[ThreadId]->move!=MoveNone);
      search_update_current(ThreadId);
      return;
   }

   // SearchRoot

   list_copy(SearchRoot[ThreadId]->list,SearchInput->list);

   // SearchCurrent

   board_copy(SearchCurrent[ThreadId]->board,SearchInput->board);
   my_timer_reset(SearchCurrent[ThreadId]->timer);
   my_timer_start(SearchCurrent[ThreadId]->timer);

   // init

   sort_init(ThreadId);
   search_full_init(SearchRoot[ThreadId]->list,SearchCurrent[ThreadId]->board,ThreadId);
   last_move = MoveNone;

   // analyze game for evaluation
   
   /*if (SearchCurrent[ThreadId]->board->piece_size[White] < 3 
	   && SearchCurrent[ThreadId]->board->piece_size[Black] < 3
	   && (SearchCurrent[ThreadId]->board->pawn_size[White]+SearchCurrent[ThreadId]->board->pawn_size[Black]) > 0){
	   trans_endgame = true;
   }
   else{
	   trans_endgame = false;
   } */

   // iterative deepening

   search_ready = false;
   

   if (ThreadId == 0){ // main thread
	   for (depth = 1; depth < DepthMax; depth++) {
	   	   delta = 16; 
		   for (SearchCurrent[ThreadId]->multipv = 0; SearchCurrent[ThreadId]->multipv <= SearchInput->multipv; SearchCurrent[ThreadId]->multipv++){

			  if (DispDepthStart && SearchCurrent[ThreadId]->multipv == 0) send("info depth %d",depth);

			  SearchRoot[ThreadId]->bad_1 = false;
			  SearchRoot[ThreadId]->change = false;

			  board_copy(SearchCurrent[ThreadId]->board,SearchInput->board);
			  
			  // Aspiration windows (JD)
			  
			  if (depth <= 4){	// TODO: Try other values	  
		         alpha = -ValueInf;
                 beta = +ValueInf;          
              } 
          
              while (1){

    		    if (UseShortSearch && depth <= ShortSearchDepth) {
    		  	   search_full_root(SearchRoot[ThreadId]->list,SearchCurrent[ThreadId]->board,alpha,beta,depth,SearchShort,ThreadId);
    		    } else {
    			   search_full_root(SearchRoot[ThreadId]->list,SearchCurrent[ThreadId]->board,alpha,beta,depth,SearchNormal,ThreadId);
    		    }
    		    
    		    // Aspiration windows
    		    
    		    // break on mate value
    		    if (value_is_mate(SearchBest[ThreadId]->value)) break;
    		    
    		    // adjust on fail high/low
    		    if (SearchBest[ThreadId]->value <= alpha){
    		    	beta = (alpha+beta)/2;
                    alpha = SearchBest[ThreadId]->value-delta;
                    delta += delta/4 + 5; // Stockfish
                    
                     // fail high depth skipping idea; doesn't work well for SMP so disable then
                } else if (SearchBest[ThreadId]->value >= beta && (SearchInput->multipv >= 1 || NumberThreads != 1 || last_move != SearchBest[ThreadId]->move)){
                    beta = SearchBest[ThreadId]->value+delta;
                    delta += delta/4 + 5;
                } else {
                    alpha = SearchBest[ThreadId]->value-delta;
                    beta = SearchBest[ThreadId]->value+delta; 
                    break;
                }    
              }

              last_move = SearchBest[ThreadId]->move;
			  search_update_current(ThreadId);

			  node_nb = 0;
			  speed = 0;
			  
			  // calculate nodes/speed
			  for (i = 0; i < NumberThreads; i++){
				node_nb += SearchCurrent[i]->node_nb;
				speed += SearchCurrent[i]->speed;
			  }

			  if (DispDepthEnd && SearchCurrent[ThreadId]->multipv == SearchInput->multipv) {
				 send("info depth %d seldepth %d time %.0f nodes " S64_FORMAT " nps %.0f",depth,SearchCurrent[ThreadId]->max_depth,SearchCurrent[ThreadId]->time*1000.0,node_nb,speed);
			  }

			  // update search info

			  if (depth >= 1) SearchInfo[ThreadId]->can_stop = true;

			  if (depth == 1
			   && LIST_SIZE(SearchRoot[ThreadId]->list) >= 2
			   && LIST_VALUE(SearchRoot[ThreadId]->list,0) >= LIST_VALUE(SearchRoot[ThreadId]->list,1) + EasyThreshold) {
				 SearchRoot[ThreadId]->easy = true;
			  }

			  if (UseBad && depth > 1) {
				 SearchRoot[ThreadId]->bad_2 = SearchRoot[ThreadId]->bad_1;
				 SearchRoot[ThreadId]->bad_1 = false;
				 ASSERT(SearchRoot[ThreadId]->bad_2==(SearchBest[ThreadId]->value<=SearchRoot[ThreadId]->last_value-BadThreshold));
			  }

			  SearchRoot[ThreadId]->last_value = SearchBest[ThreadId][0].value;

			  // stop search?

			  if (SearchInput->depth_is_limited && SearchCurrent[ThreadId]->multipv >= SearchInput->multipv
			   && depth >= SearchInput->depth_limit) {
				 SearchRoot[ThreadId]->flag = true;
			  }

			  if (SearchInput->time_is_limited
			   && SearchCurrent[ThreadId]->time >= SearchInput->time_limit_1
			   && !SearchRoot[ThreadId]->bad_2) {
				 SearchRoot[ThreadId]->flag = true;
			  }

			  if (UseEasy
			   && SearchInput->time_is_limited
			   && SearchCurrent[ThreadId]->time >= SearchInput->time_limit_1 * EasyRatio
			   && SearchRoot[ThreadId]->easy) {
				 ASSERT(!SearchRoot[ThreadId]->bad_2);
				 ASSERT(!SearchRoot[ThreadId]->change);
				 SearchRoot[ThreadId]->flag = true;
			  }

			  if (UseEarly
			   && SearchInput->time_is_limited
			   && SearchCurrent[ThreadId]->time >= SearchInput->time_limit_1 * EarlyRatio
			   && !SearchRoot[ThreadId]->bad_2
			   && !SearchRoot[ThreadId]->change) {
				 SearchRoot[ThreadId]->flag = true;
			  }

			  if (SearchInfo[ThreadId]->can_stop 
			   && (SearchInfo[ThreadId]->stop || (SearchRoot[ThreadId]->flag && !SearchInput->infinite))) {
				  search_ready = true;
				  break;
			  }
		   }
		   if (search_ready)
			   break;
	   }
   }
   else {
	   for (depth = 1; depth < DepthMax; depth++) {
	   	  delta = 16; 
		  SearchInfo[ThreadId]->can_stop = true;
		  
		  board_copy(SearchCurrent[ThreadId]->board,SearchInput->board);

		  // Aspiration windows
		  if (depth <= 4){	// Try other values	  
	         alpha = -ValueInf;
             beta = +ValueInf;          
          } 
      
          while (1){

		    if (UseShortSearch && depth <= ShortSearchDepth) {
		  	   search_full_root(SearchRoot[ThreadId]->list,SearchCurrent[ThreadId]->board,alpha,beta,depth,SearchShort,ThreadId);
		    } else {
			   search_full_root(SearchRoot[ThreadId]->list,SearchCurrent[ThreadId]->board,alpha,beta,depth,SearchNormal,ThreadId);
		    }
		    
		    if (value_is_mate(SearchBest[ThreadId]->value)) break;
		    
		    if (SearchBest[ThreadId]->value <= alpha){
		    	beta = (alpha+beta)/2; // Stockfish
                alpha = SearchBest[ThreadId]->value-delta;
                delta += delta/4 + 5;
            } else if (SearchBest[ThreadId]->value >= beta /* && last_move != SearchBest[ThreadId]->move*/){
                beta = SearchBest[ThreadId]->value+delta;
                delta += delta/4 + 5;
            } else {
                alpha = SearchBest[ThreadId]->value-delta;
                beta = SearchBest[ThreadId]->value+delta; 
                break;
            }    
          }
          
          last_move = SearchBest[ThreadId]->move;
		  search_update_current(ThreadId);

	   }
   }
}

// search_update_best()

void search_update_best(int ThreadId) {

   int move, value, flags, depth, max_depth;
   const mv_t * pv;
   double time;
   sint64 node_nb;
   int mate, i, z;
   bool found;
   char move_string[256], pv_string[512];
      
   search_update_current(ThreadId);
#ifdef _WIN32
   EnterCriticalSection(&CriticalSection); 
#else
   pthread_mutex_lock(&CriticalSection);
#endif

/*   if (DispBest && 
      (save_multipv[0].depth < SearchBest[ThreadId][0].depth || 
           (save_multipv[0].depth == SearchBest[ThreadId][0].depth && 
           save_multipv[0].value < SearchBest[ThreadId][0].value))) {*/ 

     if (ThreadId == 0) {  // Norman Schmidt (kranium): multi-pv fix

      move = SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].move;
      value = SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].value;
      flags = SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].flags;
      depth = SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].depth;
      pv = SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].pv;

      max_depth = SearchCurrent[ThreadId]->max_depth;
      time = SearchCurrent[ThreadId]->time;
      node_nb = SearchCurrent[ThreadId]->node_nb;

      move_to_string(move,move_string,256);
      pv_to_string(pv,pv_string,512);

	  mate = value_to_mate(value);

	  if (SearchCurrent[ThreadId]->multipv == 0){
		  save_multipv[SearchCurrent[ThreadId]->multipv].mate = mate;
		  save_multipv[SearchCurrent[ThreadId]->multipv].depth = depth;
		  save_multipv[SearchCurrent[ThreadId]->multipv].max_depth = max_depth;
		  save_multipv[SearchCurrent[ThreadId]->multipv].value = value;
		  save_multipv[SearchCurrent[ThreadId]->multipv].time = time*1000.0;
		  save_multipv[SearchCurrent[ThreadId]->multipv].node_nb = node_nb;
		  strcpy(save_multipv[SearchCurrent[ThreadId]->multipv].pv_string,pv_string); 
	  }
	  else{
		  found = false;
		  for (i = 0; i < SearchCurrent[ThreadId]->multipv; i++){
			  if (save_multipv[i].value < value){
				  found = true;
				  break;
			  }
		  }
		  if (found){

			  for (z = SearchCurrent[ThreadId]->multipv; z > i; z--){
				  save_multipv[z].mate = save_multipv[z-1].mate;
				  save_multipv[z].depth = save_multipv[z-1].depth;
				  save_multipv[z].max_depth = save_multipv[z-1].max_depth;
				  save_multipv[z].value = save_multipv[z-1].value;
				  save_multipv[z].time = save_multipv[z-1].time;
				  save_multipv[z].node_nb = save_multipv[z-1].node_nb;
				  strcpy(save_multipv[z].pv_string,save_multipv[z-1].pv_string); 
			  }
			  
			  save_multipv[i].mate = mate;
		      save_multipv[i].depth = depth;
		      save_multipv[i].max_depth = max_depth;
		      save_multipv[i].value = value;
		      save_multipv[i].time = time*1000.0;
		      save_multipv[i].node_nb = node_nb;
		      strcpy(save_multipv[i].pv_string,pv_string); 
			  
		  }
		  else{
			  save_multipv[SearchCurrent[ThreadId]->multipv].mate = mate;
			  save_multipv[SearchCurrent[ThreadId]->multipv].depth = depth;
			  save_multipv[SearchCurrent[ThreadId]->multipv].max_depth = max_depth;
			  save_multipv[SearchCurrent[ThreadId]->multipv].value = value;
			  save_multipv[SearchCurrent[ThreadId]->multipv].time = time*1000.0;
			  save_multipv[SearchCurrent[ThreadId]->multipv].node_nb = node_nb;
			  strcpy(save_multipv[SearchCurrent[ThreadId]->multipv].pv_string,pv_string); 
		  }
	  }
	  
      if (depth > 1 || (depth == 1 && SearchCurrent[ThreadId]->multipv == SearchInput->multipv)){
		  for (i = 0; i <= SearchInput->multipv; i++){

			  if (save_multipv[i].mate == 0) {

				 // normal evaluation

			  if (false) {
				 } else if (flags == SearchExact) {
					send("info multipv %d depth %d seldepth %d score cp %d time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].value,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
				 } else if (flags == SearchLower) {
					send("info multipv %d depth %d seldepth %d score cp %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].value,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
				 } else if (flags == SearchUpper) {
					send("info multipv %d depth %d seldepth %d score cp %d upperbound time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].value,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
				 }

			  } else {

				 // mate announcement

				 if (false) {
				 } else if (flags == SearchExact) {
					send("info multipv %d depth %d seldepth %d score mate %d time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].mate,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
				 } else if (flags == SearchLower) {
					send("info multipv %d depth %d seldepth %d score mate %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].mate,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
				 } else if (flags == SearchUpper) {
					send("info multipv %d depth %d seldepth %d score mate %d upperbound time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].mate,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
				 }
			  }
		  }
	  }
   }

   // update time-management info

/*   danger = false;
   for (i = 1; i < NumberThreads; i++){
	   if (SearchBest[0][0].depth < SearchBest[i][0].depth ||
		     (SearchBest[0][0].depth == SearchBest[i][0].depth &&
			  SearchBest[0][0].value < SearchBest[i][0].value)){
		   danger = true;
	   }
   }*/

   if (UseBad && ThreadId == 0 && SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].depth > 1) {
      if (SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].value <= SearchRoot[ThreadId]->last_value - BadThreshold) {
         SearchRoot[ThreadId]->bad_1 = true;
         SearchRoot[ThreadId]->easy = false;
         SearchRoot[ThreadId]->flag = false;
      } else {
         SearchRoot[ThreadId]->bad_1 = false;
      }
   }
#ifdef _WIN32
   LeaveCriticalSection(&CriticalSection);
#else
   pthread_mutex_unlock(&CriticalSection);
#endif
}

// search_update_root()

void search_update_root(int ThreadId) {

   int move, move_pos, move_nb;
   double time;
   sint64 node_nb;
   char move_string[256];

   if (DispRoot && ThreadId == 0) {

      search_update_current(ThreadId);

      if (SearchCurrent[ThreadId]->time >= 1.0) {

         move = SearchRoot[ThreadId]->move;
         move_pos = SearchRoot[ThreadId]->move_pos;
         move_nb = SearchRoot[ThreadId]->move_nb;

         time = SearchCurrent[ThreadId]->time;
         node_nb = SearchCurrent[ThreadId]->node_nb;

         move_to_string(move,move_string,256);

         send("info currmove %s currmovenumber %d",move_string,move_pos+1);
      }
   }
}

// search_update_current()

void search_update_current(int ThreadId) {

   my_timer_t *timer;
   sint64 node_nb;
   double time, speed, cpu;
	
   timer = SearchCurrent[ThreadId]->timer;

   node_nb = SearchCurrent[ThreadId]->node_nb;
   time = (UseCpuTime) ? my_timer_elapsed_cpu(timer) : my_timer_elapsed_real(timer);
   speed = (time >= 1.0) ? double(node_nb) / time : 0.0;
   cpu = my_timer_cpu_usage(timer);

   SearchCurrent[ThreadId]->time = time;
   SearchCurrent[ThreadId]->speed = speed;
   SearchCurrent[ThreadId]->cpu = cpu;
}

// search_check()

void search_check(int ThreadId) {

	if (ThreadId == 0){
		search_send_stat(ThreadId);

	   if (UseEvent) event();

	   if (SearchInput->depth_is_limited
		&& SearchRoot[ThreadId]->depth > SearchInput->depth_limit) {
		  SearchRoot[ThreadId]->flag = true;
	   }

	   if (SearchInput->time_is_limited
		&& SearchCurrent[ThreadId]->time >= SearchInput->time_limit_2) {
		  SearchRoot[ThreadId]->flag = true;
	   }

	   if (SearchInput->time_is_limited
		&& SearchCurrent[ThreadId]->time >= SearchInput->time_limit_1
		&& !SearchRoot[ThreadId]->bad_1
		&& !SearchRoot[ThreadId]->bad_2
		&& (!UseExtension || SearchRoot[ThreadId]->move_pos == 0)) {
		  SearchRoot[ThreadId]->flag = true;
	   }

	   if (SearchInfo[ThreadId]->can_stop 
		&& (SearchInfo[ThreadId]->stop || (SearchRoot[ThreadId]->flag && !SearchInput->infinite))) {
		  longjmp(SearchInfo[ThreadId]->buf,1);
	   }
	}
	else{
		if (SearchInfo[ThreadId]->stop){
		  longjmp(SearchInfo[ThreadId]->buf,1);
	   }
	}
}

// search_send_stat()

static void search_send_stat(int ThreadId) {

   double time, speed, cpu;
   sint64 node_nb;
   int i;

   search_update_current(ThreadId);

   if (DispStat && ThreadId == 0 && SearchCurrent[ThreadId]->time >= SearchInfo[ThreadId]->last_time + 1.0) { // at least one-second gap

      SearchInfo[ThreadId]->last_time = SearchCurrent[ThreadId]->time;

      time = SearchCurrent[ThreadId]->time;
      speed = SearchCurrent[ThreadId]->speed;
      cpu = SearchCurrent[ThreadId]->cpu;
	  node_nb = 0;
	  speed = 0;
	  for (i = 0; i < NumberThreads; i++){
		node_nb += SearchCurrent[ThreadId]->node_nb;
		speed += SearchCurrent[ThreadId]->speed;
	  }

      send("info time %.0f nodes " S64_FORMAT " nps %.0f cpuload %.0f",time*1000.0,node_nb,speed,cpu*1000.0);

      trans_stats(Trans);
   }
}

// end of search.cpp

