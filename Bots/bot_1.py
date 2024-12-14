import chess
import time
from typing import Tuple
from Chessnut import Game

class EvilBot:
    def __init__(self):
        self.max_search_time = 0
        self.searching_depth = 0
        self.last_score = 0
        
        self.timer = None
        self.board = None
        
        self.search_best_move = None
        self.root_best_move = None
        
        # Transposition table (192 MiB)
        self.transposition_table = [(0, 0, 0, 0, 0)] * 0x800000
        
        # Piece-to history tables, per-color
        self.history = [[[0 for _ in range(64)] for _ in range(7)] for _ in range(2)]
        
        self.packed_data = [
            0x0000000000000000, 0x2328170f2d2a1401, 0x1f1f221929211507, 0x18202a1c2d261507,
            0x252e3022373a230f, 0x585b47456d65321c, 0x8d986f66a5a85f50, 0x0002000300070005,
            0xfffdfffd00060001, 0x2b1f011d20162306, 0x221c0b171f15220d, 0x1b1b131b271c1507,
            0x232d212439321f0b, 0x5b623342826c2812, 0x8db65b45c8c01014, 0x0000000000000000,
            0x615a413e423a382e, 0x6f684f506059413c, 0x82776159705a5543, 0x8b8968657a6a6150,
            0x948c7479826c6361, 0x7e81988f73648160, 0x766f7a7e70585c4e, 0x6c7956116e100000,
            0x3a3d2d2840362f31, 0x3c372a343b3a3838, 0x403e2e343c433934, 0x373e3b2e423b2f37,
            0x383b433c45433634, 0x353d4b4943494b41, 0x46432e354640342b, 0x55560000504f0511,
            0x878f635c8f915856, 0x8a8b5959898e5345, 0x8f9054518f8e514c, 0x96985a539a974a4c,
            0x9a9c67659e9d5f59, 0x989c807a9b9c7a6a, 0xa09f898ba59c6f73, 0xa1a18386a09b7e84,
            0xbcac7774b8c9736a, 0xbab17b7caebd7976, 0xc9ce7376cac57878, 0xe4de6f70dcd87577,
            0xf4ef7175eedc7582, 0xf9fa8383dfe3908e, 0xfffe7a81f4ec707f, 0xdfe79b94e1ee836c,
            0x2027252418003d38, 0x4c42091d31193035, 0x5e560001422c180a, 0x6e6200004d320200,
            0x756c000e5f3c1001, 0x6f6c333f663e3f1d, 0x535b55395c293c1b, 0x2f1e3d5e22005300,
            0x004c0037004b001f, 0x00e000ca00be00ad, 0x02e30266018800eb, 0xffdcffeeffddfff3,
            0xfff9000700010007, 0xffe90003ffeefff4, 0x00000000fff5000d,
        ]

    def eval_weight(self, item):
        return (self.packed_data[item >> 1] >> (item * 32)) & 0xFFFFFFFF

    def think(self, board, timer):
        self.board = board
        self.timer = timer

        self.max_search_time = timer.milliseconds_remaining // 4
        self.searching_depth = 1

        while True:
            try:
                if abs(self.last_score - self.negamax(self.last_score - 20, self.last_score + 20, self.searching_depth)) >= 20:
                    self.negamax(-32000, 32000, self.searching_depth)
                self.root_best_move = self.search_best_move
            except Exception:
                # out of time
                break

            self.searching_depth += 1
            if self.searching_depth > 200 or timer.milliseconds_elapsed_this_turn >= self.max_search_time // 10:
                break

        return self.root_best_move

    def negamax(self, alpha: int, beta: int, depth: int) -> int:
        # abort search if out of time, but we must search at least depth 1
        if self.timer.milliseconds_elapsed_this_turn >= self.max_search_time and self.searching_depth > 1:
            raise Exception()

        tt = self.transposition_table[self.board.zobrist_hash & 0x7FFFFF]
        tt_hash, tt_move_raw, score, tt_depth, tt_bound = tt

        tt_hit = tt_hash == self.board.zobrist_hash
        non_pv = alpha + 1 == beta
        in_q_search = depth <= 0
        piece_is_white = False  # This should be determined based on the board state.
        
        eval = 0x000b000a  # tempo
        best_score = self.board.ply_count - 30000
        old_alpha = alpha
        
        # Search loop vars
        move_count = 0  

        # Rest of the negamax implementation...

        return best_score

def chess_bot(obs):
    try:
        # Create a chess.Board object from the FEN string provided in obs.
        game = Game(obs.board)
        
        bot = EvilBot()
        
        # Timer setup (you may need to adjust this according to your timer implementation)
        class Timer:
            def __init__(self):
                # Initialize your timer attributes here
                pass
            
            @property
            def milliseconds_remaining(self):
                return int(time.time() * 1000) % (100 * 1000)  # Example: replace with actual logic
            
            @property
            def milliseconds_elapsed_this_turn(self):
                return int(time.time() * 1000) % (10 * 1000)   # Example: replace with actual logic

        timer = Timer()
        
        best_move = bot.think(game, timer)
        
        return best_move

    except Exception as e:
        print(f"Error in chess_bot: {e}")
