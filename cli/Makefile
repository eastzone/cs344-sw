# Builds the standalone CLI server

all: test

include ../Makefile.common
APP = cli
CC = gcc
CFLAGS = -g -Wall $(ARCH) -I ../lwtcp -D_STANDALONE_CLI_

LEX = flex
YACC = bison
COMP_LIBS = -lm -lpthread

PARSER_MAGIC_OBJS = y.tab.o lex.yy.o
PARSER_SRCS = cli.c cli_help.c cli_main.c helper.c socket_helper.c
PARSER_OBJS = $(patsubst %.c, %.o, $(PARSER_SRCS))
PARSER_ALL_OBJS = $(PARSER_MAGIC_OBJS) $(PARSER_OBJS)

LEX_FLAGS_LONG = --debug --case-insensitive
LEX_FLAGS = -d -i

.yy.o : $*.yy.c
	$(CC) $(CFLAGS) -c -o $@ $*.c

lex.yy.c : cli_scanner.lex cli_parser.y y.tab.h cli_parser.h cli_scanner.h
	$(LEX) $(LEX_FLAGS) cli_scanner.lex

y.tab.o : y.tab.c
	$(CC) $(CFLAGS) -c -o y.tab.o y.tab.c

y.tab.h y.tab.c : cli_parser.y cli_parser.h cli_scanner.h
	$(YACC) -d --verbose --debug --yacc cli_parser.y

socket_helper_to_stderr.o : socket_helper.c socket_helper.h
	$(CC) $(CFLAGS) -D_PRINT_TO_STDERR_NOT_SOCKET_ -c -o $@ socket_helper.c

test : clean qtest

qtest : socket_helper_to_stderr.o
	make MORE_FLAGS=-D_STANDALONE_CLI_ -C ../ test_cli.exe

CLI_SRCS = $(PARSER_SRCS)
CLI_OBJS = $(patsubst %.c, %.o, $(CLI_SRCS)) $(PARSER_MAGIC_OBJS)
CLI_DEPS = $(patsubst %.c, .%.d, $(CLI_SRCS))

cli: $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $(APP) $(CLI_OBJS) $(LIBS) $(USER_LIBS)

$(CLI_DEPS): .%.d : %.c
	$(CC) -MM $(CFLAGS) $< > $@

include $(CLI_DEPS)

clean:
	rm -f *.o $(APP) *.yy.c *.tab.* y.output *.exe

clean-deps:
	rm -f .*.d
