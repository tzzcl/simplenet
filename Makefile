CFLAGS = -g -pthread -std=c99 -Wall
GITFLAGS = -q  --no-verify --allow-empty
all:
	-@git add . --ignore-errors
	-@(echo "> compile" && uname -a) | git commit -F - $(GITFLAGS)
clean:
	rm -rf *.o

