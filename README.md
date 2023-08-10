# Clover

Clover is a chess engine, written in C++, inspired by my favorite plant. Since the 2020 lockdown, I got bored and decided to combine the 2 things I love the most: chess and programing, into making a program that can beat me.

# Features

Clover now has a page with all the features: https://www.chessprogramming.org/Clover .

# Rating

Clover is and was tested in multiple rating lists (thanks to all testers btw), such as:

- [CCRL 40/15](https://ccrl.chessdom.com/ccrl/4040/) - 3458 4CPU, 3412 1CPU (v5.0, #12)
- [CCRL 2+1](https://ccrl.chessdom.com/ccrl/404/) - 3644 4CPU (v5.0, #11)
- [SP-CC](https://www.sp-cc.de/) - 3629 1CPU (v5.1, #10 if same engine versions are removed)
- [CEGT](http://www.cegt.net/40_4_Ratinglist/40_4_single/rangliste.html) - 3482 (v5.0)
- [IPMAN CHESS](https://ipmanchess.yolasite.com/r9-7945hx.php) - 3419 (v6.0.16, #7 if same engine versions are removed)

There may be other that I omited, if so please bring to my attention =).

# Fathom

Currently, Clover supports Endgame Tablebases, thanks to [Fathom](https://github.com/jdart1/Fathom).

# Usage

Clover is UCI compatible, but doesn't have a GUI, so, in order to play with it, you need a GUI like Arena, Cute chess etc.

# Compiling locally

``` 
git clone https://github.com/lucametehau/CloverEngine.git
cd src
make release 
```

This will create 3 compiles: old, avx2 and avx512. Choose the latest that doesn't crash (if you don't know your PC specs).

To run it's pretty easy:
```
./Clover.3.1-avx2.exe
```

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

- Bench command
```
bench <depth>
```

# Contributing

If one spots a bug or finds an improvement, I'm open to any suggestion.

# Credits

- [Weather-Factory](https://github.com/dsekercioglu/weather-factory) For being a great tuning tool!
- [Open Bench](https://github.com/AndyGrant/OpenBench) For making testing easier!

I inspired myself from:

- [Stockfish](https://github.com/official-stockfish/Stockfish)
- [Ethereal](https://github.com/AndyGrant/Ethereal)
- [Igel](https://github.com/vshcherbyna/igel)
- [Topple](https://github.com/konsolas/ToppleChess)
- [Koivisto](https://github.com/Luecx/Koivisto)
- and of course [ChessProgrammingWiki](https://www.chessprogramming.org/Main_Page)
