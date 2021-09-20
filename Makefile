all: bestline_example

bestline_example: bestline.o example.o
	$(CC) $(LDFLAGS) bestline.o example.o -o $@

bestline.o: bestline.c bestline.h Makefile
example.o: example.c bestline.h Makefile

clean:
	rm -f bestline_example bestline.o example.o

check:
	gcc -c -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	g++ -xc++ -c -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	gcc -pedantic -std=c99 -c -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	clang -c -Wall -Wextra -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	clang++ -xc++ -c -Wall -Wextra -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	clang -pedantic -std=c99 -c -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
