# Clover

Clover is a chess engine, written in C++, inspired by my favorite plant. Since the 2020 lockdown, I got bored and decided to combine the 2 things I love the most: chess and programing, into making a program that can beat me.

# Features

Clover now has a page with all the features: https://www.chessprogramming.org/Clover .

# Fathom

Currently, Clover supports Endgame Tablebases, thanks to [Fathom](https://github.com/jdart1/Fathom).

# Usage

Clover is UCI compatible, but doesn't have a GUI, so, in order to play with it, you need a GUI like Arena, Cute chess etc.

# Compiling locally

``` 
git clone https://github.com/lucametehau/CloverEngine.git
make run 
```
This will create 5 executables and you can choose which one fits your PC the best.
For the best performance, you should try ``make native``, because it uses all compiler flags supported by your PC.

To run it's pretty easy:
```
./Clover.2.2-bmi2.exe
```

# Testing

These are the versions sorted by speed, the ones at the top being the fastest:

- -native, use this if it doesn't crash and if you have at least the same CPU specs as my I5 8250U :)
- -bmi2, supported on any CPU that supports bmi2 and popcnt
- -avx2, supported on any CPU that supports avx2 and popcnt
- -popcnt, supported by most non-ancient CPUs
- normal version, no additional compile flags

# UCI

Setting thread count, Syzygy Path and Hash size is available.

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
bench
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
