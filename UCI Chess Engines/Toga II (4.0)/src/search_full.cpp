// search_full.cpp

// includes

#include <math.h>

#include "pawn.h"
#include "attack.h"
#include "board.h"
#include "colour.h"
#include "eval.h"
#include "list.h"
#include "move.h"
#include "move_check.h"
#include "move_do.h"
#include "option.h"
#include "piece.h"
#include "pst.h"
#include "pv.h"
#include "recog.h"
#include "search.h"
#include "search_full.h"
#include "see.h"
#include "sort.h"
#include "trans.h"
#include "util.h"
#include "value.h"

#define ABS(x) ((x)<0?-(x):(x))
#define MIN(X, Y)  ((X) < (Y) ? (X) : (Y))

// constants and variables

// main search

static const bool UseDistancePruning = true;

// transposition table

static const bool UseTrans = true;
static const int TransDepth = 1;
static const bool UseExact = true;

static const bool UseMateValues = true; // use mate values from shallower searches?

static /* const */ bool UseTransPruning = true;
static /* const */ int earlyTransPruningDepth = 1;
static /* const */ int earlyTransPruningMargin = 100;

// null move

static /* const */ bool UseNull = true;
static /* const */ bool UseNullEval = true; // true
static const int NullDepth = 2; // was 2
static /* const */ int NullReduction = 3;

// move ordering

static const bool UseIID = true;
static const int IIDDepth = 3;
static const int IIDReduction = 2;

// extensions

static const bool ExtendSingleReply = true; // true

// razoring

static /* const */ bool UseRazor = true;
static /* const */ int RazorDepth = 4;
static /* const */ int RazorMargin = 300; 

// eval pruning

static /* const */ bool UseEval = true;
static /* const */ int EvalDepth = 5;
static /* const */ int EvalMargin[7] = {0, 130, 250, 360, 460, 550, 630};

// history pruning

static /* const */ bool UseHistory = true;
static const int HistoryDepth = 3; // was 3
static const int HistoryPVDepth = 3; // was 3
static const int HistoryMoveNb = 3; // was 3
static /* const */ int HistoryValue = 9830; // 60%

// Late Move Reductions (Stockfish/Protector)
static int QuietPVMoveReduction[64][64]; // [depth][played move count][ThreadId]
static int QuietMoveReduction[64][64]; // [depth][played move count][ThreadId] 

static const int MoveCountLimit[6] = {0, 4, 7, 12, 19, 28};

// quiescence search

static /* const */ bool UseDelta = true; // false
static /* const */ int DeltaMargin = 50;

// misc

static const int NodeAll = -1;
static const int NodePV  =  0;
static const int NodeCut = +1;

// macros

#define NODE_OPP(type)     (-(type))
#define DEPTH_MATCH(d1,d2) ((d1)>=(d2))

// prototypes

static int  full_root            (list_t * list, board_t * board, int alpha, int beta, int depth, int height, int search_type, int ThreadId);

static int  full_search          (board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int node_type, bool extended, int ThreadId);

static int  full_quiescence      (board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int ThreadId);

static int  full_new_depth       (int depth, int move, board_t * board, bool single_reply, bool in_pv, int height, bool extended, bool * cap_extended, int ThreadId);

static bool do_null              (const board_t * board);
static bool do_ver               (const board_t * board);

static void pv_fill              (const mv_t pv[], board_t * board);

static bool move_is_dangerous    (int move, const board_t * board);
static bool capture_is_dangerous (int move, const board_t * board);

static bool simple_stalemate     (const board_t * board);

static bool passed_pawn_move     (int move, const board_t * board);
static bool is_passed		     (const board_t * board, int to);

static bool pawn_is_endgame      (int move, const board_t * board);

// functions

// search_full_init()

void search_full_init(list_t * list, board_t * board, int ThreadId) {

   const char * string;
   int trans_move, trans_depth, trans_flags, trans_value;
   int i, j;
   entry_t * found_entry;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));
   
   // trans pruning options
   
   UseTransPruning = option_get_bool("Hash Pruning");
   earlyTransPruningDepth = option_get_int("Hash Pruning Depth");
   earlyTransPruningMargin = option_get_int("Hash Pruning Margin");

   // null-move options

   string = option_get_string("NullMove Pruning");

   if (false) {
   } else if (my_string_equal(string,"Always")) {
      UseNull = true;
      UseNullEval = false;
   } else if (my_string_equal(string,"Fail High")) {
      UseNull = true;
      UseNullEval = true;
   } else if (my_string_equal(string,"Never")) {
      UseNull = false;
      UseNullEval = false;
   } else {
      ASSERT(false);
      UseNull = true;
      UseNullEval = true;
   }

   NullReduction = option_get_int("NullMove Reduction");
   
   // razoring / eval pruning options
   
   UseRazor = option_get_bool("Razoring");
   RazorDepth = option_get_int("Razoring Depth");
   RazorMargin = option_get_int("Razoring Margin");
   
   UseEval = option_get_bool("Evaluation Pruning");
   EvalDepth = option_get_int("Evaluation Pruning Depth");

   // history-pruning options

   UseHistory = option_get_bool("History Pruning");
   HistoryValue = (option_get_int("History Threshold") * 16384 + 50) / 100;

   
   // Late Move Reductions (idea from Stockfish): about 20 elo improvement
   for (i = 0; i < 64; i++){   
      for (j = 0; j < 64; j++){
         if (i == 0 || j == 0){
            QuietPVMoveReduction[i][j] = QuietMoveReduction[i][j] = 0;
         } else {
            double PVReduction = log((double) (i)) * log((double) (j)) / (3.0);
            double NonPVReduction = (1.0 + log((double) (i))) * log((double) (j)) / (2.5);
            QuietPVMoveReduction[i][j]  = (int) (PVReduction >= 1.0 ? floor(PVReduction) : 0);
            QuietMoveReduction[i][j]  = (int) (NonPVReduction >= 1.0 ? floor(NonPVReduction) : 0);  
		 }
      }
   }

   // delta-pruning options

   UseDelta = option_get_bool("Delta Pruning");
   DeltaMargin = option_get_int("Delta Margin");

   // quiescence-search options

   SearchCurrent[ThreadId]->CheckNb = option_get_int("Quiescence Check Plies");
   SearchCurrent[ThreadId]->CheckDepth = 1 - SearchCurrent[ThreadId]->CheckNb;

   // standard sort

   list_note(list);
   list_sort(list);
   
   // misc
   
   SearchCurrent[ThreadId]->last_move = MoveNone;

   // basic sort

   trans_move = MoveNone;
   if (UseTrans) trans_retrieve(Trans,&found_entry,board->key,&trans_move,&trans_depth,&trans_flags,&trans_value);
   note_moves(list,board,0,trans_move,ThreadId);
   list_sort(list);
}

// search_full_root()

int search_full_root(list_t * list, board_t * board, int a, int b, int depth, int search_type, int ThreadId) {

   int value;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));
   ASSERT(depth_is_ok(depth));
   ASSERT(search_type==SearchNormal||search_type==SearchShort);

   ASSERT(list==SearchRoot[ThreadId]->list);
   ASSERT(!LIST_IS_EMPTY(list));
   ASSERT(board==SearchCurrent[ThreadId]->board);
   ASSERT(board_is_legal(board));
   ASSERT(depth>=1);

   value = full_root(list,board,a,b,depth,0,search_type, ThreadId);

   ASSERT(value_is_ok(value));
   ASSERT(LIST_VALUE(list,0)==value);

   return value;
}

// full_root()

static int full_root(list_t * list, board_t * board, int alpha, int beta, int depth, int height, int search_type, int ThreadId) {

   int old_alpha;
   int value, best_value[MultiPVMax];
   int i, move, j;
   int new_depth;
   undo_t undo[1];
   mv_t new_pv[HeightMax];
   bool found;
   bool cap_extended;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(search_type==SearchNormal||search_type==SearchShort);

   ASSERT(list==SearchRoot[ThreadId]->list);
   ASSERT(!LIST_IS_EMPTY(list));
   ASSERT(board==SearchCurrent[ThreadId]->board);
   ASSERT(board_is_legal(board));
   ASSERT(depth>=1);

   // init

   SearchCurrent[ThreadId]->node_nb++;
   SearchInfo[ThreadId]->check_nb--;
   SearchCurrent[ThreadId]->trans_reduction = false;
   SearchCurrent[ThreadId]->do_nullmove = true;

   if (SearchCurrent[ThreadId]->multipv == 0)
	  for (i = 0; i < LIST_SIZE(list); i++) list->value[i] = ValueNone;

   old_alpha = alpha;
   best_value[SearchCurrent[ThreadId]->multipv] = ValueNone;

   // move loop

   for (i = 0; i < LIST_SIZE(list); i++) {

      move = LIST_MOVE(list,i);

	  if (SearchCurrent[ThreadId]->multipv > 0){
		  found = false;
		  for (j = 0; j < SearchCurrent[ThreadId]->multipv; j++){
			  if (SearchBest[ThreadId][j].pv[0] == move){
				  found = true;
				  break;
			  }
		  }
		  if (found == true)
				continue;
	  }
	  
      SearchRoot[ThreadId]->depth = depth;
      SearchRoot[ThreadId]->move = move;
      SearchRoot[ThreadId]->move_pos = i;
      SearchRoot[ThreadId]->move_nb = LIST_SIZE(list);

      search_update_root(ThreadId);

      new_depth = full_new_depth(depth,move,board,board_is_check(board)&&LIST_SIZE(list)==1,true, height, false, &cap_extended,ThreadId);
	  //new_depth1 = full_new_depth(depth,move,board,board_is_check(board)&&LIST_SIZE(list)==1,false, height, ThreadId);

      move_do(board,move,undo);
      
      // search move. Changed by Jerry Donald to use aspiration windows (no Inf window researches needed)
      if (search_type == SearchShort || best_value[SearchCurrent[ThreadId]->multipv] == ValueNone) { // first move
		  value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NodePV,cap_extended,ThreadId);
      } else { // other moves
         value = -full_search(board,-alpha-1,-alpha,new_depth,height+1,new_pv,NodeCut,cap_extended,ThreadId);
         if (value > alpha) { // && value < beta
            SearchRoot[ThreadId]->change = true;
            SearchRoot[ThreadId]->easy = false;
            SearchRoot[ThreadId]->flag = false;
            search_update_root(ThreadId);
			value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NodePV,cap_extended,ThreadId);
         }
      }

      move_undo(board,move,undo);

      if (value <= alpha) { // upper bound
         list->value[i] = old_alpha;
      } else if (value >= beta) { // lower bound
         list->value[i] = beta;
      } else { // alpha < value < beta => exact value
         list->value[i] = value;
      }

      if (value > best_value[SearchCurrent[ThreadId]->multipv] && (best_value[SearchCurrent[ThreadId]->multipv] == ValueNone || value > alpha)) {

         SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].move = move;
		 SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].value = value;
         if (value <= alpha) { // upper bound
            SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].flags = SearchUpper;
         } else if (value >= beta) { // lower bound
            SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].flags = SearchLower;
         } else { // alpha < value < beta => exact value
            SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].flags = SearchExact;
         }
         SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].depth = depth;
         pv_cat(SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].pv,new_pv,move);

         search_update_best(ThreadId);
      }

      if (value > best_value[SearchCurrent[ThreadId]->multipv]) {
         best_value[SearchCurrent[ThreadId]->multipv] = value;
         if (value > alpha) {
            if (search_type == SearchNormal) alpha = value;
            if (value >= beta) break;
         }
      }
   }

   ASSERT(value_is_ok(best_value));

   list_sort(list);

   ASSERT(SearchBest[ThreadId]->move==LIST_MOVE(list,0));
   ASSERT(SearchBest[ThreadId]->value==best_value);

   if (UseTrans && best_value[SearchCurrent[ThreadId]->multipv] > old_alpha && best_value[SearchCurrent[ThreadId]->multipv] < beta) {
      pv_fill(SearchBest[ThreadId][SearchCurrent[ThreadId]->multipv].pv,board);
   }

   return best_value[SearchCurrent[ThreadId]->multipv];
}

// full_search()

static int full_search(board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int node_type, bool extended, int ThreadId) {

   bool in_check;
   bool single_reply;
   bool good_cap;
   int trans_move, trans_depth, trans_flags, trans_value;
   int old_alpha;
   int value, best_value;
   int move, best_move;
   int new_depth;
   int played_nb;
   int i;
   int opt_value;
   int futility_margin;
   int newHistoryValue;
   int threshold;
   int reduction;
   int last_move;
   int quiet_move_count;
   int depth_margin;
   bool reduced, cap_extended;
   bool cut_node;
   attack_t attack[1];
   sort_t sort[1];
   undo_t undo[1];
   mv_t new_pv[HeightMax];
   mv_t played[256];
   entry_t * found_entry;
      
   ASSERT(board!=NULL);
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(pv!=NULL);
   ASSERT(node_type==NodePV||node_type==NodeCut||node_type==NodeAll);

   ASSERT(board_is_legal(board));

   // horizon?

   if (depth <= 0){
	   if (node_type == NodePV)
			SearchCurrent[ThreadId]->CheckDepth = 1 - SearchCurrent[ThreadId]->CheckNb - 1;
	   else
			SearchCurrent[ThreadId]->CheckDepth = 1 - SearchCurrent[ThreadId]->CheckNb;
	   return full_quiescence(board,alpha,beta,0,height,pv,ThreadId);
   }

   // init

   SearchCurrent[ThreadId]->node_nb++;
   SearchInfo[ThreadId]->check_nb--;
   PV_CLEAR(pv);

   if (height > SearchCurrent[ThreadId]->max_depth) SearchCurrent[ThreadId]->max_depth = height;

   if (SearchInfo[ThreadId]->check_nb <= 0) {
      SearchInfo[ThreadId]->check_nb += SearchInfo[ThreadId]->check_inc;
      search_check(ThreadId);
   }

   // draw?

   if (board_is_repetition(board)) return ValueDraw;

   if (recog_draw(board,ThreadId)) return ValueDraw;

   // mate-distance pruning

   if (UseDistancePruning) {

      // lower bound

      value = VALUE_MATE(height+2); // does not work if the current position is mate
      if (value > alpha && board_is_mate(board)) value = VALUE_MATE(height);

      if (value > alpha) {
         alpha = value;
         if (value >= beta) return value;
      }

      // upper bound

      value = -VALUE_MATE(height+1);

      if (value < beta) {
         beta = value;
         if (value <= alpha) return value;
      }
   }

   // transposition table

   trans_move = MoveNone;

   if (UseTrans && depth >= TransDepth) {

      	if (trans_retrieve(Trans,&found_entry,board->key,&trans_move,&trans_depth,&trans_flags,&trans_value)) {
          
		  	if (node_type != NodePV /*|| ThreadId > 0*/) {

            	if (UseMateValues) {

            		if (trans_depth < depth) {
				    	if (trans_value < -ValueEvalInf && TRANS_IS_UPPER(trans_flags)) {
					  		trans_depth = depth;
					  		trans_flags = TransUpper;
				    	} else if (trans_value > +ValueEvalInf && TRANS_IS_LOWER(trans_flags)) {
					  		trans_depth = depth;
					  		trans_flags = TransLower;
				    	}
  
                        // hash pruning (about 10 elo self-play)
				    	if (!SearchCurrent[ThreadId]->trans_reduction && NumberThreads == 1){ // hash pruning doesn't work well with shared hash
				   			if (trans_depth+earlyTransPruningDepth >= depth){
				   	   			trans_value = value_from_trans(trans_value,height);
				   	   
				   	   			depth_margin = depth-trans_depth;

				  	 			if ((TRANS_IS_LOWER(trans_flags) && trans_value >= beta + earlyTransPruningMargin*depth_margin)
									|| (TRANS_IS_UPPER(trans_flags) && trans_value <= alpha - earlyTransPruningMargin*depth_margin)) {
									return trans_value;
				  				} 
				  			} else if (trans_depth+earlyTransPruningDepth+1 >= depth){
				   	   			trans_value = value_from_trans(trans_value,height);
				   	   
				   	   			depth_margin = depth-trans_depth-1;
                     
                     			// if last time we returned early from trans
				  	 			if ((TRANS_IS_LOWER(trans_flags) && trans_value >= beta + earlyTransPruningMargin*depth_margin)
									|| (TRANS_IS_UPPER(trans_flags) && trans_value <= alpha - earlyTransPruningMargin*depth_margin)) { 
							
					   				// we do a new search at depth-1
					  				SearchCurrent[ThreadId]->trans_reduction = true;
				       				value = full_search(board,alpha,beta,depth-earlyTransPruningDepth,height,new_pv,node_type,false,ThreadId); 
				       				SearchCurrent[ThreadId]->trans_reduction = false;
				                    
				                    // if the search returns the expected value, we return. if not, go on to full depth search
				                    if (node_type == NodeCut && value >= beta)
				       					return value;
				       				if (node_type == NodeAll && value <= alpha)
				       					return value; 
				       					
				       				// use info from previous search	
				       				trans_move = new_pv[0];
				  				} 
				   			}
						}
			    	}

					if (trans_depth >= depth) {

				   		trans_value = value_from_trans(trans_value,height);

				   		if ((UseExact && TRANS_IS_EXACT(trans_flags))
							|| (TRANS_IS_LOWER(trans_flags) && trans_value >= beta)
							|| (TRANS_IS_UPPER(trans_flags) && trans_value <= alpha)) {
							return trans_value;
				   } 
				}
			}
		}
	  }
   }

   // height limit

   if (height >= HeightMax-1) return eval(board, alpha, beta, ThreadId);

   // more init

   old_alpha = alpha;
   best_value = ValueNone;
   best_move = MoveNone;
   played_nb = 0;
   last_move = SearchCurrent[ThreadId]->last_move;
   
   attack_set(attack,board);
   in_check = ATTACK_IN_CHECK(attack);
   
   // Eval pruning (also known as static null move pruning) - added JD
   if (UseEval && node_type != NodePV && !in_check && depth <= EvalDepth){
   	    
   	    // if hash table tells us we are below beta, don't eval prune
   	    if (!(trans_move != MoveNone && !TRANS_IS_LOWER(trans_flags) && trans_value < beta)){
   	    	
   	    	threshold = beta+EvalMargin[depth];
			value = eval(board,threshold-1, threshold, ThreadId);
			   
   	    	if (value >= threshold && value < ValueEvalInf)
   	    		return value;
   	    }  
   }

   // null-move pruning
   if (!SearchCurrent[ThreadId]->do_nullmove){
   	    SearchCurrent[ThreadId]->do_nullmove = true;
   } 
   
   else if (UseNull 
   && depth >= NullDepth 
   && node_type != NodePV
   && !(trans_move != MoveNone && TRANS_IS_UPPER(trans_flags) && trans_depth >= depth - NullReduction - depth/4 && trans_value < beta)) { 
   // if hash table tells us we are < beta, the null move search probably wastes time

      if (!in_check
       && !value_is_mate(beta)
       && do_null(board)
       && (!UseNullEval || depth <= NullReduction+1 || eval(board,alpha, beta, ThreadId) >= beta)) {

         // null-move search
         
		 new_depth = depth - NullReduction - depth/4; // JD: from Stockfish, 10 elo better		
		 
	     move_do_null(board,undo);	  
		    
	     SearchCurrent[ThreadId]->last_move = MoveNone; // refutation table
	     SearchCurrent[ThreadId]->do_nullmove = false; // no double null move
	     
         value = -full_search(board,-beta,-beta+1,new_depth,height+1,new_pv,NODE_OPP(node_type),false,ThreadId);
         
         SearchCurrent[ThreadId]->do_nullmove = true; 
         
		 move_undo_null(board,undo);

         // pruning

         if (value >= beta) {

            if (value > +ValueEvalInf) value = +ValueEvalInf; // do not return unproven mates
            ASSERT(!value_is_mate(value));

            best_move = MoveNone;
            best_value = value;
            goto cut;
         }
	  }
   }
    
   // Razoring: idea by Tord Romstad (Glaurung), fixed by Jerry Donald
   
   if (UseRazor && node_type != NodePV && !in_check && trans_move == MoveNone && depth <= RazorDepth){ 
        threshold = alpha - RazorMargin - (depth-1)*39; // Values from Protector (Raimund Heid)
        if (eval(board,threshold-1, threshold, ThreadId) < threshold){
		   value = full_quiescence(board,threshold-1,threshold,0,height,pv,ThreadId); 
           if (value < threshold) return value;
        }
   }


   // Internal Iterative Deepening
   
   if (UseIID && depth >= IIDDepth && node_type == NodePV && trans_move == MoveNone) {

	  new_depth = MIN(depth - IIDReduction,depth/2);
      ASSERT(new_depth>0);
      
      value = full_search(board,alpha,beta,new_depth,height,new_pv,node_type,false,ThreadId);
      if (value <= alpha) value = full_search(board,-ValueInf,beta,new_depth,height,new_pv,node_type,false,ThreadId);

      trans_move = new_pv[0];
   }

   // move generation

   sort_init(sort,board,attack,depth,height,trans_move,last_move,ThreadId);

   single_reply = false;
   if (in_check && LIST_SIZE(sort->list) == 1) single_reply = true; // HACK

   // move loop

   opt_value = +ValueInf;
   good_cap = true;
   quiet_move_count = 0;
   cut_node = (node_type == NodeCut);
   
   while ((move=sort_next(sort,ThreadId)) != MoveNone) {

	  // extensions

      new_depth = full_new_depth(depth,move,board,single_reply,node_type==NodePV, height, extended, &cap_extended, ThreadId);
      
      // history pruning

      value = sort->value; // history score
	  if (!in_check && depth <= 6 && node_type != NodePV 
		  && new_depth < depth && value < 2 * HistoryValue / (depth + depth % 2)
		  && played_nb >= 1+depth && !move_is_dangerous(move,board)){ 
			continue;
	  }

      // quiet move count based pruning (added by Jerry Donald Watson: ~10 elo)

	  if (node_type != NodePV && depth <= 5) {
		  
         if (!in_check && new_depth < depth&& !move_is_tactical(move,board) && !move_is_dangerous(move,board)) {

            ASSERT(!move_is_check(move,board));
            
            if (quiet_move_count >= MoveCountLimit[depth] + NumberThreads-1-ThreadId) continue;     // widen search as # of threads increases       
            quiet_move_count++;

         }
      } 
      
      // Late Move Reductions
	  
	  // init	  
	  reduced = false;
	  reduction = 0;

      // lookup reduction tables     
	  if (UseHistory) {
		 if (!in_check && new_depth < depth && played_nb >= HistoryMoveNb 
			&& depth >= HistoryDepth && !move_is_dangerous(move,board)) {
                         
			if (good_cap && !move_is_tactical(move,board)){
			   good_cap = false;
			}
					
			if (!good_cap){
               reduction = (node_type == NodePV ? QuietPVMoveReduction[depth<64 ? depth: 63][played_nb<64? played_nb: 63]:
                            QuietMoveReduction[depth<64 ? depth: 63][played_nb<64? played_nb: 63]);
                        
		       // reduce bad captures less
			   if (move_is_tactical(move,board)) reduction = reduction / 2; // bad captures
			   else if (cut_node && new_depth - reduction > 1) reduction++;
						
			   // set reduction flag
			   if (reduction > 0)  reduced = true;
            }
		 }
	  }

	  // recursive search

	  move_do(board,move,undo);
	  
	  SearchCurrent[ThreadId]->last_move = move;

      if (node_type != NodePV || best_value == ValueNone) { // first move or non-pv
		 value = -full_search(board,-beta,-alpha,new_depth-reduction,height+1,new_pv,NODE_OPP(node_type),cap_extended,ThreadId);
         
         // The move was reduced and fails high; so we research with full depth
         if (reduced && value >= beta){
		        value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NODE_OPP(node_type),cap_extended,ThreadId);

         }
         
      } else { // other moves (all PV children)
      
		 value = -full_search(board,-alpha-1,-alpha,new_depth-reduction,height+1,new_pv,NodeCut,cap_extended,ThreadId);
         
         // In case of fail high:
               
         // If reduced then we try a research with node_type = NodePV
         if (value > alpha && reduced){  // && value < beta
			value = -full_search(board,-beta,-alpha,new_depth-reduction,height+1,new_pv,NodePV,cap_extended,ThreadId);
		    
            // Still fails high! We research to full depth
            if (reduced && value >= beta){	
		        value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NodePV,cap_extended,ThreadId);
            }
         
         // If not reduced we research as a PV node   
         } else if (value > alpha){	
		    value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NodePV,cap_extended,ThreadId);
         }
      }

      move_undo(board,move,undo);

      played[played_nb++] = move;
	  
      if (value > best_value) {
         best_value = value;
         pv_cat(pv,new_pv,move);
         if (value > alpha) {
            alpha = value;
            best_move = move;
			if (value >= beta){ 
				goto cut;
			}
         }
      }

      if (node_type == NodeCut) node_type = NodeAll;
   }

   // ALL node

   if (best_value == ValueNone) { // no legal move
      if (in_check) {
         ASSERT(board_is_mate(board));
         return VALUE_MATE(height);
      } else {
         ASSERT(board_is_stalemate(board));
         return ValueDraw;
      }
   }

cut:

   ASSERT(value_is_ok(best_value));

   // move ordering

   if (best_move != MoveNone) {

      good_move(best_move,board,depth,height,ThreadId);

      if (best_value >= beta && !move_is_tactical(best_move,board)) { // check is 204b and d, e
         
         // refutation table ~5 elo
         if (last_move != MoveNull && last_move != MoveNone)refutation_update(best_move, last_move, board, ThreadId);

		 ASSERT(played_nb>0&&played[played_nb-1]==best_move);

	 	 for (i = 0; i < played_nb-1; i++) {
			move = played[i];
			ASSERT(move!=best_move);
			history_bad(move,board,ThreadId);
			bad_move(move,board,depth,height,ThreadId); // added JD ~ 5 elo
		 }

		 history_good(best_move,board,ThreadId);
		
      }
   }

   // transposition table

   if (UseTrans && depth >= TransDepth) {

      trans_move = best_move;
      trans_depth = depth;
      trans_flags = TransUnknown;
      if (best_value > old_alpha) trans_flags |= TransLower;
      if (best_value < beta) trans_flags |= TransUpper;
      trans_value = value_to_trans(best_value,height);

      trans_store(Trans,board->key,trans_move,trans_depth,trans_flags,trans_value);

   }

   return best_value;
}

// full_quiescence()

static int full_quiescence(board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int ThreadId) {

   bool in_check;
   int old_alpha;
   int value, best_value;
   int best_move;
   int move;
   int opt_value;
   attack_t attack[1];
   sort_t sort[1];
   undo_t undo[1];
   mv_t new_pv[HeightMax];
   int trans_move, trans_depth, trans_flags, trans_value;
   entry_t * found_entry;

   ASSERT(board!=NULL);
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(pv!=NULL);

   ASSERT(board_is_legal(board));
   ASSERT(depth<=0);

   // init

   SearchCurrent[ThreadId]->node_nb++;
   SearchInfo[ThreadId]->check_nb--;
   PV_CLEAR(pv);

   if (height > SearchCurrent[ThreadId]->max_depth) SearchCurrent[ThreadId]->max_depth = height;

   if (SearchInfo[ThreadId]->check_nb <= 0) {
      SearchInfo[ThreadId]->check_nb += SearchInfo[ThreadId]->check_inc;
      search_check(ThreadId);
   }

   // draw?

   if (board_is_repetition(board)) return ValueDraw;

   if (recog_draw(board,ThreadId)) return ValueDraw;

   // mate-distance pruning

   if (UseDistancePruning) {

      // lower bound

      value = VALUE_MATE(height+2); // does not work if the current position is mate
      if (value > alpha && board_is_mate(board)) value = VALUE_MATE(height);

      if (value > alpha) {
         alpha = value;
         if (value >= beta) return value;
      }

      // upper bound

      value = -VALUE_MATE(height+1);

      if (value < beta) {
         beta = value;
         if (value <= alpha) return value;
      }
   }
   
   // transposition table: added by Jerry Donald, about +20 elo self-play
   // new in Toga II 4.0: accept hash hits in PV ~ +3 elo
   
   trans_move = MoveNone;
   
   if (UseTrans) { 

      if (trans_retrieve(Trans,&found_entry,board->key,&trans_move,&trans_depth,&trans_flags,&trans_value)) {

		 trans_value = value_from_trans(trans_value,height);

	 	 if ((UseExact && trans_value != ValueNone && TRANS_IS_EXACT(trans_flags)) 
		 	 || (TRANS_IS_LOWER(trans_flags) && trans_value >= beta)
			 || (TRANS_IS_UPPER(trans_flags) && trans_value <= alpha)) {
			 return trans_value;
		 } 
	  }
   }

   // more init

   attack_set(attack,board);
   in_check = ATTACK_IN_CHECK(attack);

   if (in_check) {
      ASSERT(depth<0);
      depth++; // in-check extension
   }

   // height limit

   if (height >= HeightMax-1) return eval(board, alpha, beta, ThreadId);

   // more init

   old_alpha = alpha;
   best_value = ValueNone;
   best_move = MoveNone;

   /* if (UseDelta) */ opt_value = +ValueInf;

   if (!in_check) {

      // lone-king stalemate?

      if (simple_stalemate(board)) return ValueDraw;

      // stand pat

      value = eval(board, alpha, beta, ThreadId);
      
      if (trans_move != MoveNone && trans_value != ValueNone){
      	  // if (UseExact && TRANS_IS_EXACT(trans_flags)) value = trans_value; // HACK: not used as would've returned earlier
      	  if (TRANS_IS_LOWER(trans_flags) && trans_value > value) value = trans_value;
      	  else if (TRANS_IS_UPPER(trans_flags) && trans_value < value) value = trans_value;
	  }

      ASSERT(value>best_value);
      best_value = value;
      if (value > alpha) {
         alpha = value;
         if (value >= beta) goto cut;
      }

      if (UseDelta) {
         opt_value = value + DeltaMargin;
         ASSERT(opt_value<+ValueInf);
      }
   }

   // move loop

   sort_init_qs(sort,board,attack, depth>=SearchCurrent[ThreadId]->CheckDepth /* depth>=cd */);

   while ((move=sort_next_qs(sort)) != MoveNone) {

	  // delta pruning

      if (UseDelta && beta == old_alpha+1) { // i.e. non-PV

         if (!in_check && !move_is_check(move,board) && !capture_is_dangerous(move,board)) {

            ASSERT(move_is_tactical(move,board));

            // optimistic evaluation

            value = opt_value;

            int to = MOVE_TO(move);
            int capture = board->square[to];

            if (capture != Empty) {
               value += VALUE_PIECE(capture);
            } else if (MOVE_IS_EN_PASSANT(move)) {
               value += ValuePawn;
            }

            if (MOVE_IS_PROMOTE(move)) value += ValueQueen - ValuePawn;

            // pruning

            if (value <= alpha) {

               if (value > best_value) {
                  best_value = value;
                  PV_CLEAR(pv);
               }

               continue;
            }
         }
      }

      move_do(board,move,undo);
      value = -full_quiescence(board,-beta,-alpha,depth-1,height+1,new_pv,ThreadId);
      move_undo(board,move,undo);

      if (value > best_value) {
         best_value = value;
         pv_cat(pv,new_pv,move);
         if (value > alpha) {
            alpha = value;
            best_move = move;
			if (value >= beta) goto cut;
         }
      }
   }

   // ALL node

   if (best_value == ValueNone) { // no legal move
      ASSERT(board_is_mate(board));
      return VALUE_MATE(height);
   }

cut:
    
   // store result in hash table (JD)
    
   if (UseTrans) {

      trans_move = best_move;
      trans_depth = 0;
      trans_flags = TransUnknown;
      if (best_value > old_alpha) trans_flags |= TransLower;
      if (best_value < beta) trans_flags |= TransUpper;
      trans_value = value_to_trans(best_value,height);

      trans_store(Trans,board->key,trans_move,trans_depth,trans_flags,trans_value);

   }

   ASSERT(value_is_ok(best_value));

   return best_value;
}

// full_new_depth()

static int full_new_depth(int depth, int move, board_t * board, bool single_reply, bool in_pv, int height, bool extended, bool * cap_extended, int ThreadId) {

   int new_depth;

   ASSERT(depth_is_ok(depth));
   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(single_reply==true||single_reply==false);
   ASSERT(in_pv==true||in_pv==false);

   ASSERT(depth>0);

   new_depth = depth - 1;
   *cap_extended = false; // not used currently
   
   // transition to simple endgame
   if (in_pv && board->square[MOVE_TO(move)] != Empty && !PIECE_IS_PAWN(board->square[MOVE_TO(move)])){
	  if ((board->piece_size[White] + board->piece_size[Black]) == 3){
		 return new_depth+1;
	  }
	  else if ((board->piece_size[White] == 3 && board->piece_size[Black] == 2) 
		 || (board->piece_size[White] == 2 && board->piece_size[Black] == 3))
		 return new_depth+1; 
   }
   
   // single reply & check
   if ((single_reply && ExtendSingleReply)
	  || move_is_check(move,board) && (in_pv || see_move(move,board) >= -100)) {
      // @tried no check extension in endgame (~ -5 elo)
      // @tried no bad SEE check extension in PV (~ -5 elo)
	  return new_depth+1;
   }
   
   // passed pawn moves
   if (in_pv && PIECE_IS_PAWN(MOVE_PIECE(move,board))){
	  if (is_passed(board,MOVE_TO(move))) return new_depth+1;
   }

   // interesting captures
   if (in_pv &&  board->square[MOVE_TO(move)] != PieceNone256 
	  && !extended && see_move(move,board) >= -100){
	  *cap_extended = true;
	  return new_depth+1;
   }
   
   // pawn endgame
   if (board->square[MOVE_TO(move)] != Empty && pawn_is_endgame(move,board)){
   	  return new_depth+1;
   }
  
   ASSERT(new_depth>=0&&new_depth<=depth);

   return new_depth;
}

// do_null()

static bool do_null(const board_t * board) {

   ASSERT(board!=NULL);

   // use null move if the side-to-move has at least one piece

   return board->piece_size[board->turn] >= 2; // king + one piece
}

// do_ver()

static bool do_ver(const board_t * board) {

   ASSERT(board!=NULL);

   // use verification if the side-to-move has at most one piece

   return board->piece_size[board->turn] <= 3; // king + one piece was 2
}

// pv_fill()

static void pv_fill(const mv_t pv[], board_t * board) {

   int move;
   int trans_move, trans_depth;
   undo_t undo[1];

   ASSERT(pv!=NULL);
   ASSERT(board!=NULL);

   ASSERT(UseTrans);

   move = *pv;

   if (move != MoveNone && move != MoveNull) {

      move_do(board,move,undo);
      pv_fill(pv+1,board);
      move_undo(board,move,undo);

      trans_move = move;
      trans_depth = -127; // HACK
      
      trans_store(Trans,board->key,trans_move,trans_depth,TransUnknown,-ValueInf);
   }
}

// move_is_dangerous()

static bool move_is_dangerous(int move, const board_t * board) {

   int piece;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(!move_is_tactical(move,board));

   piece = MOVE_PIECE(move,board);

   if (PIECE_IS_PAWN(piece)
    && is_passed(board,MOVE_TO(move)) /*PAWN_RANK(MOVE_TO(move),board->turn) >= Rank7*/) {
      return true;
   }

   return false;
}

// capture_is_dangerous()

static bool capture_is_dangerous(int move, const board_t * board) {

   int piece, capture;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(move_is_tactical(move,board));

   piece = MOVE_PIECE(move,board);

   if (PIECE_IS_PAWN(piece)
    && PAWN_RANK(MOVE_TO(move),board->turn) >= Rank7) {
      return true;
   }

   capture = move_capture(move,board);

   if (PIECE_IS_QUEEN(capture)) return true;

   if (PIECE_IS_PAWN(capture)
    && PAWN_RANK(MOVE_TO(move),board->turn) <= Rank2) {
      return true;
   }

   return false;
}

// simple_stalemate()

static bool simple_stalemate(const board_t * board) {

   int me, opp;
   int king;
   int opp_flag;
   int from, to;
   int capture;
   const inc_t * inc_ptr;
   int inc;

   ASSERT(board!=NULL);

   ASSERT(board_is_legal(board));
   ASSERT(!board_is_check(board));

   // lone king?

   me = board->turn;
   if (board->piece_size[me] != 1 || board->pawn_size[me] != 0) return false; // no

   // king in a corner?

   king = KING_POS(board,me);
   if (king != A1 && king != H1 && king != A8 && king != H8) return false; // no

   // init

   opp = COLOUR_OPP(me);
   opp_flag = COLOUR_FLAG(opp);

   // king can move?

   from = king;

   for (inc_ptr = KingInc; (inc=*inc_ptr) != IncNone; inc_ptr++) {
      to = from + inc;
      capture = board->square[to];
      if (capture == Empty || FLAG_IS(capture,opp_flag)) {
         if (!is_attacked(board,to,opp)) return false; // legal king move
      }
   }

   // no legal move

   ASSERT(board_is_stalemate((board_t*)board));

   return true;
}

static bool is_passed(const board_t * board, int to) { 

   int t2; 
   int me, opp;
   int file, rank;

   me = board->turn; 
   opp = COLOUR_OPP(me);
   file = SQUARE_FILE(to);
   rank = PAWN_RANK(to,me);
 
   t2 = board->pawn_file[me][file] | BitRev[board->pawn_file[opp][file]]; 

   // passed pawns 
   if ((t2 & BitGT[rank]) == 0) {  
      if (((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitGT[rank]) == 0) 
          return true; 
   } 

   return false;
 
}

// pawn_is_endgame() from Fruit 3.2.1

static bool pawn_is_endgame(int move, const board_t * board) {

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(move_is_capture(move,board));
   
   int piece;
   
   piece = board->square[MOVE_TO(move)];

   // special cases

   if (PIECE_IS_PAWN(piece)) return false;

   if (MOVE_IS_EN_PASSANT(move)) return false;

   if (MOVE_IS_PROMOTE(move)) return false;

   if (board->piece_size[White] + board->piece_size[Black] - 1 == 2) return true;

   return false;
   
}

// end of search_full.cpp

