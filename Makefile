CC = cc

all: myshell.o lex.yy.o
	$(CC) -o myshell myshell.o lex.yy.o -lfl

object: myshell.c lex.yy.c
	$(CC) -c myshell.c lex.yy.c

clean:
	rm -f myshell myshell.o lex.yy.o myshell.core
