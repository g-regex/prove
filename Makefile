DEBUG    = -ggdb
OPTIMISE = -O0
WARNINGS = -Wall -Wextra -Wno-variadic-macros -Wno-overlength-strings -pedantic
CC       = gcc
LDFLAGS  = -lgmp
CFLAGS   += -fcommon
RM       = rm -f
COMPILE  = $(CC) $(CFLAGS) $(DFLAGS) $(LDFLAGS) $(DEBUG)
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

docc: doc.c tikz.h | $(BINDIR)
	$(COMPILE) -o $(BINDIR)/$@ $^

$(BINDIR):
	mkdir $(BINDIR)

.PHONY: all clean check checknd checkcmplt pdf runchecks safecheck debug docgen doc types

all: proveparser

cleanbin:
	$(RM) $(foreach EXEFILE, $(EXES), $(BINDIR)/$(EXEFILE))
	$(RM) *.o
	$(RM) -rf $(BINDIR)

cleandbg:
	$(RM) -rf testcases/out
	$(RM) -rf debug
	
cleantex:
	$(RM) -rf debug/*.tex
	$(RM) -rf debug/*.aux
	$(RM) -rf debug/*.log

cleandoc:
	$(RM) -rf doc/_minted-doc
	$(RM) -rf doc/examples/out
	$(RM) -rf _minted-doc
	$(RM) -rf doc/*.aux
	$(RM) -rf doc/*.lyx~
	$(RM) -rf doc/*.bbl
	$(RM) -rf doc/*.blg
	$(RM) -rf doc/*.bcf
	$(RM) -rf doc/*.log
	$(RM) -rf doc/*.out
	$(RM) -rf doc/*.xml
	$(RM) -rf doc/tikz

clean: cleanbin cleandbg cleantex cleandoc

debug: DFLAGS+=-DDPARSER -DDTIKZ -DDVERIFY -DDGRAPH -DDCOLOUR
debug: cleanbin proveparser

check: CHECKARGS=--dtikz --dfinish --dverify
check: debug runchecks
noveri: CHECKARGS=--dtikz --dfinish --dcomplete --noveri
noveri: debug runchecks
checkcmplt: CHECKARGS=--dparser --dtikz --dcomplete --dfinish
checkcmplt: debug runchecks
checknd: all runchecks

doc: DFLAGS+=-DDPARSER -DDTIKZ -DDVERIFY -DDGRAPH -DDCOLOUR -DDGMP
doc: cleanbin cleantex proveparser docc docgen

pdf: cleanbin cleantex safenoveri pdflatex

.ONESHELL:
safenoveri:
	@-${MAKE} noveri &> /dev/null
safecheck:
	@-${MAKE} check &> /dev/null
docgen:
	mkdir -p doc/tikz
	mkdir -p doc/examples/out
	$(BINDIR)/docc
	for T in `ls doc/examples/*.prove`
	do
		#$(BINDIR)/proveparser $$T --dtikz --dfinish --dverify --dcomplete 2>&1 | fold -w80 -s - &> doc/examples/out/$$(basename $$T .prove).out
		#$(BINDIR)/proveparser $$T --dtikz --dfinish --dverify --dcomplete &> doc/examples/out/$$(basename $$T .prove).out
		#$(BINDIR)/proveparser $$T --dtikz --dfinish --dverify --dcomplete 2>&1 | ./breaklines.sh > doc/examples/out/$$(basename $$T .prove).out
		$(BINDIR)/proveparser $$T --dtikz --dfinish --dverify --dcomplete &> doc/tmp.tex
		SUCCESS=$$?
		cat doc/tmp.tex | ./breaklines.sh > doc/examples/out/$$(basename $$T .prove).out
		if (test $$SUCCESS -eq 2)
		then
			rm -f debug/$$(basename $$T .prove).tex &> /dev/null
		else
			pdflatex -output-directory doc/tikz debug/$$(basename $$T .prove).tex
		fi
	done
	rm doc/tmp.tex
	pdflatex -output-directory doc/tikz debug/legend.tex
	cd doc
	pdflatex --shell-escape doc.tex
	biber -E utf8 doc
	pdflatex --shell-escape doc.tex
	pdflatex --shell-escape doc.tex

runchecks:
	@rm -rf testcases/out
	mkdir -p testcases/out
	S=0
	printf "\nValidating testcases:\n"
	for T in `ls testcases/valid/*.prove |  sort -V`
	do
		$(BINDIR)/proveparser $$T $(CHECKARGS) 2> testcases/out/$$(basename $$T).err > testcases/out/$$(basename $$T).out
		SUCCESS=$$?
		if (test $$SUCCESS -eq 2)
		then
			rm -f debug/$$(basename $$T .prove).tex &> /dev/null
		fi
		if (test $$SUCCESS -ne 0)
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
		SUCCESS=$$?
		if (test $$SUCCESS -eq 2)
		then
			rm -f debug/$$(basename $$T .prove).tex &> /dev/null
		fi
		if (test $$SUCCESS -eq 0)
		then
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

types: types.vim
types.vim: *.[ch]
	ctags --c-kinds=t -o- *.[ch] |\
		awk 'BEGIN{printf("syntax keyword cType\t")}\
		{printf("%s ", $$1)}\
		END{if(!NR){print "XXX_placeholder"}else{print ""}}' > $@
	ctags --c-kinds=gsu -o- *.[ch] |\
		awk 'BEGIN{printf("syntax keyword cStorageClass\t")}\
		{printf("%s ", $$1)}\
		END{if(!NR){print "XXX_placeholder"}else{print ""}}' >> $@
	ctags --c-kinds=e -o- *.[ch] |\
		awk 'BEGIN{printf("syntax keyword cConstant\t")}\
		{printf("%s ", $$1)}\
		END{if(!NR){print "XXX_placeholder"}else{print ""}}' >> $@
	ctags --c-kinds=d -o- *.[ch] |\
		awk 'BEGIN{printf("syntax keyword cDefine\t")}\
		{printf("%s ", $$1)}\
		END{if(!NR){print "XXX_placeholder"}else{print ""}}' >> $@
