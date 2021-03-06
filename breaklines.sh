#!/bin/bash
awk '
{ 
	gsub(/\t/, " ")
	s = split($0, chars, "\033")
	if (s > 1 && length(chars[1]) != 0 && !(chars[1] ~ /^[ ]/)) {
		printf("\033")
	}
	printf("%s", chars[1])
	split(chars[1], m, "m")	
	l = length(chars[1]) - length(m[1]) - 1
	for (i=2; i <= s; i++) {
		split(chars[i], m, "m")	
		l += length(chars[i]) - length(m[1]) - 1
		if (l > 80) {
				printf("\\color{black}\n\\ensuremath{\\color{cornflowerblue}\\hookrightarrow\\space}")
			l = length(chars[i]) - length(m[1]) + 2
		}
		sub(/\{/, "\\textbf\{\\\{", chars[i])
		sub(/\}/, "\\\}\}", chars[i])
		printf("\033%s", chars[i])
	}
	printf("\n")
}'
