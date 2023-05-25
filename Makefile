CC := /usr/bin/gcc

CFLAGS := -O3 -fanalyzer -g
LIBS := -lSDL2

bin/libvega.so: bin/vegashader.o bin/vegaio.o bin/vegavideo.o
	$(CC) -shared -fPIC -o $@ $^ $(LIBS)

bin/%.o: src/%.c
	$(CC) -c -fPIC -o $@ $^ $(CFLAGS)

bin/%.o: src/%.S
	$(CC) -c -fPIC -o $@ $^ $(CFLAGS) -Isrc

install:
	cp bin/libvega.so /usr/local/lib

clean:
	rm -f $(wildcard bin/*.o) bin/libvega.so