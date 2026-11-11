VERSION = 0.1
PREFIX  = /usr/local
CC      = cc

CFLAGS  = -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os
LDFLAGS = -lX11

DBG_CFLAGS  = -std=c99 -O0 -g3 \
              -Wall -Wextra -Werror -Wpedantic \
              -Wshadow -Wconversion -Wsign-conversion \
              -Wstrict-prototypes -Wmissing-prototypes \
              -fsanitize=address,undefined
DBG_LDFLAGS = -lX11 -fsanitize=address,undefined
