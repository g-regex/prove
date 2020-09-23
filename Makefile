DEBUG    = -ggdb
OPTIMISE = -O0
WARNINGS = -Wall -Wextra -Wno-variadic-macros -Wno-overlength-strings -pedantic
CC       = gcc
RM       = rm -f
COMPILE  = $(CC) $(CFLAGS) $(DFLAGS)
EXES     = proveparser pscanner

BINDIR   = bin
LOCALBIN = ~/.local/bin

proveparser: proveparser.c pscanner.o token.o | $(BINDIR)
	$(COMPILE) -o $(BINDIR)/$@ $^
pscanner.o: pscanner.c pscanner.h
	$(COMPILE) -c $<
token.o: token.c token.h
	$(COMPILE) -c $<

$(BINDIR):
	mkdir $(BINDIR)

.PHONY: all clean check

all: proveparser

clean:
	$(RM) $(foreach EXEFILE, $(EXES), $(BINDIR)/$(EXEFILE))
	$(RM) *.o
	$(RM) -rf $(BINDIR)/*.dSYM

.ONESHELL:
check:
	for T in `ls testcases/valid/*.prove |  sort -V`
	do
		echo -ne "$$T: \t"
		$(BINDIR)/proveparser $$T 0 quiet
		if (test $$? -eq 1)
		then
			echo -e "\t[\033[0;31m failure \033[0;0m]"
		else
			echo -e "\t[\033[0;32m success \033[0;0m]"
		fi
	done
	for T in `ls testcases/invalid/*.prove |  sort -V`
	do
		echo -ne "$$T: \t"
		$(BINDIR)/proveparser $$T 0 quiet
		if (test $$? -eq 0)
		then
			echo -e "\t[\033[0;31m failure \033[0;0m]"
		else
			echo -e "\t[\033[0;32m success \033[0;0m]"
		fi
	done
