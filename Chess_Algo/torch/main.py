from chess import Board, pgn
import torch
import pickle
import numpy as np
import torch.nn as nn

def board_to_matrix(board: Board):
    # 8x8 is the size of the chess board.
    # 12 = number of unique pieces.
    # 13th board for legal moves (WHERE we can move)
    matrix = np.zeros((13, 8, 8))
    piece_map = board.piece_map()

    # Populate first 12 8x8 boards (where pieces are)
    for square, piece in piece_map.items():
        row, col = divmod(square, 8)
        piece_type = piece.piece_type - 1
        piece_color = 0 if piece.color else 6
        matrix[piece_type + piece_color, row, col] = 1

    # Populate the legal moves board (13th 8x8 board)
    legal_moves = board.legal_moves
    for move in legal_moves:
        to_square = move.to_square
        row_to, col_to = divmod(to_square, 8)
        matrix[12, row_to, col_to] = 1

    return matrix

class ChessModel(nn.Module):
    def __init__(self, num_classes):
        super(ChessModel, self).__init__()
        self.conv1 = nn.Conv2d(13, 64, kernel_size=3, padding=1)
        self.conv2 = nn.Conv2d(64, 128, kernel_size=3, padding=1)
        self.flatten = nn.Flatten()
        self.fc1 = nn.Linear(8 * 8 * 128, 256)
        self.fc2 = nn.Linear(256, num_classes)
        self.relu = nn.ReLU()

        # Initialize weights
        nn.init.kaiming_uniform_(self.conv1.weight, nonlinearity='relu')
        nn.init.kaiming_uniform_(self.conv2.weight, nonlinearity='relu')
        nn.init.xavier_uniform_(self.fc1.weight)
        nn.init.xavier_uniform_(self.fc2.weight)

    def forward(self, x):
        x = self.relu(self.conv1(x))
        x = self.relu(self.conv2(x))
        x = self.flatten(x)
        x = self.relu(self.fc1(x))
        x = self.fc2(x)  # Output raw logits
        return x

'''
def prepare_input(board: Board):
    matrix = board_to_matrix(board)
    X_tensor = torch.tensor(matrix, dtype=torch.float32).unsqueeze(0)
    return X_tensor
'''

def prepare_input(fen):
    # Check if the input is a Struct object and extract the FEN string
    if not isinstance(fen, str):
        # Assuming the FEN string is stored as `fen.board` in the Struct
        fen = fen.board  # Access the FEN string from the Struct object

    board = Board(fen=fen)
    matrix = board_to_matrix(board)
    X_tensor = torch.tensor(matrix, dtype=torch.float32).unsqueeze(0)
    return X_tensor, board

# Load the mapping
with open(r"move_to_int", "rb") as file:
    move_to_int = pickle.load(file)

# Check for GPU
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f'Using device: {device}')

# Load the model
model = ChessModel(num_classes=len(move_to_int))
model.load_state_dict(torch.load('/kaggle/input/test/pytorch/default/1/TORCH_100EPOCHS.pth'))
model.to(device)
model.eval()  # Set the model to evaluation mode

int_to_move = {v: k for k, v in move_to_int.items()}

# Function to make predictions
def predict_move(fen: str):
    # Prepare input
    X_tensor, board = prepare_input(fen)
    X_tensor = X_tensor.to(device)
    
    with torch.no_grad():
        logits = model(X_tensor)
    
    logits = logits.squeeze(0)  # Remove batch dimension
    
    # Convert logits to probabilities
    probabilities = torch.softmax(logits, dim=0).cpu().numpy()
    
    # Get legal moves
    legal_moves = list(board.legal_moves)
    legal_moves_uci = [move.uci() for move in legal_moves]
    
    # Sort moves by predicted probability
    sorted_indices = np.argsort(probabilities)[::-1]
    
    for move_index in sorted_indices:
        move = int_to_move[move_index]
        if move in legal_moves_uci:
            return move  # Return the best legal move based on the model
    
    return None 
