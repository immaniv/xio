
all: vwrio d.vwrio

vwrio: main.c common.c io.c
	cc -g -o vwrio main.c common.c io.c -lrt  -lpthread -I . 
	strip vwrio

d.vwrio: main.c common.c io.c
	cc -g -o d.vwrio main.c common.c io.c -lrt -lpthread -I . -DDEBUG

clean: 
	rm -f vwrio d.vwrio
