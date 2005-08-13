all: libcowdancer.so

libcowdancer.so: cowdancer.lo
	gcc -shared -o $@ $<

%.lo: %.c
	gcc -fPIC $< -o $@ -c -Wall -O2 -g
