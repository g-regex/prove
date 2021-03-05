**Usage:** `$PATH_TO_PROVE_BINARY --help | [--dall | --dparser | --dverify | --dtikz | --dcomplete] <filename>`


**GENERAL options:**


`--help`	display this message


**DEBUGGING options:**


`--noveri`  	do not perform any verification (useful, when creating TIKZ graphs)

`--dparser`  	activate debugging output for parser

`--dverify`  	activate debugging output for verification (implies `--dparser`)

`--dpath`  	print list of successfully verified nodes, which were discarded (implies `--dverify`)

`--dfail`  	print list of failed verifications (implies `--dverify`)

`--dtikz`    	generate TikZ graph representation in ./debug/

`--dcomplete`	do not break verification loop after first success

`--dfinish`  	finish execution, even if verification fails

`--dall`     	activate all debugging options

