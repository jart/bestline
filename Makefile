all: bestline_example

bestline_example: bestline.o example.o
	$(CC) $(LDFLAGS) bestline.o example.o -o $@

bestline.o: bestline.c bestline.h
example.o: example.c bestline.h

clean:
	rm -f bestline_example bestline.o example.o
