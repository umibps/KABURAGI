lcairo.so: lcairo.o
	gcc -O -shared -fPIC -o lcairo.so \
	`pkg-config --libs lua5.1` \
	`pkg-config --libs cairo` \
	`pkg-config --libs gtk+-2.0` \
	`pkg-config --libs gdk-2.0` \
	lcairo.o

lcairo.o: lcairo.c
	gcc -Wall -O2 -fPIC -c \
	`pkg-config --cflags lua5.1` \
	`pkg-config --cflags cairo` \
	`pkg-config --cflags gtk+-2.0` \
	`pkg-config --cflags gdk-2.0` \
	lcairo.c

clean: 
	rm -f lcairo.so lcairo.o
