fpc: fpc.c
	$(CC) -Wall -g fpc.c -lm -o fpc

convert: convert.c
	$(CC) -Wall -g convert.c -lm -o convert

.PHONY: test_output
test_output:
	./tests.sh &> test_output.txt

.PHONY: test
test:
	./tests.sh 2>&1 | diff -U 3 test_output.txt -

.PHONY: clean
clean:
	rm -f fpc
