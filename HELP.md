**Usage:** `$PATH_TO_PROVE_BINARY --help | [--dall | --dparser | --dverify | --dtikz | --dcomplete] <filename>`


**GENERAL options:**


`--help`	display this message


**DEBUGGING options:**


`--dparser`  	activate debugging output for parser

`--dgraph`   	activate debugging output for graph creation (implies `--dparser`)

`--dverify`  	activate debugging output for verification (implies `--dparser`)

`--dtikz`    	generate TikZ graph representation in ./debug/

`--dcomplete`	do not break verification loop after first success

`--dfinish`  	finish execution, even if verification fails

`--dall`     	activate all debugging options

