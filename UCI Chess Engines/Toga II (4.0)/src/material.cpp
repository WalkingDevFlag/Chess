
// material.cpp

// includes

#include <cstring>

#include "board.h"
#include "colour.h"
#include "hash.h"
#include "material.h"
#include "option.h"
#include "piece.h"
#include "protocol.h"
#include "square.h"
#include "util.h"
#include "search.h"

// constants

static const bool UseTable = true;
static const uint32 TableSize = 256; // was 256 4kB tried 16384

static const int PawnPhase   = 0;
static const int KnightPhase = 1;
static const int BishopPhase = 1;
static const int RookPhase   = 2;
static const int QueenPhase  = 4;

static const int TotalPhase = PawnPhase * 16 + KnightPhase * 4 + BishopPhase * 4 + RookPhase * 4 + QueenPhase * 2;

// constants and variables

#define ABS(x) ((x)<0?-(x):(x))

static /* const */ int OpeningExchangePenalty = 20; /* Thomas penalty exchange piece pawn */
static /* const */ int EndgameExchangePenalty = 20;

static /* const */ int MaterialWeight = 256; // 100%

static const int PawnOpening   = 75; /// J Donald's setting
static const int PawnEndgame   = 90; // was 100
static const int KnightOpening = 325;
static const int KnightEndgame = 310; // J Donald's settings, was 315
static const int BishopOpening = 325;
static const int BishopEndgame = 320; // Chris Formula's setting was 315
static const int RookOpening   = 490; // J Donald's settings, was 500
static const int RookEndgame   = 510; // J Donald's settings, was 500
static const int QueenOpening  = 975; // was 1000
static const int QueenEndgame  = 1000; 

static const int BishopPairOpening = 45; // JD setting; was 50
static const int BishopPairEndgame = 65; // JD setting; was 50

static /* const */ int KingPawnBonus = 30; // Endgame material adjustments
static /* const */ int RookPawnPenalty = 10;

// types

typedef material_info_t entry_t;

struct material_t {
   entry_t * table;
   uint32 size;
   uint32 mask;
   uint32 used;
   sint64 read_nb;
   sint64 read_hit;
   sint64 write_nb;
   sint64 write_collision;
};

// variables

static material_t Material[MaxThreads][1];

// prototypes

static void material_comp_info (material_info_t * info, const board_t * board);

// functions

// material_init()

void material_parameter() {

   // UCI options

   MaterialWeight = (option_get_int("Material") * 256 + 50) / 100;
   OpeningExchangePenalty = option_get_int("Toga Exchange Bonus");
   EndgameExchangePenalty = OpeningExchangePenalty; 
   
   // Endgame settings
   KingPawnBonus = option_get_int("Toga King Pawn Endgame Bonus");
   RookPawnPenalty = option_get_int("Toga Rook Pawn Endgame Penalty");

}

void material_init() {

	int ThreadId;

   // UCI options

   material_parameter();

   // material table

	for (ThreadId = 0; ThreadId < NumberThreads; ThreadId++){
		Material[ThreadId]->size = 0;
		Material[ThreadId]->mask = 0;
		Material[ThreadId]->table = NULL;
	}
}

// material_alloc()

void material_alloc() {

	int ThreadId;

   ASSERT(sizeof(entry_t)==16);

   if (UseTable) {

		for (ThreadId = 0; ThreadId < NumberThreads; ThreadId++){
			Material[ThreadId]->size = TableSize;
			Material[ThreadId]->mask = TableSize - 1;
			Material[ThreadId]->table = (entry_t *) my_malloc((uint64) Material[ThreadId]->size*sizeof(entry_t));

			material_clear(ThreadId);
		}
   }
}

// material_free()

void material_free() {

	int ThreadId;

   ASSERT(sizeof(entry_t)==16);

   if (UseTable) {

		for (ThreadId = 0; ThreadId < NumberThreads; ThreadId++){
		  my_free(Material[ThreadId]->table);
		}
   }
}

// material_clear()

void material_clear(int ThreadId) {

   if (Material[ThreadId]->table != NULL) {
      memset(Material[ThreadId]->table,0,Material[ThreadId]->size*sizeof(entry_t));
   }

   Material[ThreadId]->used = 0;
   Material[ThreadId]->read_nb = 0;
   Material[ThreadId]->read_hit = 0;
   Material[ThreadId]->write_nb = 0;
   Material[ThreadId]->write_collision = 0;
}

// material_get_info()

void material_get_info(material_info_t * info, const board_t * board, int ThreadId) {

   uint64 key;
   entry_t * entry;

   ASSERT(info!=NULL);
   ASSERT(board!=NULL);

   // probe

   if (UseTable) {

      Material[ThreadId]->read_nb++;

      key = board->material_key;
      entry = &Material[ThreadId]->table[KEY_INDEX(key)&Material[ThreadId]->mask];

      if (entry->lock == KEY_LOCK(key)) {

         // found

         Material[ThreadId]->read_hit++;

         *info = *entry;

         return;
      }
   }

   // calculation

   material_comp_info(info,board);

   // store

   if (UseTable) {

      Material[ThreadId]->write_nb++;

      if (entry->lock == 0) { // HACK: assume free entry
         Material[ThreadId]->used++;
      } else {
         Material[ThreadId]->write_collision++;
      }

      *entry = *info;
      entry->lock = KEY_LOCK(key);
   }
}

// material_comp_info()

static void material_comp_info(material_info_t * info, const board_t * board) {

   int wp, wn, wb, wr, wq;
   int bp, bn, bb, br, bq;
   int wt, bt;
   int wm, bm;
   int colour;
   int recog;
   int flags;
   int cflags[ColourNb];
   int mul[ColourNb];
   int phase;
   int opening, endgame;
   int owf,obf,ewf,ebf; /* Thomas */
   int WhiteMinors,BlackMinors,WhiteMajors,BlackMajors;

   ASSERT(info!=NULL);
   ASSERT(board!=NULL);

   // init

   wp = board->number[WhitePawn12];
   wn = board->number[WhiteKnight12];
   wb = board->number[WhiteBishop12];
   wr = board->number[WhiteRook12];
   wq = board->number[WhiteQueen12];

   bp = board->number[BlackPawn12];
   bn = board->number[BlackKnight12];
   bb = board->number[BlackBishop12];
   br = board->number[BlackRook12];
   bq = board->number[BlackQueen12];

   wt = wq + wr + wb + wn + wp; // no king
   bt = bq + br + bb + bn + bp; // no king

   wm = wb + wn;
   bm = bb + bn;

   // recogniser

   recog = MAT_NONE;

   if (false) {

   } else if (wt == 0 && bt == 0) {

      recog = MAT_KK;

   } else if (wt == 1 && bt == 0) {

      if (wb == 1) recog = MAT_KBK;
      if (wn == 1) recog = MAT_KNK;
      if (wp == 1) recog = MAT_KPK;

   } else if (wt == 0 && bt == 1) {

      if (bb == 1) recog = MAT_KKB;
      if (bn == 1) recog = MAT_KKN;
      if (bp == 1) recog = MAT_KKP;

   } else if (wt == 1 && bt == 1) {

      if (wq == 1 && bq == 1) recog = MAT_KQKQ;
      if (wq == 1 && bp == 1) recog = MAT_KQKP;
      if (wp == 1 && bq == 1) recog = MAT_KPKQ;

      if (wr == 1 && br == 1) recog = MAT_KRKR;
      if (wr == 1 && bp == 1) recog = MAT_KRKP;
      if (wp == 1 && br == 1) recog = MAT_KPKR;

      if (wb == 1 && bb == 1) recog = MAT_KBKB;
      if (wb == 1 && bp == 1) recog = MAT_KBKP;
      if (wp == 1 && bb == 1) recog = MAT_KPKB;

      if (wn == 1 && bn == 1) recog = MAT_KNKN;
      if (wn == 1 && bp == 1) recog = MAT_KNKP;
      if (wp == 1 && bn == 1) recog = MAT_KPKN;

   } else if (wt == 2 && bt == 0) {

      if (wb == 1 && wp == 1) recog = MAT_KBPK;
      if (wn == 1 && wp == 1) recog = MAT_KNPK;

   } else if (wt == 0 && bt == 2) {

      if (bb == 1 && bp == 1) recog = MAT_KKBP;
      if (bn == 1 && bp == 1) recog = MAT_KKNP;

   } else if (wt == 2 && bt == 1) {

      if (wr == 1 && wp == 1 && br == 1) recog = MAT_KRPKR;
      if (wb == 1 && wp == 1 && bb == 1) recog = MAT_KBPKB;

   } else if (wt == 1 && bt == 2) {

      if (wr == 1 && br == 1 && bp == 1) recog = MAT_KRKRP;
      if (wb == 1 && bb == 1 && bp == 1) recog = MAT_KBKBP;
   }

   // draw node (exact-draw recogniser)

   flags = 0; // TODO: MOVE ME
   for (colour = 0; colour < ColourNb; colour++) cflags[colour] = 0;

   if (wq+wr+wp == 0 && bq+br+bp == 0) { // no major piece or pawn
      if (wm + bm <= 1 // at most one minor => KK, KBK or KNK
       || recog == MAT_KBKB) {
         flags |= DrawNodeFlag;
      }
   } else if (recog == MAT_KPK  || recog == MAT_KKP
           || recog == MAT_KBPK || recog == MAT_KKBP) {
      flags |= DrawNodeFlag;
   }

   // bishop endgame

   if (wq+wr+wn == 0 && bq+br+bn == 0) { // only bishops
      if (wb == 1 && bb == 1) {
         if (wp-bp >= -2 && wp-bp <= +2) { // pawn diff <= 2
            flags |= DrawBishopFlag;
         }
      }
   }

   // multipliers

   for (colour = 0; colour < ColourNb; colour++) mul[colour] = 16; // 1

   // white multiplier

   if (wp == 0) { // white has no pawns

      int w_maj = wq * 2 + wr;
      int w_min = wb + wn;
      int w_tot = w_maj * 2 + w_min;

      int b_maj = bq * 2 + br;
      int b_min = bb + bn;
      int b_tot = b_maj * 2 + b_min;

      if (false) {

      } else if (w_tot == 1) {

         ASSERT(w_maj==0);
         ASSERT(w_min==1);

         // KBK* or KNK*, always insufficient

         mul[White] = 0;

      } else if (w_tot == 2 && wn == 2) {

         ASSERT(w_maj==0);
         ASSERT(w_min==2);

         // KNNK*, usually insufficient

         if (b_tot != 0 || bp == 0) {
            mul[White] = 0;
         } else { // KNNKP+, might not be draw
            mul[White] = 1; // 1/16
         }

      } else if (w_tot == 2 && wb == 2 && b_tot == 1 && bn == 1) {

         ASSERT(w_maj==0);
         ASSERT(w_min==2);
         ASSERT(b_maj==0);
         ASSERT(b_min==1);

         // KBBKN*, barely drawish (not at all?)

         mul[White] = 8; // 1/2

      } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

         // no more than 1 minor up, drawish

         mul[White] = 2; // 1/8
      }

   } else if (wp == 1) { // white has one pawn

      int w_maj = wq * 2 + wr;
      int w_min = wb + wn;
      int w_tot = w_maj * 2 + w_min;

      int b_maj = bq * 2 + br;
      int b_min = bb + bn;
      int b_tot = b_maj * 2 + b_min;

      if (false) {

      } else if (b_min != 0) {

         // assume black sacrifices a minor against the lone pawn

         b_min--;
         b_tot--;

         if (false) {

         } else if (w_tot == 1) {

            ASSERT(w_maj==0);
            ASSERT(w_min==1);

            // KBK* or KNK*, always insufficient

            mul[White] = 4; // 1/4

         } else if (w_tot == 2 && wn == 2) {

            ASSERT(w_maj==0);
            ASSERT(w_min==2);

            // KNNK*, usually insufficient

            mul[White] = 4; // 1/4

         } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[White] = 8; // 1/2
         }

      } else if (br != 0) {

         // assume black sacrifices a rook against the lone pawn

         b_maj--;
         b_tot -= 2;

         if (false) {

         } else if (w_tot == 1) {

            ASSERT(w_maj==0);
            ASSERT(w_min==1);

            // KBK* or KNK*, always insufficient

            mul[White] = 4; // 1/4

         } else if (w_tot == 2 && wn == 2) {

            ASSERT(w_maj==0);
            ASSERT(w_min==2);

            // KNNK*, usually insufficient

            mul[White] = 4; // 1/4

         } else if (w_tot-b_tot <= 1 && w_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[White] = 8; // 1/2
         }
      }
   }

   // black multiplier

   if (bp == 0) { // black has no pawns

      int w_maj = wq * 2 + wr;
      int w_min = wb + wn;
      int w_tot = w_maj * 2 + w_min;

      int b_maj = bq * 2 + br;
      int b_min = bb + bn;
      int b_tot = b_maj * 2 + b_min;

      if (false) {

      } else if (b_tot == 1) {

         ASSERT(b_maj==0);
         ASSERT(b_min==1);

         // KBK* or KNK*, always insufficient

         mul[Black] = 0;

      } else if (b_tot == 2 && bn == 2) {

         ASSERT(b_maj==0);
         ASSERT(b_min==2);

         // KNNK*, usually insufficient

         if (w_tot != 0 || wp == 0) {
            mul[Black] = 0;
         } else { // KNNKP+, might not be draw
            mul[Black] = 1; // 1/16
         }

      } else if (b_tot == 2 && bb == 2 && w_tot == 1 && wn == 1) {

         ASSERT(b_maj==0);
         ASSERT(b_min==2);
         ASSERT(w_maj==0);
         ASSERT(w_min==1);

         // KBBKN*, barely drawish (not at all?)

         mul[Black] = 8; // 1/2

      } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

         // no more than 1 minor up, drawish

         mul[Black] = 2; // 1/8
      }

   } else if (bp == 1) { // black has one pawn

      int w_maj = wq * 2 + wr;
      int w_min = wb + wn;
      int w_tot = w_maj * 2 + w_min;

      int b_maj = bq * 2 + br;
      int b_min = bb + bn;
      int b_tot = b_maj * 2 + b_min;

      if (false) {

      } else if (w_min != 0) {

         // assume white sacrifices a minor against the lone pawn

         w_min--;
         w_tot--;

         if (false) {

         } else if (b_tot == 1) {

            ASSERT(b_maj==0);
            ASSERT(b_min==1);

            // KBK* or KNK*, always insufficient

            mul[Black] = 4; // 1/4

         } else if (b_tot == 2 && bn == 2) {

            ASSERT(b_maj==0);
            ASSERT(b_min==2);

            // KNNK*, usually insufficient

            mul[Black] = 4; // 1/4

         } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[Black] = 8; // 1/2
         }

      } else if (wr != 0) {

         // assume white sacrifices a rook against the lone pawn

         w_maj--;
         w_tot -= 2;

         if (false) {

         } else if (b_tot == 1) {

            ASSERT(b_maj==0);
            ASSERT(b_min==1);

            // KBK* or KNK*, always insufficient

            mul[Black] = 4; // 1/4

         } else if (b_tot == 2 && bn == 2) {

            ASSERT(b_maj==0);
            ASSERT(b_min==2);

            // KNNK*, usually insufficient

            mul[Black] = 4; // 1/4

         } else if (b_tot-w_tot <= 1 && b_maj <= 2) {

            // no more than 1 minor up, drawish

            mul[Black] = 8; // 1/2
         }
      }
   }

   // potential draw for white

   if (wt == wb+wp && wp >= 1) cflags[White] |= MatRookPawnFlag;
   if (wt == wb+wp && wb <= 1 && wp >= 1 && bt > bp) cflags[White] |= MatBishopFlag;

   if (wt == 2 && wn == 1 && wp == 1 && bt > bp) cflags[White] |= MatKnightFlag;

   // potential draw for black

   if (bt == bb+bp && bp >= 1) cflags[Black] |= MatRookPawnFlag;
   if (bt == bb+bp && bb <= 1 && bp >= 1 && wt > wp) cflags[Black] |= MatBishopFlag;

   if (bt == 2 && bn == 1 && bp == 1 && wt > wp) cflags[Black] |= MatKnightFlag;

   // draw leaf (likely draw)

   if (recog == MAT_KQKQ || recog == MAT_KRKR) {
      mul[White] = 0;
      mul[Black] = 0;
   }

   // king safety

   if (bq >= 1 && bq+br+bb+bn >= 2) cflags[White] |= MatKingFlag;
   if (wq >= 1 && wq+wr+wb+wn >= 2) cflags[Black] |= MatKingFlag;

   // phase (0: opening -> 256: endgame)

   phase = TotalPhase;

   phase -= wp * PawnPhase;
   phase -= wn * KnightPhase;
   phase -= wb * BishopPhase;
   phase -= wr * RookPhase;
   phase -= wq * QueenPhase;

   phase -= bp * PawnPhase;
   phase -= bn * KnightPhase;
   phase -= bb * BishopPhase;
   phase -= br * RookPhase;
   phase -= bq * QueenPhase;

   if (phase < 0) phase = 0;

   ASSERT(phase>=0&&phase<=TotalPhase);
   phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;

   ASSERT(phase>=0&&phase<=256);

   // material

   opening = 0;
   endgame = 0;

   /* Thomas */
   owf = wn*KnightOpening + wb*BishopOpening + wr*RookOpening + wq*QueenOpening; 
   opening += owf;
   opening += wp * PawnOpening;

   obf = bn*KnightOpening + bb*BishopOpening + br*RookOpening + bq*QueenOpening; 
   opening -= obf;
   opening -= bp * PawnOpening;

   ewf = wn*KnightEndgame + wb*BishopEndgame + wr*RookEndgame + wq*QueenEndgame;  
   endgame += wp * PawnEndgame;
   endgame += ewf;

   ebf = bn*KnightEndgame + bb*BishopEndgame + br*RookEndgame + bq*QueenEndgame; 
   endgame -= bp * PawnEndgame;
   endgame -= ebf;

/*   WhiteMinors = wn + wb;
   BlackMinors = bn + bb;
   WhiteMajors = 2*wq + wr;
   BlackMajors = 2*bq + br; */

   // Trade Bonus

   if (owf > obf && bp > wp){
	   opening += OpeningExchangePenalty;
	   endgame += OpeningExchangePenalty;
   } 
   else if (obf > owf && wp > bp){
	   opening -= OpeningExchangePenalty;
	   endgame -= OpeningExchangePenalty; 
   } 

/*   if (WhiteMinors != BlackMinors) {
		if (WhiteMajors == BlackMajors) {
		  if (WhiteMinors > BlackMinors){
			opening += OpeningExchangePenalty;
			endgame += OpeningExchangePenalty;
		  }
		  else{
			opening -= OpeningExchangePenalty;
			endgame -= OpeningExchangePenalty;
		  }
		}
	} */
   
   // Adjust knight and rook material values
   // White
   /* opening += wr * (5 - wp) * PawnOpening/8;
   endgame += wr * (5 - wp) * PawnEndgame/8;

   opening += wn * (wp - 5) * PawnOpening/16;
   endgame += wn * (wp - 5) * PawnEndgame/16;
   
   // Black
   opening -= br * (5 - bp) * PawnOpening/8;
   endgame -= br * (5 - bp) * PawnEndgame/8; 

   opening -= bn * (bp - 5) * PawnOpening/16;
   endgame -= bn * (bp - 5) * PawnEndgame/16; */

   // bishop pair

   if (wb >= 2) { // HACK: assumes different colours
      opening += BishopPairOpening;
      endgame += BishopPairEndgame;
   }

   if (bb >= 2) { // HACK: assumes different colours
      opening -= BishopPairOpening;
      endgame -= BishopPairEndgame;
   }
   
   // JD: King and Pawn Endgames (usually winning)
   
   if (wt - wp == 0 && bt - bp == 0){
          
      opening = (wp-bp)*150;
      endgame = (wp-bp)*150;
      
   }
   
   // Rook and Pawn Endgames (drawish)
   if (wt - wp == 1 && bt - bp == 1 && wr == 1 && br == 1){
      if (wp > bp){
         endgame -= RookPawnPenalty; // note - sign
      }
      if (bp > wp){
         endgame += RookPawnPenalty;
      }
   } 
   
   // Piece combo: Queen + Knight against Queen + Bishop
   
   if (wt-wp == 2 && bt-wp == 2 && wq == 1 && bq == 1 && wn == 1 && bb == 1)
       endgame += 10;
       
   else if (wt-wp == 2 && bt-wp == 2 && wq == 1 && bq == 1 && wb == 1 && bn == 1)
       endgame -= 10;

   // store info
   info->recog = recog;
   info->flags = flags;
   for (colour = 0; colour < ColourNb; colour++) info->cflags[colour] = cflags[colour];
   for (colour = 0; colour < ColourNb; colour++) info->mul[colour] = mul[colour];
   info->phase = phase;
   info->opening = (opening * MaterialWeight) / 256;
   info->endgame = (endgame * MaterialWeight) / 256;
}

// end of material.cpp

