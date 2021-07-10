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

Run `perft` tests with:


```
perft/perft
```

Play simple game:

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

```

## History

* 2021-07-10: Replaced old console chess with working version of turquoise test engine
