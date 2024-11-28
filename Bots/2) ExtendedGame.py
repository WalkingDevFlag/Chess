from Chessnut import Game

def chess_bot(obs):
    """
    We will save ExtendedGame as part of the chess_bot function. This only needs to be done once.
    See the references at the bottom of this workbook for a link about how to do this.
    """
    if not hasattr(chess_bot, "extend"):
        """
        Define the ExtendedGame class
        """
        class ExtendedGame(Game): # We are creating a new class with the Game class as an argument
            def __init__(self, fen, validate=True):
                # This calls the parent class's initializer
                super().__init__(fen, validate)
                """
                Create a property of ExtendedGame that is a List. Here we'll store data about taking 
                an opponents piece. 
                """
                self.potential_takes = []

            # To redefine the _trace_ray method of Game, we restate its name
            def _trace_ray(self, start, piece, ray, player):

                res_moves = []
        
                for end in ray:
        
                    sym = piece.lower()
                    del_x = abs(end - start) % 8
                    move = [Game.i2xy(start) + Game.i2xy(end)]
                    tgt_owner = self.board.get_owner(end)
        
                    if tgt_owner == player:
                        break
        
                    if sym == 'k' and del_x == 2:
                        gap_owner = self.board.get_owner((start + end) // 2)
                        out_owner = self.board.get_owner(end - 1)
                        rights = {62: 'K', 58: 'Q', 6: 'k', 2: 'q'}.get(end, ' ')
                        if (tgt_owner or gap_owner or rights not in self.state.rights or
                                (rights.lower() == 'q' and out_owner)):
                            # Abort castling because missing castling rights
                            # or piece in the way
                            break
        
                    if sym == 'p':
                        if del_x == 0 and tgt_owner:
                            break
        
                        elif del_x != 0 and not tgt_owner:
                            ep_coords = self.state.en_passant
                            if ep_coords == '-' or end != Game.xy2i(ep_coords):
                                break
        
                        if (end < 8 or end > 55):
                            move = [move[0] + s for s in ['b', 'n', 'r', 'q']]
        
                    res_moves.extend(move)
        
                    # break after capturing an enemy piece
                    if tgt_owner:

                        """
                        All of Chessnut's _trace_ray code above is unchanged. At this conditional
                        statement, we add two lines of code to collect data about moves that could 
                        result in capturing an opponent's piece. Note "could", because these moves 
                        are not necessarily valid. To keep things simple, we'll handle validation  
                        outside of the Game class.
                        """
                        take_data = [move[-1], piece, self.board.get_piece(end)] # The move and the pieces
                        self.potential_takes.append(take_data) # Add to the 'potential_takes' property List
                        
                        break
        
                return res_moves        
        
            def get_potential_takes(self):
                return self.potential_takes

        chess_bot.extend = ExtendedGame
    
    """
    Instead of using Chessnut's Game here, we use the new ExtendedGame, which is stored at chess_bot.extend
    """
    game = chess_bot.extend(obs.board)
    moves = list(game.get_moves())
    
    """
    Validate takes
    """
    potential_takes = game.get_potential_takes()
    valid_takes = [take for take in potential_takes if take[0] in moves]

    # Sort valid_takes by value of opponent's piece
    def opponent_piece_value(take):
        return {'q': 9, 'r': 5, 'n': 3, 'b': 3, 'p': 1}[take[2].lower()]
    sorted_takes = sorted(valid_takes, key = opponent_piece_value, reverse = True)
    print(sorted_takes)

    # Take highest value piece if possible; otherwise just do the first move in the moves List
    if len(sorted_takes) > 0: return sorted_takes[0][0]
    return moves[0]
