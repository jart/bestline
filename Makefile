all: bestline_example

bestline_example: bestline.o example.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

bestline.o: bestline.c bestline.h
example.o: example.c bestline.h

clean:
	rm -f bestline_example bestline.o example.o
