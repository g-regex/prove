#ifdef DEBUG
#ifdef MD
#define MDC "`"
#define MDN "\n"
#define MDS "**"
#else
#define MDC ""
#define MDN ""
#define MDS ""
#endif
#define USAGE \
	MDS "Usage: " MDS MDC "%s --help | "\
	"[--dall | --dparser | --dverify | --dtikz | --dcomplete] "\
	"<filename>" MDC "\n" MDN

#define HELP \
	"\n" MDS "GENERAL options:" MDS "\n\n" MDN\
	MDC "--help" MDC "\tdisplay this message\n" MDN\
	"\n" MDS "DEBUGGING options:" MDS "\n\n" MDN\
	MDC "--dparser" MDC "  \tactivate debugging output for parser\n" MDN\
	MDC "--dgraph" MDC "   \tactivate debugging output for graph creation "\
	 			"(implies " MDC "--dparser" MDC ")\n" MDN\
	MDC "--dverify" MDC "  \tactivate debugging output for verification "\
	 			"(implies " MDC "--dparser" MDC ")\n" MDN\
	MDC "--dtikz" MDC "    \tgenerate TikZ graph representation in ./debug/\n" MDN\
	MDC "--dcomplete" MDC "\tdo not break verification loop after first success\n" MDN\
	MDC "--dfinish" MDC "  \tfinish execution, even if verification fails\n" MDN\
	MDC "--dall" MDC "     \tactivate all debugging options\n" MDN

#else

#define USAGE \
	"Usage: %s --help | <filename>\n"

#define HELP \
	"\nGENERAL options:\n\n"\
	"--help\tdisplay this message\n"\
	"\nDEBUGGING options:\n\n"\
	"--dcomplete\tdo not break verification loop after first success\n"\
	"--dfinish  \tfinish execution, even if verification fails\n"\
	"\nFor more debugging options, compile with full debugging support.\n"

#endif

#define NOSUPPORT \
	fprintf(stderr, "Executable has been compiled without this feature.");\
	exit(EXIT_FAILURE);
