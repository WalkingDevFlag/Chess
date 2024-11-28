import numpy as np
import torch
from game import TicTacToe
from MCTS import MCTS
from MCTS import Node
from ResNet import ResNet 

tictactoe = TicTacToe()
args = {'C': 2, 'num_searches': 1000}
model = ResNet(tictactoe, 4, 64)
model.eval()
mcts = MCTS(tictactoe, args, model)

state = tictactoe.get_initial_state()
player = 1

while True:
    tictactoe.render_board(state)  # Display the board with X, O, and _
    
    if player == 1:
        valid_moves = tictactoe.get_valid_moves(state)
        print(f"Valid Moves: {np.argwhere(valid_moves).flatten()}")
        action = int(input("Enter Action: "))
    else:
        action_probs = mcts.search(state)
        action = np.argmax(action_probs)
    
    state = tictactoe.get_next_state(state, action, player)
    value, is_terminated = tictactoe.get_value_and_terminated(state, action)
    if is_terminated:
        tictactoe.render_board(state)  # Show the final state
        print("Game Over")
        break
    
    player = tictactoe.get_opponent(player)




'''
import numpy as np
import torch
from game import TicTacToe
from MCTS import MCTS
from MCTS import Node
from ResNet import ResNet 

tictactoe = TicTacToe()
args = {'C': 2, 'num_searches': 1000}
model = ResNet(tictactoe, 4, 64)
model.eval()
mcts = MCTS(tictactoe, args, model)

state = tictactoe.get_initial_state()
player = 1

while True:
    print(state)
    if player == 1:
        valid_moves = tictactoe.get_valid_moves(state)
        print("Valid moves:", [i for i in range(tictactoe.action_size) if valid_moves[i] == 1])
        action = int(input(f"Player {player}: "))

        if valid_moves[action] == 0:
            print("Invalid move")
            continue
    else:
        neutral_state = tictactoe.change_perspective(state, player)
        mcts_probs = mcts.search(neutral_state)
        action = np.argmax(mcts_probs)
        
    state = tictactoe.get_next_state(state, action, player)
    value, is_terminal = tictactoe.get_value_and_terminated(state, action)
    
    if is_terminal:
        print(state)
        if value == 1:
            print(f"Player {player} won!")
        else:
            print("It's a draw!")
        break
    
    player = tictactoe.get_opponent(player)
'''
