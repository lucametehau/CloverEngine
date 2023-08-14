UNAME=$(shell uname -s)
ifeq ($(UNAME),Linux)
    TARGET=fathom.linux
endif
ifeq ($(UNAME),Darwin)
    TARGET=fathom.macosx
endif
CC?=gcc
STRIP=strip
CFLAGS=-std=gnu99 -O2 -Wall -Wshadow -DTB_NO_THREADS -I..

main: $(TARGET)

fathom.linux:
	$(CC) $(CFLAGS) fathom.c ../tbprobe.c -o fathom.linux
	$(STRIP) fathom.linux

fathom.macosx:
	$(CC) $(CFLAGS) fathom.c ../tbprobe.c -o fathom.macosx
	$(STRIP) fathom.macosx

fathom.windows: CC=x86_64-w64-mingw32-gcc
fathom.windows: STRIP=x86_64-w64-mingw32-strip
fathom.windows:
	$(CC) $(CFLAGS) fathom.c ../tbprobe.c -o fathom.exe
	$(STRIP) fathom.exe

INSTALL=Fathom-1.0
PACKAGE=Fathom-1.0.zip
release:
	mkdir $(INSTALL)
	cp fathom.linux $(INSTALL)
	cp fathom.macosx $(INSTALL)
	cp fathom.exe $(INSTALL)
	zip -r $(PACKAGE) $(INSTALL)
	rm -rf $(INSTALL)

