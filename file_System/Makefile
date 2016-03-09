CC=gcc
FLAGS=-O0 -w

all:
	$(CC) -o measureSeqtime measureSeqtime.c $(FLAGS)
	$(CC) -o measureRantime measureRantime.c $(FLAGS)
	$(CC) -o measurePGtime measurePGtime.c $(FLAGS)
	$(CC) -o measureContentiontime measureContentiontime.c $(FLAGS)

clean:
	rm *csv
