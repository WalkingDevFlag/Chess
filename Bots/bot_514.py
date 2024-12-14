from Chessnut import Game
import random
import chess  # Python Chess Library for advanced board operations
import time  # For tracking elapsed time

def chess_bot(obs):
    try:
        game = Game(obs.board)  # Initialize the game with the current FEN string

        class ChessBot514:
            def __init__(self):
                # Initialize quiet move history
                self.quiet_history = [0] * 4096

                # Transposition table
                self.transposition_table = [None] * 2097152

                # Evaluation constants, packed
                self.extracted = [
                    4835740172228143389605888, 1862983114964290202813595648, 6529489037797228073584297991,
                    6818450810788061916507740187, 7154536855449028663353021722, 14899014974757699833696556826,
                    25468819436707891759039590695, 29180306561342183501734565961, 944189991765834239743752701,
                    4194697739, 4340114601700738076711583744, 3410436627687897068963695623,
                    11182743911298765866015857947, 10873240011723255639678263585,
                    17684436730682332602697851426, 17374951722591802467805509926, 31068658689795177567161113954,
                    1534136309681498319279645285, 18014679997410182140, 1208741569195510172352512,
                    13789093343132567021105512448, 6502873946609222871099113472, 1250
                ]

                # Process and pack evalValues
                self.eval_values = self.process_eval_values()

            def process_eval_values(self):
                eval_values = []
                for value in self.extracted:
                    bits = list(value.to_bytes(16, byteorder="little"))
                    eval_values.extend(bits[:6])
                return eval_values

            def think(self, fen, timer=None):
                board = chess.Board(fen)  # Initialize chess.Board from FEN
                root_best_move = None
                killers = [None] * 256
                allocated_time = timer.milliseconds_remaining // 8 if timer else 5  # Default to 5 seconds
                start_time = time.time()  # Record start time
                depth = 1

                # Decay quiet history
                self.quiet_history = [x // 8 for x in self.quiet_history]

                def search(ply, depth, alpha, beta, null_allowed):
                    nonlocal board

                    # Repetition detection
                    if null_allowed and board.is_repetition(3):
                        return 0

                    # Check extension
                    in_check = board.is_check()
                    if in_check:
                        depth += 1

                    # Use FEN string for the transposition table key
                    key = hash(board.fen())
                    in_qsearch = depth <= 0
                    best_score = -2_000_000
                    do_pruning = alpha == beta - 1 and not in_check
                    score = 15
                    phase = 0

                    # Static evaluation
                    for is_white in (not board.turn, board.turn):
                        score = -score
                        for piece_index in range(1, 7):  # Iterate over piece types
                            # Add your logic for piece evaluation and mobility here
                            pass

                    # Tapered evaluation
                    score = ((score * phase + (score >> 16) * (24 - phase)) // 24)

                    # Rest of the logic for searching moves and pruning
                    return score

                while True:
                    elapsed_time = time.time() - start_time

                    # Stop the search if time runs out
                    if elapsed_time >= allocated_time:
                        break

                    # Perform search and update the best move
                    current_best_move = search(0, depth, -1_000_000, 1_000_000, True)
                    if current_best_move:
                        root_best_move = current_best_move

                    depth += 1

                return root_best_move

        # Instantiate the bot
        bot = ChessBot514()
        best_move = bot.think(game.get_fen())
        return best_move
    
    except Exception as e:
        print(f"Error in chess_bot: {e}")
        return "0000"  # Return invalid move to indicate failure