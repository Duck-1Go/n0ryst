all: n0ryst

n0ryst: src/n0ryst.c
	gcc -o n0ryst src/n0ryst.c

clean:
	rm -f n0ryst *.o out.asm N0roshi

.PHONY: all clean
