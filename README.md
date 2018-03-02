# console-chess

Simplistic chess implementation using only Unicode chess font graphics for testing of reinforcement algorithms

## Dependencies

None. Console font needs to support Unicode characters for chess.

## Build

Travis CI status: [![Build Status](https://travis-ci.org/domschl/console-chess.svg?branch=master)](https://travis-ci.org/domschl/console-chess)

`console-chess` uses the `cmake` build system:

```bash
mkdir Build
cd Build

cmake ..
make
chess/chess
```

## Sample output

![chess output](doc/images/chess.png)
