fpc: fpc.c
	gcc -g fpc.c -lm -o fpc

.PHONY: clean
clean:
	rm -f fpc
