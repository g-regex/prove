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

tikz: tikz.c tikz.h | $(BINDIR)
	$(COMPILE) -o $(BINDIR)/$@ $^

$(BINDIR):
	mkdir $(BINDIR)

.PHONY: all clean check checknd checkcmplt pdf runchecks safecheck debug docgen doc

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

debug: DFLAGS+=-DDPARSER -DDTIKZ -DDVERIFY -DDGRAPH
debug: proveparser

check: CHECKARGS=--dparser --dtikz --dfinish
check: debug runchecks
checkcmplt: CHECKARGS=--dparser --dtikz --dcomplete --dfinish
checkcmplt: debug runchecks
checknd: all runchecks

doc: cleanbin cleantex debug tikz docgen

pdf: cleanbin cleantex safecheck pdflatex

.ONESHELL:
safecheck:
	@-${MAKE} check
docgen:
	for T in `ls doc/examples/*.prove`
	do
		$(BINDIR)/proveparser $$T --dtikz --dfinish
		$(BINDIR)/tikz
		pdflatex -output-directory doc/tikz debug/$$(basename $$T .prove).tex
		pdflatex -output-directory doc/tikz debug/legend.tex
		cd doc
		biber -E utf8 doc
		pdflatex --shell-escape doc.tex
	done
runchecks:
	@rm -rf testcases/out
	mkdir -p testcases/out
	S=0
	printf "\nValidating testcases:\n"
	for T in `ls testcases/valid/*.prove |  sort -V`
	do
		$(BINDIR)/proveparser $$T $(CHECKARGS) 2> testcases/out/$$(basename $$T).err > testcases/out/$$(basename $$T).out
		if (test $$? -eq 1)
		then
			printf "%-50s[\033[0;31m failure \033[0;0m]\n" $$T
			printf ">>> [VALID] $$(basename $$T):\n" >> testcases/out/report_failure.txt
			printf " >> original file:\n" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_failure.txt
			printf "\n" >> testcases/out/report_failure.txt
			printf " >> [prove] output:\n" >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_failure.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).err | sed 's/^/    /g' >> testcases/out/report_failure.txt
			printf "\n" >> testcases/out/report_failure.txt
			S=1
		else
			printf "%-50s[\033[0;32m success \033[0;0m]\n" $$T
			printf ">>> [VALID] $$(basename $$T):\n" >> testcases/out/report_success.txt
			printf " >> original file:\n" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' $$T | sed 's/^/    /g' >> testcases/out/report_success.txt
			printf "\n" >> testcases/out/report_success.txt
			printf " >> [prove] output:\n" >> testcases/out/report_success.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).out | sed 's/^/    /g' >> testcases/out/report_success.txt
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).err | sed 's/^/    /g' >> testcases/out/report_success.txt
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
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).err | sed 's/^/    /g' >> testcases/out/report_failure.txt
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
			sed 's/\\/\\/g' testcases/out/$$(basename $$T).err | sed 's/^/    /g' >> testcases/out/report_success.txt
			printf "\n" >> testcases/out/report_success.txt
		fi
	done
	if (test $$S -eq 1)
	then
		echo "-------- UNSUCCESSFULLY VALIDATED TESTCASES --------" >> testcases/out/report.txt
		cat testcases/out/report_failure.txt >> testcases/out/report.txt 2> /dev/null
		printf "\n" >> testcases/out/report.txt
	fi
	echo "--------- SUCCESSFULLY VALIDATED TESTCASES ---------" >> testcases/out/report.txt
	cat testcases/out/report_success.txt >> testcases/out/report.txt 2> /dev/null
	exit $$S

pdflatex:
	@printf "\nGenerating PDFs:\n"
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
