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
This will create an avx2 executable, any CPU that supports avx2 can run it. If you don't have avx2, then it will probably crash.

To run it's pretty easy:
```
./Clover.3.1-avx2.exe
```

# Testing
The only version supported for now is avx2 (any CPU that supports avx2 can run it), because I use simd and avx2 code.
I plan on doing something for the non-avx2 CPU users in the future.

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
