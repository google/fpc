fpc: fpc.c
	$(CC) -Wall -g fpc.c -lm -o fpc

.PHONY: clean
clean:
	rm -f fpc
