
import time,random
from Chessnut import Game


piece_sym = ['K', 'k', 'Q', 'q', 'R', 'r', 'B', 'b', 'N', 'n', 'P', 'p']
piece_val = [0, 0, 100, 100, 50, 50, 30, 30, 30, 30, 10, 10]
piece_val_dict = dict(zip(piece_sym, piece_val))


def evaluate_position(board):
    total_score = 0
    for square in range(64):
        piece = board.get_piece(square)
        if piece != ' ':
            piece_value = piece_val_dict.get(piece.upper(), 0)
            if piece.isupper(): 
                total_score += piece_value # White piece
            else: 
                total_score -= piece_value # Black piece
    return total_score
    

def make_move(move, time_to_start_thinking, is_execute_print=False):
    duration = round(time.time() - time_to_start_thinking,3)
    if is_execute_print == True:
        print(f'[{move}, {duration}]')
    return move


def chess_bot__Step_0002__invisible(obs):
    time_start = time.time()
    game = Game(obs.board)
    moves = list(game.get_moves())

    evas, moves_eva, eva_init = [],[], evaluate_position(game.board)

    # Remove 'random' queen
    
    for move in moves:
        g = Game(obs.board)
        g.apply_move(move)  
        eva_after = evaluate_position(g.board)
        moves_eva.append({
            'move': move,
            'max' : eva_after - eva_init}
        )
        evas.append(eva_after)

    if len(moves_eva) >= 1:
        moves_eva = sorted(moves_eva, key=lambda x: x['max'], reverse=True)
        max_move = moves_eva[0] if moves_eva != None and len(moves_eva) >= 1 else None
        if max_move != None and type(max_move['move'])=='string':
            if max_move['max'] == 100 : return make_move(time_start, max_move['move'])
            if max_move['max'] == 50  : return make_move(time_start, max_move['move'])

    # some step Italian game, debut

    if 'e1g1' in moves: return make_move('e1g1',time_start)
    if 'e8g8' in moves: return make_move('e8g8',time_start)
    if 'e1c1' in moves: return make_move('e1c1',time_start)
    if 'e8c8' in moves: return make_move('e8c8',time_start)
    
    if obs.mark == 'white':
        if obs.step == 0 and 'e2e4' in moves: return make_move('e2e4',time_start)
        if obs.step == 2 and 'f1c4' in moves: return make_move('f1c4',time_start)
        if obs.step == 4 and 'g1f3' in moves: return make_move('g1f3',time_start)

    # exact copy of Bovard Doerschuk-Tiberi
    # 1. Check a subset of moves for checkmate
    for move in moves:
        g = Game(obs.board)
        g.apply_move(move)
        if g.status == Game.CHECKMATE:
            return make_move(move,time_start)

    # 2. Check for captures
    for move in moves:
        if game.board.get_piece(Game.xy2i(move[2:4])) != ' ':
            #print(f'\n{move}\n')
            return make_move(move,time_start)

    # 3. Check for queen promotions
    for move in moves:
        if "q" in move.lower():
            #print(f'\n{move}\n')
            return make_move(move,time_start)

    # 4. Random move if no checkmates or captures

    move = random.choice(moves)
    
    return make_move(move,time_start)
