import Chessnut
from Chessnut import Game
import random
from random import sample
import time

piece_sym = ['K', 'k', 'Q', 'q', 'R', 'r', 'B', 'b', 'N', 'n', 'P', 'p']
piece_val = [0, 0, 9, 9, 5, 5, 3, 3, 3, 3, 1, 1]
piece_val_dict = dict(zip(piece_sym, piece_val))

def evaluate_position(board):
    total_score = 0
    for square in range(64):
        piece = board.get_piece(square)
        if piece != ' ':
            piece_value = piece_val_dict.get(piece.upper(), 0)
            if piece.isupper():  # White piece
                total_score += piece_value
            else:  # Black piece
                total_score -= piece_value
    return total_score


# Minimax algorithm with alpha-beta pruning to evaluate and choose the best move for the bot.
def minimax(game, depth, maximizing_player, alpha=float('-inf'), beta=float('inf'), start_time=None, time_limit=None):
    if game.status in [Game.CHECKMATE, Game.STALEMATE]:
        return 0, None
    elif depth == 0:
        return evaluate_position(game.board), None
    elif start_time and time_limit and (time.time() - start_time) > time_limit:
        return evaluate_position(game.board), None

    # Limiting possible moves here, to save on search time.
    moves = sample(list(game.get_moves()), len(list(game.get_moves())) // 10)[:10] 
    best_move = None

    if maximizing_player:
        max_eval = float('-inf')
        for move in moves:
            g = Game(game.get_fen())
            g.apply_move(move)
            eval_score, _ = minimax(g, depth - 1, False, alpha, beta, start_time, time_limit)
            if eval_score > max_eval:
                max_eval = eval_score
                best_move = move
            alpha = max(alpha, eval_score)
            if beta <= alpha:
                break  # Beta cut-off
        return max_eval, best_move
    else:
        min_eval = float('inf')
        for move in moves:
            g = Game(game.get_fen())
            g.apply_move(move)
            eval_score, _ = minimax(g, depth - 1, True, alpha, beta, start_time, time_limit)
            if eval_score < min_eval:
                min_eval = eval_score
                best_move = move
            beta = min(beta, eval_score)
            if beta <= alpha:
                break  # Alpha cut-off
        return min_eval, best_move


default_fen = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'


def chess_bot(obs):
    # Set start_time
    start_time = time.time()
    time_limit = 0.15
    
    # Get FEN string from the Board object
    game = Game(obs.board)
    moves_full = list(game.get_moves())
    moves_sample = sample(moves_full, len(list(game.get_moves())))[:10]
    
    # 1. Try to detect checkmate
    for move in moves_full:
        g = Game(obs.board)
        g.apply_move(move)
        if g.status == Game.CHECKMATE:
            return move

    # 2. Check for captures of higher value piece using a lower value piece
    highest_val_capture = ''
    highest_val = 0
    for move in moves_full:
        moving_piece_val = piece_val_dict.get(game.board.get_piece(Game.xy2i(move[:2])), 0)
        captured_piece_val = piece_val_dict.get(game.board.get_piece(Game.xy2i(move[2:4])), 0)
        if captured_piece_val == moving_piece_val == 9:
            return move
        elif (captured_piece_val > moving_piece_val) and (captured_piece_val >= highest_val):
            highest_val_capture = move
            highest_val = captured_piece_val
    
    if highest_val_capture != '' or highest_val > 1:
        return highest_val_capture

    # 3. Check for captures
    for move in moves_sample:
        if game.board.get_piece(Game.xy2i(move[2:4])) != ' ':
            return move
    
    # 4. Use minimax algorithm with a depth of 1 to find the best move
    best_move = None
    best_eval = float('-inf')
    
    for move in moves_full:
        elapsed_time = time.time() - start_time
        if elapsed_time > float(time_limit):
            break
        
        g = Game(obs.board)
        g.apply_move(move)
        eval_score, _ = minimax(g, depth=1, maximizing_player=True, start_time=start_time, time_limit=time_limit)

        if eval_score > best_eval:
            best_eval = eval_score
            best_move = move
    
    if best_move is not None:
        return best_move
    else:
        return random.choice(moves_full)
