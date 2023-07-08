PROG=MainMenu
CC=clang
CFLAGS=-std=c17 -Wall -Wextra -pedantic -g -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
       -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED

LIBS=`pkg-config --cflags --libs gtk+-3.0` -lgtk-3 -lgdk-3 -lGL -lGLU -lglut -lm

all: $(PROG)

$(PROG): MainMenu.c 
	$(CC) $< -o $@ $(CFLAGS) $(LIBS)

clean: 
	rm -f *.o $(PROG)

