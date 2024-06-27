all: bestline_example bestline_multi

bestline_example: bestline.o example.o
	$(CC) $(LDFLAGS) bestline.o example.o -o $@

bestline_multi: bestline.o multi.o
	$(CC) $(LDFLAGS) bestline.o multi.o -o $@

bestline.o: bestline.c bestline.h Makefile
example.o: example.c bestline.h Makefile
multi.o: multi.c bestline.h Makefile

libbestline.so: bestline.c
	$(CC) $(LDFLAGS) -fPIC -shared bestline.c -o $@

clean:
	rm -f bestline_example bestline.o example.o bestline_example.com bestline_example.com.dbg multi.o bestline_multi libbestline.so

################################################################################
# compile on linux the demo as a binary that runs on seven operating systems

bestline_example.com: bestline_example.com.dbg
	objcopy -S -O binary $< $@
bestline_example.com.dbg: bestline.c example.c crt.o ape.o ape.lds cosmopolitan.a cosmopolitan.h
	gcc -g -Os -static -fno-pie -no-pie -mno-red-zone -nostdlib -nostdinc -fno-omit-frame-pointer -pg -mnop-mcount -o $@ bestline.c example.c -Wl,--gc-sections -fuse-ld=bfd -Wl,-T,ape.lds -include cosmopolitan.h crt.o ape.o cosmopolitan.a
bestline_multi.com: bestline_multi.com.dbg
	objcopy -S -O binary $< $@
bestline_multi.com.dbg: bestline.c multi.c crt.o ape.o ape.lds cosmopolitan.a cosmopolitan.h
	gcc -g -Os -static -fno-pie -no-pie -mno-red-zone -nostdlib -nostdinc -fno-omit-frame-pointer -pg -mnop-mcount -o $@ bestline.c example.c -Wl,--gc-sections -fuse-ld=bfd -Wl,-T,ape.lds -include cosmopolitan.h crt.o ape.o cosmopolitan.a
crt.o:; wget --compression=gzip https://justine.lol/cosmopolitan/crt.o
ape.o:; wget --compression=gzip https://justine.lol/cosmopolitan/ape.o
ape.lds:; wget --compression=gzip https://justine.lol/cosmopolitan/ape.lds
cosmopolitan.h:; wget --compression=gzip https://justine.lol/cosmopolitan/cosmopolitan.h
cosmopolitan.a:; wget --compression=gzip https://justine.lol/cosmopolitan/cosmopolitan.a

################################################################################
# make sure it compiles in weird environments

check:
	gcc -c -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	g++ -xc++ -c -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	gcc -pedantic -std=c99 -c -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	clang -c -Wall -Wextra -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	clang++ -xc++ -c -Wall -Wextra -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
	clang -pedantic -std=c99 -c -Wall -Wextra -O3 -o /tmp/bestline.o bestline.c
