# Clover

Clover is a chess engine, written in C++, inspired by my favorite plant. Since the 2020 lockdown, I got bored and decided to combine the 2 things I love the most: chess and programing, into making a program that can beat me.

# Features

Clover now has a page with all the features: https://www.chessprogramming.org/Clover .

# Fathom

Currently, Clover supports Endgame Tablebases, thanks to [Fathom](https://github.com/jdart1/Fathom).

# Usage

Clover is UCI compatible, but doesn't have a GUI, so, in order to play with it, you need a GUI like Arena, Cute chess etc.

# Compiling locally

If you want to compile locally, I recommend you to use:

```g++ *.cpp *.c -march=native -Wall -O3 -o output.exe -std=c++17```

without ```-flto```, because it crashes for some strange reason.

If you are compiling on Linux, you need to add ```-pthread``` to your command line.

In the future, I will add a makefile too.

# Testing

These are the versions sorted by speed, the ones at the top being the fastest:

- -bmi2, supported on any CPU that supports bmi2
- -popcnt-avx2, supported on any CPU that supports avx2
- -popcnt, supported by most non-ancient CPUs
- normal version, no additional compile flags

# UCI

Setting thread count, Syzygy Path and Hash size is now available.

Additional UCI commands:

- Perft command (after setting position)

```
perft <depth>
```

- Eval command (after setting position)

```
eval
```

- Tune command
```
tune <nrThreads> <path for positions>
```

- Bench command
```
bench <path for positions>
```

# Contributing

If one spots a bug or finds an improvement, I'm open to any suggestion.

# Credits

I inspired myself from:

- [Stockfish](https://github.com/official-stockfish/Stockfish)
- [Ethereal](https://github.com/AndyGrant/Ethereal)
- [Igel](https://github.com/vshcherbyna/igel)
- [Topple](https://github.com/konsolas/ToppleChess)
- and of course [ChessProgrammingWiki](https://www.chessprogramming.org/Main_Page)
