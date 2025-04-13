# Clover

Clover is a chess engine, written in C++, inspired by my favorite plant. Since the 2020 lockdown, I got bored and decided to combine the 2 things I love the most: chess and programing, into making a program that can beat me.

# Features

Clover now has a page with all the features: [https://www.chessprogramming.org/Clover](https://www.chessprogramming.org/Clover).

# Rating

Clover is and was tested in multiple rating lists (thanks to all testers btw), such as:

- [CCRL 40/15](https://computerchess.org.uk/ccrl/4040/) - 3577 4CPU, 3551 1CPU (v6.2, #12)
- [CCRL 2+1](https://computerchess.org.uk/ccrl/404/) - 3726 8CPU (v6.2, #16)
- [SP-CC](https://www.sp-cc.de/) - 3601 1CPU (v6.2, #10 if same engine versions are removed)
- [CEGT](http://www.cegt.net/40_4_Ratinglist/40_4_single/rangliste.html) - 3532 (v6.1)
- [IPMAN CHESS](https://ipmanchess.yolasite.com/r9-7945hx.php) - 3468 (v6.2, #7 if same engine versions are removed)

There may be other that I omited, if so please bring to my attention =).

# Fathom

Currently, Clover supports Endgame Tablebases, thanks to [Fathom](https://github.com/jdart1/Fathom).

# Usage

Clover is UCI compatible, but doesn't have a GUI, so, in order to play with it, you need a GUI like Arena, Cute chess etc.

# Compiling locally

``` 
git clone https://github.com/lucametehau/CloverEngine.git
cd CloverEngine/src
make release 
```

This will create 3 compiles: old, avx2 and avx512. Choose the latest that doesn't crash (if you don't know your PC specs).

To run it's pretty easy:
```
./Clover.6.2-avx2.exe
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

- [Weather-Factory](https://github.com/dsekercioglu/weather-factory), for being a great tuning tool!
- [OpenBench](https://github.com/AndyGrant/OpenBench), for making testing easier!
- [Bullet](https://github.com/jw1912/bullet), for making net training fast and easy!

I inspired myself from:

- [Stockfish](https://github.com/official-stockfish/Stockfish)
- [Ethereal](https://github.com/AndyGrant/Ethereal)
- [Igel](https://github.com/vshcherbyna/igel)
- [Topple](https://github.com/konsolas/ToppleChess)
- [Koivisto](https://github.com/Luecx/Koivisto)
- and of course [ChessProgrammingWiki](https://www.chessprogramming.org/Main_Page)
