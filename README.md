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

Example:
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

## History

* 2021-07-10: Replaced old console chess with working version of turquoise test engine

## References

* Perft test suite at https://www.chessprogramming.org/Perft_Results
* Perft tests at https://gist.github.com/peterellisjones/8c46c28141c162d1d8a0f0badbc9cff9

