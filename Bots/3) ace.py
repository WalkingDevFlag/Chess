from Chessnut import Game
from collections import Counter

# Basic piece values

MATERIAL_VALUE: dict[str, int] = {
    "P": 82,
    "N": 337,
    "B": 365,
    "R": 477,
    "Q": 1025,
    "K": 0,
    "p": -82,
    "n": -337,
    "b": -365,
    "r": -477,
    "q": -1025,
    "k": 0,
}

# Positional values from Rofchade:
# https://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19

PAWN_VALUES: list[int] = [
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
]


KNIGHT_VALUES: list[int] = [
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
];

BISHOP_VALUES: list[int] = [
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
];

ROOK_VALUES: list[int] = [
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
];

QUEEN_VALUES: list[int] = [
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
];

KING_VALUES: list[int] = [
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
];

POSITIONAL_VALUE: dict[str, list[int]] = {
    "P": PAWN_VALUES,
    "N": KNIGHT_VALUES,
    "B": BISHOP_VALUES,
    "R": ROOK_VALUES,
    "Q": QUEEN_VALUES,
    "K": KING_VALUES
}

### Board Evaluation ###
def evaluate(game: Game) -> int:
    eval: int = 0

    # Basic material eval.
    eval += sum(MATERIAL_VALUE.get(piece, 0) for piece in game.get_fen().split(" ")[0])

    # Positional eval
    for i in range(64):
        piece: str = game.board.get_piece(i)

        if piece.upper() not in POSITIONAL_VALUE:
            continue

        piece_positional_value: int = POSITIONAL_VALUE[piece.upper()][i]

        eval += piece_positional_value if piece.isupper() else -piece_positional_value

    # IMPORTANT: In negamax, board evaluation must be side-to-move-relative
    if game.state.player == "b":
        eval = -eval

    return eval

### Define Constants ###
# Maximum possible evaluation, acts as a root bound for alpha-beta pruning
MAX_EVAL: int = 10000000

# Evaluation when board is drawn. Can be adjusted to discourage or encourage draws
DRAW_EVAL: int = 0

### Threefold Detection ###
def is_threefold(fen_history: list[str]) -> bool:
    return Counter(fen_history).most_common(1)[0][1] >= 3

### Negamax Search ###
def search(
    depth: int, ply: int, game: Game, alpha: int, beta: int
) -> tuple[int, str | None]:
    # Evaluate when depth <= 0
    if depth <= 0:
        return evaluate(game), None

    # Check for three-fold repetition
    if is_threefold(game.fen_history):
        return DRAW_EVAL, None

    legal_moves: list[str] = game.get_moves()

    # Check for stalemate or checkmate. Code taken from Chessnut library
    # and simplified
    if len(legal_moves) == 0:
        # Calculate whether the king is in check
        us: str = game.state.player
        king_piece, them = {"w": ("K", "b"), "b": ("k", "w")}.get(us)
        king_location: str = Game.i2xy(game.board.find_piece(king_piece))
        in_check = bool(
            [m[2:] for m in game._all_moves(player=them) if m[2:] == king_location]
        )

        # Stalemate
        if not in_check:
            return DRAW_EVAL, None

        # Prioritize shorter mates
        return -MAX_EVAL + ply, None

    # TODO: Here, you can order moves by most promising in order
    # to reach a beta-cutoff faster should it happen.

    # Track best value and best move
    best_value: int = -MAX_EVAL
    best_move: str | None = None

    # Make a copy of the position to perform search
    original_fen: str = game.get_fen()
    fen_history_reference: list[str] = game.fen_history

    for move in legal_moves:
        # Make move
        game.apply_move(move)

        # Negamax search, result is negated to keep score side-relative
        value: int = -search(depth - 1, ply + 1, game, -beta, -alpha)[0]

        # Update best move
        if value > best_value:
            best_value = value
            best_move = move

        if best_value > alpha:
            alpha = best_value
            # Beta cutoff
            if alpha >= beta:
                break

        # This part is messy. Since Chessnut cannot automatically restore fen history
        # we have to restore it by hand
        game.set_fen(original_fen)
        game.fen_history = fen_history_reference
        fen_history_reference.pop()

    return best_value, best_move

def chess_bot(obs):
    game = Game(obs.board)

    # Limit search depth depending on time left
    # so we don't run out of time
    root_depth = 3 - (obs.remainingOverageTime < 2) - (obs.remainingOverageTime < 1)

    # Perform negamax search with alpha-beta pruning
    value, move = search(root_depth, 0, game, -MAX_EVAL, MAX_EVAL)
    return move
