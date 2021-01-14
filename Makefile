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

clean:
	$(RM) $(foreach EXEFILE, $(EXES), $(BINDIR)/$(EXEFILE))
	$(RM) *.o
	$(RM) -rf $(BINDIR)/*.dSYM
	$(RM) -rf testcases/out
	$(RM) -rf debug

debug: DFLAGS+=-DDPARSER -DDTIKZ -DDVERIFY
debug: proveparser

check: debug runchecks
checknd: all runchecks
pdf: check pdflatex

.ONESHELL:
runchecks:
	mkdir -p testcases/out
	S=0
	for T in `ls testcases/valid/*.prove |  sort -V`
	do
		echo -ne "$$T: \t"
		$(BINDIR)/proveparser $$T -dall 2> testcases/out/$$(basename $$T).err > testcases/out/$$(basename $$T).out
		if (test $$? -eq 1)
		then
			rm debug/$$(basename $$T .prove).tex &> /dev/null
			echo -e "\t[\033[0;31m failure \033[0;0m]"
			echo ">>> $$(basename $$T):" >> testcases/out/report_failure.txt
			echo " >> original file:" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_failure.txt
			echo "" >> testcases/out/report_failure.txt
			echo " >> [prove] output:" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_failure.txt
			echo "    "`cat testcases/out/$$(basename $$T).err` >> testcases/out/report_failure.txt
			echo >> testcases/out/report_failure.txt
			S=1
		else
			echo -e "\t[\033[0;32m success \033[0;0m]"
			rm debug/$$(basename $$T .prove).log &> /dev/null
			rm debug/$$(basename $$T .prove).aux &> /dev/null
			echo ">>> $$(basename $$T):" >> testcases/out/report_success.txt
			echo " >> original file:" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_success.txt
			echo "" >> testcases/out/report_success.txt
			echo " >> [prove] output:" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_success.txt
			echo "    "`cat testcases/out/$$(basename $$T).err` >> testcases/out/report_success.txt
			echo >> testcases/out/report_success.txt
		fi
	done
	for T in `ls testcases/invalid/*.prove |  sort -V`
	do
		echo -ne "$$T: \t"
		$(BINDIR)/proveparser $$T -dall 2> testcases/out/$$(basename $$T).err > testcases/out/$$(basename $$T).out
		if (test $$? -eq 0)
		then
			rm debug/$$(basename $$T .prove).tex &> /dev/null
			echo -e "\t[\033[0;31m failure \033[0;0m]"
			echo ">>> $$(basename $$T):" >> testcases/out/report_failure.txt
			echo " >> original file:" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_failure.txt
			echo "" >> testcases/out/report_failure.txt
			echo " >> [prove] output:" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_failure.txt
			echo "    "`cat testcases/out/$$(basename $$T).err` >> testcases/out/report_failure.txt
			echo >> testcases/out/report_failure.txt
			S=1
		else
			rm debug/$$(basename $$T .prove).tex &> /dev/null
			echo -e "\t[\033[0;32m success \033[0;0m]"
			echo ">>> $$(basename $$T):" >> testcases/out/report_success.txt
			echo " >> original file:" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_success.txt
			echo "" >> testcases/out/report_success.txt
			echo " >> [prove] output:" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_success.txt
			echo "    "`cat testcases/out/$$(basename $$T).err` >> testcases/out/report_success.txt
			echo >> testcases/out/report_success.txt
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
	for T in `ls debug/*.tex |  sort -V`
	do
		pdflatex -output-directory debug $$T
	done
