fpc: fpc.c
	$(CC) -Wall -g fpc.c -lm -o fpc

convert: convert.c
	$(CC) -Wall -g convert.c -lm -o convert

.PHONY: clean
clean:
	rm -f fpc
