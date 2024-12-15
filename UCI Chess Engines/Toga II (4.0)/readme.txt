
Brief Description
------------------------

TOGA II 4.0
- This development of Toga is by Jerry Donald (Watson)
- The code was based on Toga II 3.0 by Fabien Letouzey, Thomas Gaksch, and Jerry Donald Watson
- Thanks to Thomas Gaksch and Fabien Letouzey for the Fruit/TogaII source code.
- Thanks to the computer chess community for the many good ideas which have improved Toga II.
- The IntelPGO compiles should be the fastest. However, if there are problems, GCC and Intel non-PGO compiles are also provided.
- The x64 compile runs about 10% faster than the w32 compile on my machines.

Changes:
- code tidy-up, removed a lot of unused / defunct code and fixed a few bugs.
- improved MP design. Toga II now searches wider (less pruning/reductions) with more threads.
- Toga II can now use up to 16 GB hash and up to 64 threads. 
- fixed aspiration windows, and skip fail-high research during competitive games.
- removed egbb. I plan to add support for Ronald De Man's excellent syzygy bases for the next release.
- new tt-based pruning/reduction algorithm for single-cpu version (see post on Talkchess)
- added eval pruning (also called static null move pruning)
- move count based pruning
- improved null move: reduction (3 + depth/4) from Stockfish used, and double null move avoided
- better usage of hash in quiescent search
- bad captures reduced less
- history aging
- improved history increment
- removed verification search as it cost Elo
- improved mobility evaluation from Fruit reloaded 3.2.1 
- pawn endgame extension from Fruit reloaded 3.2.1
- rook/queen on 7th rank improved
- pawn support bonus added
- knight outpost bonus improved (also depends on no. of pawns) 
- tweak a few evaluation terms
- removed a few "features" that were of no Elo benefit

TOGA II 3.0
- This development of Toga is by Jerry Donald Watson
- The code was based on Toga II 2.0 by Fabien Letouzey, Thomas Gaksch, Jerry Donald Watson and Chris Formula
- Thanks to Denis Mendoza for the compile support
- Thanks to Thomas Gaksch and Fabien Letouzey for the Fruit/TogaII source code
- Thanks to Shaun Brewer for the test support

Changes:
- Multi-PV fixed
- Aspiration windows
- Hash table used in quiescent search (for lookup as well as store)
- LMR improved (thanks to the authors of Stockfish and Protector)
- TT avoid null-move flag
- New Endgame Knowledge
- Improved Mobility Evaluation (Ben Tennison)
- Piece Combo (QN vs QB) adjustment

TOGA II 2.0
- This development of Toga is by Jerry Donald
- The code was based on Toga II 1.4.1SE by Fabien LetouzeyThomas Gaksch and Chris Formula
- Thanks to Denis Mendoza for the compile support
- Thanks to Thomas Gaksch and Fabien Letouzey for the Fruit/TogaII source code
- Thanks to Shaun Brewer for the test support

Changes:
- Futility Margin is now dependent on played move count
- Razoring full_quiescence() bounds fixed (thanks to Lucas Braesch)
- Several eval() bounds fixed
- Improved Pawn Shelter
- Improved Knight Outpost
- Improved Bishop Mobility
- New Endgame Material Adjustments
- Piece Values tweaked
- Egbb bugfix (Teemu Pudas)

TOGA II 1.4.1SE
- This is an experimental engine by Chris Formula
- The code was based on Toga II 1.4beta5c by Thomas Gaksch
- Changes were an extended version of settings implemented in Toga II 3.1.2SE
- Improved search efficiency
- Thanks to Thomas Gaksch and Fabien Letouzey for the Fruit/TogaII source code
- Thanks to Shaun Brewer for the test support
- Thanks to Denis Mendoza for the compile support


TOGA II 1.4 BETA5C
- THIS VERSION is only a TEST VERSION. 
- For mp support a simple shared hashtable is used. MultiPV is not working with this beta.
- The number of threads must be set before compiling in search.h.
- Thanks a lot to Shaun Brewer, Dieter Eberle, Chris Formula, Denis Mendoza, Alessandro Scotti, 
Tord Romstad and much much more. 
- Without their help i wouldn´t be able to increase the playing strength, compared to Fruit 2.1.


Legal Details
-------------------

Toga II 4.0 based on Fruit 2.1 by Fabien Letouzey.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

See the file "copying.txt" for details.













