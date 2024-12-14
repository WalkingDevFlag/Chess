from Chessnut import Game
import random

def chess_bot(obs):
    """
    Enhanced chess bot that prioritizes capturing pieces based on their weight:

    - Queen (highest weight)
    - Rook
    - Bishop
    - Knight
    - Pawn (lowest weight)
    - Prioritizes checkmates, queen promotions, weighted captures, and random moves.

    Args:
        obs: An object with a 'board' attribute representing the current board state as a FEN string.

    Returns:
        A string representing the chosen move in UCI notation (e.g., "e2e4")
    """

    # 0. Parse the current board state and generate legal moves
    game = Game(obs.board)
    moves = list(game.get_moves())

    # 1. Check for checkmate (highest priority)
    for move in moves[:10]:
        g = Game(obs.board)
        g.apply_move(move)
        if g.status == Game.CHECKMATE:
            return move

    # 2. Prioritized captures based on piece weight
    piece_weights = {'Q': 9, 'R': 5, 'B': 3, 'N': 3, 'P': 1}  # Higher weight for more valuable pieces
    for move in moves:
        captured_piece = game.board.get_piece(Game.xy2i(move[2:4]))
        if captured_piece != ' ':  # Capture detected
            return move

    # 3. Check for queen promotions
    for move in moves:
        if "q" in move.lower():
            return move

    # 4. Random move with slight preference for captures (if no checkmates, captures, or promotions)
    capture_moves = [move for move in moves if game.board.get_piece(Game.xy2i(move[2:4])) != ' ']
    if capture_moves:  # If there are captures available, prioritize them slightly
        return random.choice(capture_moves)
    else:
        return random.choice(moves)