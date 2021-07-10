# console-chess

Simplistic chess implementation using only Unicode chess font graphics for testing of reinforcement algorithms

## Dependencies

None. Console font needs to support Unicode characters for chess.

## Build

`console-chess` uses the `cmake` build system:

```bash
mkdir Build
cd Build

cmake ..
make
```

## `perft` test-suite

Run all tests:
```
perft/perft
```

This verifies with a number of positions that have been proven to be tricky with engines,
if the move generator works according to all chess rules.

The number of generated moves at a given depth is compared to a reference value.

Since all tests pass, there is a very high probability that the move generator is ok.

Example (fen: "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"), see `perft/perft.cpp`
for more.

![](https://raw.githubusercontent.com/domschl/console-chess/master/doc/resources/strangebugs.png)
```
Active: White
Castle-rights: White-King White-Queen
No enpassant
Fifty-move-state: 1
Move-number: 8
Depth: 1 2 3 4 2103487 nodes, 8867 knps 5 89941194 nodes, 11261 knps
```
130 tests later:
```
OK: 130 Error: 0
```

## Play simple game:

```
turquoise/turquoise
```
(Simply enter UCI-formatted moves for white, e.g. `e2-e2`.)

## Sample output

Search for mate:
```
turquoise/turquoise -f "3K4/3b4/4p2k/8/8/4R3/8/1R6 w -"
```

![](https://raw.githubusercontent.com/domschl/console-chess/master/doc/resources/pos1.png)

```
Best line, depth=1/6, nodes=949: d8-d7 (3600) h6-g6 (-3600)
Best line, depth=2/9, nodes=5,423: d8-d7 (4125) h6-g6 (-3600)
Best line, depth=3/11, nodes=48,428: d8-d7 (4125) h6-g6 (-3600)
Best line, depth=4/10, nodes=62,812: b1-g1 (4122) d7-a4 (-2400) g1-g4 (2400) a4-e8 (-2400)
b1-g1 (2147483645)

Best line, depth=1/5, nodes=386: e6-e5 (-2400) d8-d7 (3600)
Best line, depth=2/7, nodes=2,450: d7-e8 (-3707) d8-e8 (3600)
Best line, depth=3/9, nodes=20,098: d7-e8 (-3707) d8-e8 (3600)
d7-e8 (-2147483645)

Best line, depth=1/6, nodes=1,391: d8-e8 (3600) h6-h5 (-3600)
Best line, depth=2/6, nodes=1,497:
d8-e8 (2147483645)

Best line, depth=1/5, nodes=129:
e6-e5 (-2147483645)

Best line, depth=1/6, nodes=317:
e3-h3 (2147483645)

Best line, depth=1/0, nodes=0:
Game over!
```

![](https://raw.githubusercontent.com/domschl/console-chess/master/doc/resources/pos2.png)

### The Indian problem (mate in 4)
The first published chess puzzle was the 'Indian Problem'. It was printed in The Chess Player's Chronicle, in 1845. 

Start with Fen: `turquoise/turquoise -f "8/8/1p5B/1p2p3/4k1P1/1P3n2/P4PB1/K2R4 w - - 0 1"`

![](https://raw.githubusercontent.com/domschl/console-chess/master/doc/resources/indian-problem-mate-4.png)

```
Best line, depth=1/8, nodes=660: d1-e1 (3640) e4-d5 (-3640)
Best line, depth=2/11, nodes=5,008: d1-e1 (4994) e4-d4 (-3640)
Best line, depth=3/13, nodes=30,693: d1-d6 (4229) b5-b4 (-3640)
Best line, depth=4/14, nodes=215,149: d1-e1 (5100) e4-d4 (-3640)
Best line, depth=5/18, nodes=1,328,816: d1-e1 (5509) e4-d3 (-3640)
Best line, depth=6/20, nodes=3,054,985: d1-d6 (5399) b5-b4 (-3640) d6-g6 (3640) e4-d4 (-3640) h6-e3 (3640) d4-c3 (-3640) e3-b6 (4040) c3-c2 (-4040) g2-f3 (5200)
d1-d6 (2147483645)
...
b5-b4, h6-c1, b6-b5, d6-d2, e4-f4, d2-d4#
```

![](https://raw.githubusercontent.com/domschl/console-chess/master/doc/resources/indian-problem-end.png)

## History

* 2021-07-10: Replaced old console chess with working version of turquoise test engine

## References

* Perft test suite at https://www.chessprogramming.org/Perft_Results
* Perft tests at https://gist.github.com/peterellisjones/8c46c28141c162d1d8a0f0badbc9cff9
* Indian problem https://www.chess.com/forum/view/more-puzzles/indian-problem-mate-in-4

