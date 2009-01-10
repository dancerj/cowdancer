KLCC=klcc

all: qemuinit-static qemuinit-static-klibc

%-static: %.o ../forkexec.o
	$(CC) -static -O2 -Wall -o $@ $^

%.o: %.c
	$(CC) -c -O2 -Wall -o $@ $^

%-static-klibc: %.c ../forkexec.c 
	$(KLCC) -Wall -o $@ $^

clean:
	rm -f qemuinit-static qemuinit-static-klibc *.o *~

check-syntax:
	$(CC) -c -O2 -Wall $(CHK_SOURCES)  -o/dev/null -D LIBDIR="\"${LIBDIR}\""

.PHONY: clean