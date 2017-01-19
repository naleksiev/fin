SANITIZER =
ifdef ubsan
	SANITIZER = -fsanitize=undefined
	NO_RTTI   =
endif
ifdef asan
	SANITIZER = -fsanitize=address
endif
ifdef tsan
	SANITIZER = -fsanitize=thread
endif
ifdef msan
	SANITIZER = -fsanitize=memory
endif

OPTIMIZE = -O0
ifdef release
	OPTIMIZE = -O3
endif
.build/fin.o: test/fin.c src/*.h src/*.c src/mod/*.h src/mod/*.c include/fin/fin.h
	mkdir -p .build
	$(CC) $(SANITIZER) $(OPTIMIZE) -std=c99 -I include test/fin.c src/*.c src/mod/*.c -o .build/fin.o -lm -Wno-typedef-redefinition

all: .build/fin.o

clean:
	rm -Rf .build

run: .build/fin.o
	@if .build/fin.o ; then echo "PASSED"; else echo "FAILED"; exit 1; fi;

