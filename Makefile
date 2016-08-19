fpc: fpc.c
	gcc fpc.c -lm -o fpc

.PHONY: clean
clean:
	rm -f fpc
