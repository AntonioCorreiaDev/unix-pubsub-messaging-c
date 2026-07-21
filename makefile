all: broker feed

broker: manager.c util.h
	gcc -o manager manager.c

feed: feed.c util.h
	gcc -o feed feed.c

clean:
	rm -f manager feed pipe f_*

