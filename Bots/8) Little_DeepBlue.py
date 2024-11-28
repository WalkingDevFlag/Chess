
import time
from Chessnut import Game
import random

def Little_DeepBlue(obs):
    """
    A heuristic-based chess bot that prioritizes:
    - Checkmates
    - Captures
    - Promotions
    - Random moves
    Args:
        obs: Object with 'board' representing board state as FEN string
    Returns:
        Move in UCI notation
    """
    game = Game(obs.board)
    moves = list(game.get_moves())
    random.shuffle(moves)  # Randomize moves to add variation

    # Prioritize checkmates
    for move in moves[:10]:
        g = Game(obs.board)
        g.apply_move(move)
        if g.status == Game.CHECKMATE:
            return move

    # Check for captures
    for move in moves:
        if game.board.get_piece(Game.xy2i(move[2:4])) != ' ':
            return move

    # Check for promotions
    for move in moves:
        if "q" in move.lower():  # Queen promotion
            return move

    # Default to random move
    return random.choice(moves)
