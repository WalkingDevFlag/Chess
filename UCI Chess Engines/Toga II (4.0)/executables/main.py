

import subprocess


class ChessEngine:
    def __init__(self, engine_path):
        # Start the engine process
        self.engine = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        self._initialize_engine()

    def _initialize_engine(self):
        # Initialize the engine with UCI protocol
        self._send_command("uci")
        while True:
            output = self._read_output()
            if output == "uciok":
                break

        # Set engine options to minimize memory usage
        self._send_command("setoption name Hash value 1")

    def _send_command(self, command):
        """Send a command to the engine."""
        self.engine.stdin.write(command + "\n")
        self.engine.stdin.flush()

    def _read_output(self):
        """Read a single line of output from the engine."""
        output = self.engine.stdout.readline().strip()
        return output

    def get_best_move(self, fen, movetime=100):
        """Get the best move for a given position."""
        # Set the position
        self._send_command(f"position fen {fen}")

        # Start the search
        self._send_command(f"go movetime {movetime}")

        # Wait for the best move
        best_move = None
        while True:
            output = self._read_output()
            if output.startswith("bestmove"):
                best_move = output.split()[1]
                break

        # Clear the engine's internal cache to minimize memory usage
        self._send_command("setoption name Clear Hash")

        return best_move

    def stop(self):
        """Stop the engine process."""
        self._send_command("quit")
        self.engine.terminate()
        self.engine.wait()


# Define a global variable to store the ChessEngine instance
ultima = None

def chess_bot(obs):
    global ultima  # Declare ultima as global to modify it
    fen = obs['board']

    engine_path = '/mnt/e/Random Python Scripts/kaggle chess/TogaII40/toga' # for Ubuntu

    if ultima is None:
        ultima = ChessEngine(engine_path)

    # Get the best move from the engine
    best_move = ultima.get_best_move(fen)

    return best_move
'''
def main():
    fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    obs = {"board": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"}
    best_move = chess_bot(obs)
    print(f"Best move: {best_move}")

if __name__ == '__main__':
    main()
'''
