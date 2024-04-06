default: all

all: clean build
clean:
	rm -rf *.o aesdsocket
build:
	${CC} -o aesdsocket aesdsocket.c -Wall -Werror
run:
	./aesdsocket