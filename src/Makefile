DEBUG    = -ggdb
OPTIMISE = -O0
WARNINGS = -Wall -Wextra -Wno-variadic-macros -Wno-overlength-strings -pedantic
CC       = gcc
RM       = rm -f
COMPILE  = $(CC) $(CFLAGS) $(DFLAGS)
EXES     = proveparser pscanner

BINDIR   = ../bin
LOCALBIN = ~/.local/bin

proveparser: proveparser.c pscanner.o token.o | $(BINDIR)
	$(COMPILE) -o $(BINDIR)/$@ $^
pscanner.o: pscanner.c pscanner.h
	$(COMPILE) -c $<
token.o: token.c token.h
	$(COMPILE) -c $<

$(BINDIR):
	mkdir $(BINDIR)

.PHONY: all clean

all: proveparser

clean:
	$(RM) $(foreach EXEFILE, $(EXES), $(BINDIR)/$(EXEFILE))
	$(RM) *.o
	$(RM) -rf $(BINDIR)/*.dSYM
