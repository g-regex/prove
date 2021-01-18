/* [prove]: A proof verification system using bracketed expressions.
 * Copyright (C) 2021  Gregor Feierabend
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef DTIKZ
#ifndef TIKZ_H
#define TIKZ_H

#include <stdio.h>

#define TIKZ_DIST 40
#define TIKZ_HSPACE 25
#define TIKZ_FLAG_SIZE 7
#define TIKZ_MINWIDTH 25
#define TIKZ_MINHEIGHT TIKZ_FLAG_SIZE * 9

#define TIKZ_COLOR1 "darkmagenta"
#define TIKZ_COLOR1_RGB "0.55, 0.0, 0.55"
#define TIKZ_COLOR2 "darkorange"
#define TIKZ_COLOR2_RGB "1.0, 0.55, 0.0"
#define TIKZ_COLOR3 "darkpastelgreen"
#define TIKZ_COLOR3_RGB "0.01, 0.75, 0.24"
#define TIKZ_COLOR4 "oucrimsonred"
#define TIKZ_COLOR4_RGB "0.6, 0.0, 0.0"
#define TIKZ_COLOR5 "darkpowderblue"
#define TIKZ_COLOR5_RGB "0.0, 0.2, 0.6"
#define TIKZ_COLOR6 "mediumspringgreen"
#define TIKZ_COLOR6_RGB "0.0, 0.98, 0.6"
#define TIKZ_COLOR7 "deepskyblue"
#define TIKZ_COLOR7_RGB "0.0, 0.75, 1.0"
#define TIKZ_COLOR8 "black"
#define TIKZ_COLOR8_RGB "0.0, 0.0, 0.0"

#define TIKZ_LGND_SPACING "30"
#define TIKZ_LGND_ASIZE "10"

#define TIKZ_LGND_IMPL "\\texttt{IMPL}: node is part of an implication"
#define TIKZ_LGND_EQTY "\\texttt{EQTY}: node is part of an equality"
#define TIKZ_LGND_FMLA "\\texttt{FMLA}: node is part of an ordinary formula"
#define TIKZ_LGND_ASMP "\\texttt{ASMP}: node is an assumption"
#define TIKZ_LGND_NEWC \
	"\\texttt{NEWC}: node contains a newly introduced constant"
#define TIKZ_LGND_LOCK \
	"\\texttt{LOCK}: \\texttt{ASMP} flag is locked for the entire subtree"
#define TIKZ_LGND_FRST \
	"\\texttt{FRST}: node is positioned before first "\
	"occurrence of an implication formulator in that formula"

#define TIKZ_HEADER \
	"%%This file was automatically generated by [prove].\n\n"\
	"\\title{Graph}\n"\
	"\\documentclass[landscape, 11pt]{article}\n"\
	"\n\\usepackage{tikz}\n"\
	"\\usetikzlibrary{calc}\n"\
	"\\usetikzlibrary{positioning}\n"\
	"\\usetikzlibrary{arrows.meta}\n"\
	"\n\\usepackage[margin=0pt, hoffset=0pt, voffset=0pt, top=20pt,"\
	"bottom=20pt]{geometry}\n"\
	"\n\\usepackage{color}\n"\
	"\\definecolor{" TIKZ_COLOR1 "}{rgb}{" TIKZ_COLOR1_RGB "}\n"\
	"\\definecolor{" TIKZ_COLOR2 "}{rgb}{" TIKZ_COLOR2_RGB "}\n"\
	"\\definecolor{" TIKZ_COLOR3 "}{rgb}{" TIKZ_COLOR3_RGB "}\n"\
	"\\definecolor{" TIKZ_COLOR4 "}{rgb}{" TIKZ_COLOR4_RGB "}\n"\
	"\\definecolor{" TIKZ_COLOR5 "}{rgb}{" TIKZ_COLOR5_RGB "}\n"\
	"\\definecolor{" TIKZ_COLOR6 "}{rgb}{" TIKZ_COLOR6_RGB "}\n"\
	"\\definecolor{" TIKZ_COLOR7 "}{rgb}{" TIKZ_COLOR7_RGB "}\n"\
	"\n\\usepackage{calc}\n"\
	"\\usepackage{subcaption}\n"\
	"\\newsavebox\\tlegend\n"\
	"\\newlength\\tlegendheight\n"\
	"\\newsavebox\\tgraph\n"\
	"\n\\usepackage{listings}\n"\
	"\\newlength\\tgraphheight\n"\
	"\n\\pagenumbering{gobble}\n"\
	"\n\n\\begin{document}\n"\
	"\n%%Legend\n"\
	"\\sbox{\\tlegend}{\n"\
	"\\resizebox{0.9\\hsize}{!}{"\
	"\\begin{tikzpicture}[node distance = 1pt, auto]\n"\
	"\\node (IMPL) at (0pt,0pt) {" TIKZ_LGND_IMPL "};\n"\
	"\\node[right = " TIKZ_LGND_SPACING "pt of IMPL] (EQTY)"\
	"{" TIKZ_LGND_EQTY "};\n"\
	"\\node[right = " TIKZ_LGND_SPACING "pt of EQTY] (FMLA)"\
	"{" TIKZ_LGND_FMLA "};\n"\
	"\\node[right = " TIKZ_LGND_SPACING "pt of FMLA] (ASMP)"\
	"{" TIKZ_LGND_ASMP "};\n"\
	"\\node[right = " TIKZ_LGND_SPACING "pt of ASMP] (NEWC)"\
	"{" TIKZ_LGND_NEWC "};\n"\
	"\\node[right = " TIKZ_LGND_SPACING "pt of NEWC] (LOCK)"\
	"{" TIKZ_LGND_LOCK "};\n"\
	"\\node[right = " TIKZ_LGND_SPACING "pt of LOCK] (FRST)"\
	"{" TIKZ_LGND_FRST "};\n"\
	"\\draw[-{Triangle[length=" TIKZ_LGND_ASIZE "pt,width=" TIKZ_LGND_ASIZE \
	"pt]}, color=" TIKZ_COLOR1 "] ([xshift=-" TIKZ_LGND_ASIZE "pt] IMPL.west)"\
	"to (IMPL.west);\n"\
	"\\draw[-{Triangle[length=" TIKZ_LGND_ASIZE "pt,width=" TIKZ_LGND_ASIZE \
	"pt]}, color=" TIKZ_COLOR2 "] ([xshift=-" TIKZ_LGND_ASIZE "pt] EQTY.west)"\
	"to (EQTY.west);\n"\
	"\\draw[-{Triangle[length=" TIKZ_LGND_ASIZE "pt,width=" TIKZ_LGND_ASIZE \
	"pt]}, color=" TIKZ_COLOR3 "] ([xshift=-" TIKZ_LGND_ASIZE "pt] FMLA.west)"\
	"to (FMLA.west);\n"\
	"\\draw[-{Triangle[length=" TIKZ_LGND_ASIZE "pt,width=" TIKZ_LGND_ASIZE \
	"pt]}, color=" TIKZ_COLOR4 "] ([xshift=-" TIKZ_LGND_ASIZE "pt] ASMP.west)"\
	"to (ASMP.west);\n"\
	"\\draw[-{Triangle[length=" TIKZ_LGND_ASIZE "pt,width=" TIKZ_LGND_ASIZE \
	"pt]}, color=" TIKZ_COLOR5 "] ([xshift=-" TIKZ_LGND_ASIZE "pt] NEWC.west)"\
	"to (NEWC.west);\n"\
	"\\draw[-{Triangle[length=" TIKZ_LGND_ASIZE "pt,width=" TIKZ_LGND_ASIZE \
	"pt]}, color=" TIKZ_COLOR6 "] ([xshift=-" TIKZ_LGND_ASIZE "pt] LOCK.west)"\
	"to (LOCK.west);\n"\
	"\\draw[-{Triangle[length=" TIKZ_LGND_ASIZE "pt,width=" TIKZ_LGND_ASIZE \
	"pt]}, color=" TIKZ_COLOR7 "] ([xshift=-" TIKZ_LGND_ASIZE "pt] FRST.west)"\
	"to (FRST.west);\n"\
	"\\end{tikzpicture} } }\n"\
	"\\sbox{\\tgraph}{\n"\
	"\\resizebox{0.9\\hsize}{!}{"\
	"\\begin{tikzpicture}[node distance = 1pt, auto]\n"

#define TIKZ_GRAPHSCOPE \
	"\n%%nodes and arrows numbered in pre-order traversal\n"\
	"\\begin{scope}["\
	"every node/.style={rectangle,inner sep=3pt,minimum width=%dpt, minimum height=%dpt, text height=5pt,yshift=0pt}, "\
	"-{Latex[length=5pt,width=5pt]}]\n\n", TIKZ_MINWIDTH, TIKZ_MINHEIGHT
#define TIKZ_STARTNODE \
	"\\node[draw] (0) at (0pt,0pt) {$0$};\n"
#define TIKZ_CHILDNODE(pnum, n) \
	"\\node[draw, below = %dpt of %d] "\
	"(%d) {$%d$};\n", TIKZ_DIST, pnum, n, n
#define TIKZ_CHILDARROW(pnum, n) \
	"\\draw (%d.south) -- (%d.north);\n", pnum, n
#define TIKZ_RIGHTTOPNODE(pnum, n, rmost) \
	"\\node[draw, right = %dpt] "\
	"(%d) at (%d -| %d.east) {\\textrm{%d}};\n", TIKZ_HSPACE,\
	n, pnum, rmost, n
#define TIKZ_RIGHTNODE(pnum, n) \
	"\\node[draw, right = %dpt of %d] (%d)  {$%d$};\n", TIKZ_HSPACE,\
	pnode->num, n, n
#define TIKZ_RIGHTARROW(pnum, n) \
	"\\draw (%d.east) -- (%d.west);\n", pnum, n

#define TIKZ_SYMSCOPE(maxdepth) \
	"\n\n%%symbols corresponding to nodes, "\
	"read before freeing memory of the graph\n"\
	"\\begin{scope}["\
	"every node/.style={rectangle,inner sep=3pt,minimum width=%dpt, "\
	"minimum height=%dpt, text height=5pt,yshift=0pt}, -]\n"\
	"\\node (symalign) at (0pt,-%dpt) {};\n\n", TIKZ_MINWIDTH, TIKZ_MINHEIGHT,\
	(TIKZ_MINHEIGHT + TIKZ_DIST) * maxdepth
#define TIKZ_FLAG_A \
	"\\draw[-{Triangle[length=%dpt,width=%dpt]}, color="
#define TIKZ_FLAG_B(pnum, shift) \
	"] ([yshift=%dpt] %d.east) to ([yshift=%dpt, xshift=%dpt] "\
	"%d.east);\n", TIKZ_FLAG_SIZE, TIKZ_FLAG_SIZE, shift * TIKZ_FLAG_SIZE,\
	pnum, shift * TIKZ_FLAG_SIZE, TIKZ_FLAG_SIZE, pnum
#define TIKZ_SYMNODE(pnum, psym) \
	"\\node[draw] (s%d) at (symalign -| %d) {\\lstinline| %s |};\n",\
	pnum, pnum, psym
#define TIKZ_SYMARROW(pnum) \
	"\\draw[thin, dash dot, color=gray] (%d.south) -- (s%d.north);\n",\
	pnum, pnum

#define TIKZ_ENDSCOPE \
	"\n\\end{scope}\n"	
#define TIKZ_FOOTER \
	"\n\\end{tikzpicture} } }\n"\
	"\n\\begin{figure}[h!]\\centering\n"\
	"\\begin{subfigure}{\\textwidth}\\centering\\usebox{\\tlegend}\\end{subfigure}"\
	"\\vspace{10pt}\n"\
	"\\begin{subfigure}{\\textwidth}\\centering\\usebox{\\tgraph}\\end{subfigure}\n"\
	"\\end{figure}\n\n"\
	"\\settototalheight\\tlegendheight{\\usebox{\\tlegend}}\n"\
	"\\settototalheight\\tgraphheight{\\usebox{\\tgraph}}\n"\
	"\\addtolength{\\tgraphheight}{\\tlegendheight}\n"\
	"\\addtolength{\\tgraphheight}{50pt}\n"\
	"\\pdfpageheight=\\the\\tgraphheight\n"\
	"\\end{document}"

FILE* tikz;
static unsigned short int rightmost_child;
static unsigned short int max_depth;
static unsigned short int cur_depth;
#endif

#endif
