# slstatus version
VERSION = 0

# customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

#X11INC = /usr/X11R6/include
#X11LIB = /usr/X11R6/lib

# flags
CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Os -g
LDFLAGS  = -s
LDLIBS   = -lX11 -lasound

# compiler and linker
CC = cc
