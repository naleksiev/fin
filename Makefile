.build/fin.o: test/fin.c src/*.h src/*.c
	mkdir -p .build
	$(CC) -Os test/fin.c src/*.c -o .build/fin.o

all: .build/fin.o

clean:
	rm -Rf .build

run: .build/fin.o
	@if .build/fin.o ; then echo "PASSED"; else echo "FAILED"; exit 1; fi;

