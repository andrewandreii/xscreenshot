exec = a.out
sources = $(wildcard *.c)
objects = $(sources:.c=.o)
flags = -g -Wall -lX11 -lpng

$(exec): $(objects)
	gcc $(objects) $(flags) -o $(exec)

%.o: %.c %.h
	gcc -c $(flags) $< -o $@

clean:
	rm -f *.out *.o *.png

lint:
	clang-tidy *.c *.h
