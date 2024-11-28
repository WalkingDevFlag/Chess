import random
import time

# Chess piece representations
EMPTY = '.'
PAWN = 'P'
KNIGHT = 'N'
BISHOP = 'B'
ROOK = 'R'
QUEEN = 'Q'
KING = 'K'

class ChessBoard:
    def __init__(self):
        self.board = [
            ['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'],
            ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'],
            ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R']
        ]
        self.current_player = 'white'

    def get_legal_moves(self):
        moves = []
        for i in range(8):
            for j in range(8):
                piece = self.board[i][j]
                if piece.isupper() == (self.current_player == 'white'):
                    moves.extend(self.get_piece_moves(i, j))
        return moves

    def get_piece_moves(self, row, col):
        piece = self.board[row][col].upper()
        moves = []
        
        if piece == PAWN:
            direction = -1 if self.current_player == 'white' else 1
            if 0 <= row + direction < 8 and self.board[row + direction][col] == EMPTY:
                moves.append((row, col, row + direction, col))
        elif piece == KNIGHT:
            knight_moves = [(-2, -1), (-2, 1), (-1, -2), (-1, 2), (1, -2), (1, 2), (2, -1), (2, 1)]
            for dr, dc in knight_moves:
                r, c = row + dr, col + dc
                if 0 <= r < 8 and 0 <= c < 8:
                    moves.append((row, col, r, c))
        # Add more piece move logic here for other pieces

        return moves

    def make_move(self, move):
        from_row, from_col, to_row, to_col = move
        self.board[to_row][to_col] = self.board[from_row][from_col]
        self.board[from_row][from_col] = EMPTY
        self.current_player = 'black' if self.current_player == 'white' else 'white'

    def evaluate(self):
        piece_values = {PAWN: 1, KNIGHT: 3, BISHOP: 3, ROOK: 5, QUEEN: 9, KING: 0}
        score = 0
        for row in self.board:
            for piece in row:
                if piece.isupper():
                    score += piece_values.get(piece.upper(), 0)
                elif piece.islower():
                    score -= piece_values.get(piece.upper(), 0)
        return score

class EfficientChessAI:
    def __init__(self):
        self.max_depth = 3

    def get_move(self, board):
        best_move = None
        best_value = float('-inf')
        alpha = float('-inf')
        beta = float('inf')
        
        for move in board.get_legal_moves():
            board.make_move(move)
            value = self.minimax(board, self.max_depth - 1, alpha, beta, False)
            board.make_move(move[::-1])  # Undo move
            
            if value > best_value:
                best_value = value
                best_move = move
            
            alpha = max(alpha, value)
            if alpha >= beta:
                break
        
        return best_move

    def minimax(self, board, depth, alpha, beta, maximizing_player):
        if depth == 0:
            return board.evaluate()
        
        if maximizing_player:
            max_eval = float('-inf')
            for move in board.get_legal_moves():
                board.make_move(move)
                eval = self.minimax(board, depth - 1, alpha, beta, False)
                board.make_move(move[::-1])  # Undo move
                max_eval = max(max_eval, eval)
                alpha = max(alpha, eval)
                if beta <= alpha:
                    break
            return max_eval
        else:
            min_eval = float('inf')
            for move in board.get_legal_moves():
                board.make_move(move)
                eval = self.minimax(board, depth - 1, alpha, beta, True)
                board.make_move(move[::-1])  # Undo move
                min_eval = min(min_eval, eval)
                beta = min(beta, eval)
                if beta <= alpha:
                    break
            return min_eval

def agent(observation, configuration):
    board = ChessBoard()
    # Update board state based on observation
    # This part depends on how the observation is structured in the Kaggle environment
    
    ai = EfficientChessAI()
    move = ai.get_move(board)
    
    # Convert move to the format expected by the Kaggle environment
    # This part also depends on how moves should be returned in the Kaggle environment
    return f"{chr(move[1] + 97)}{8 - move[0]}{chr(move[3] + 97)}{8 - move[2]}"

# Configuration for the Kaggle environment
config = {
    "timeout": 1,
    "memory": 1024 * 1024 * 1024,  # 1GB
}