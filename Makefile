DEBUG    = -ggdb
OPTIMISE = -O0
WARNINGS = -Wall -Wextra -Wno-variadic-macros -Wno-overlength-strings -pedantic
CC       = gcc
RM       = rm -f
COMPILE  = $(CC) $(CFLAGS) $(DFLAGS) $(DEBUG)
EXES     = proveparser pscanner

BINDIR   = bin
LOCALBIN = ~/.local/bin

proveparser: proveparser.c pscanner.o pgraph.o token.o verify.o | $(BINDIR)
	$(COMPILE) -o $(BINDIR)/$@ $^
pscanner.o: pscanner.c pscanner.h
	$(COMPILE) -c $<
pgraph.o: pgraph.c pgraph.h
	$(COMPILE) -c $<
verify.o: verify.c verify.h
	$(COMPILE) -c $<
token.o: token.c token.h
	$(COMPILE) -c $<

$(BINDIR):
	mkdir $(BINDIR)

.PHONY: all clean check runchecks debug

all: proveparser

cleanbin:
	$(RM) $(foreach EXEFILE, $(EXES), $(BINDIR)/$(EXEFILE))
	$(RM) *.o
	$(RM) -rf $(BINDIR)/*.dSYM

cleandbg:
	$(RM) -rf testcases/out
	$(RM) -rf debug
	
cleantex:
	$(RM) -rf debug/*.tex
	$(RM) -rf debug/*.aux
	$(RM) -rf debug/*.log

clean: cleanbin cleandbg cleantex

debug: DFLAGS+=-DDPARSER -DDTIKZ -DDVERIFY
debug: proveparser

check: CHECKARGS=--dparser --dtikz
check: debug runchecks
checknd: all runchecks

pdf: cleanbin cleantex check pdflatex

.ONESHELL:
runchecks:
	@-mkdir -p testcases/out
	S=0
	printf "\nValidating testcases:\n"
	for T in `ls testcases/valid/*.prove |  sort -V`
	do
		$(BINDIR)/proveparser $$T $(CHECKARGS) 2> testcases/out/$$(basename $$T).err > testcases/out/$$(basename $$T).out
		if (test $$? -eq 1)
		then
			rm -f debug/$$(basename $$T .prove).tex &> /dev/null
			printf "%-50s[\033[0;31m failure \033[0;0m]\n" $$T
			printf ">>> [VALID] $$(basename $$T):\n" >> testcases/out/report_failure.txt
			printf " >> original file:\n" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_failure.txt
			printf "\n" >> testcases/out/report_failure.txt
			printf " >> [prove] output:\n" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_failure.txt
			printf "    "`cat testcases/out/$$(basename $$T).err`"\n" >> testcases/out/report_failure.txt
			printf "\n" >> testcases/out/report_failure.txt
			S=1
		else
			printf "%-50s[\033[0;32m success \033[0;0m]\n" $$T
			rm -f debug/$$(basename $$T .prove).log &> /dev/null
			rm -f debug/$$(basename $$T .prove).aux &> /dev/null
			printf ">>> [VALID] $$(basename $$T):\n" >> testcases/out/report_success.txt
			printf " >> original file:\n" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_success.txt
			printf "\n" >> testcases/out/report_success.txt
			printf " >> [prove] output:\n" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_success.txt
			printf "    "`cat testcases/out/$$(basename $$T).err`"\n" >> testcases/out/report_success.txt
			printf "\n" >> testcases/out/report_success.txt
		fi
	done
	for T in `ls testcases/invalid/*.prove |  sort -V`
	do
		$(BINDIR)/proveparser $$T $(CHECKARGS) 2> testcases/out/$$(basename $$T).err > testcases/out/$$(basename $$T).out
		if (test $$? -eq 0)
		then
			rm -f debug/$$(basename $$T .prove).tex &> /dev/null
			printf "%-50s[\033[0;31m failure \033[0;0m]\n" $$T
			printf ">>> [INVALID] $$(basename $$T):\n" >> testcases/out/report_failure.txt
			printf " >> original file:\n" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_failure.txt
			printf "\n" >> testcases/out/report_failure.txt
			printf " >> [prove] output:\n" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_failure.txt
			printf "    "`cat testcases/out/$$(basename $$T).err`"\n" >> testcases/out/report_failure.txt
			printf "\n" >> testcases/out/report_failure.txt
			S=1
		else
			rm -f debug/$$(basename $$T .prove).tex &> /dev/null
			printf "%-50s[\033[0;32m success \033[0;0m]\n" $$T
			printf ">>> [INVALID] $$(basename $$T):\n" >> testcases/out/report_success.txt
			printf " >> original file:\n" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_success.txt
			printf "\n" >> testcases/out/report_success.txt
			printf " >> [prove] output:\n" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_success.txt
			printf "    "`cat testcases/out/$$(basename $$T).err`"\n" >> testcases/out/report_success.txt
			printf "\n" >> testcases/out/report_success.txt
		fi
	done
	if (test $$S -eq 1)
	then
		echo "-------- UNSUCCESSFULLY VALIDATED TESTCASES --------" >> testcases/out/report.txt
		cat testcases/out/report_failure.txt >> testcases/out/report.txt 2> /dev/null
	fi
	echo "--------- SUCCESSFULLY VALIDATED TESTCASES ---------" >> testcases/out/report.txt
	cat testcases/out/report_success.txt >> testcases/out/report.txt 2> /dev/null
	exit $$S

pdflatex:
	@-printf "\nGenerating PDFs:\n"
	for T in `ls debug/*.tex |  sort -V`
	do
		pdflatex -halt-on-error -output-directory debug $$T > /dev/null
		if (test $$? -eq 0)
		then
			printf "%-50s[\033[0;32m success \033[0;0m]\n" $$T
		else
			printf "%-50s[\033[0;31m failure \033[0;0m]\n" $$T
		fi
	done
