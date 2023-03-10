CFLAGS = -std=gnu11 -D_GNU_SOURCE
LIBS = -lm
SOURCES = main.c interface.c vmm.c
OUT = proj3

default:
	gcc $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
debug:
	gcc -g $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
clean:
	rm $(OUT)
