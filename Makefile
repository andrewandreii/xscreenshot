exec = a.out
sources = $(wildcard *.c)
objects = $(sources:.c=.o)
libs = -lX11 -lpng -lswscale -lavutil -lavcodec -lavformat
flags = -g -Wall ${libs}

$(exec): $(objects)
	gcc $(objects) $(flags) -o $(exec)

%.o: %.c %.h
	gcc -c $(flags) $< -o $@

clean:
	rm -f *.out *.o *.png

lint:
	clang-tidy *.c *.h
