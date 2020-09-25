# [prove]:

[prove] is a proof verification system using bracketed expressions. It is currently work in progress.

## syntax:
A convenient way of recursively describing CFGs ([context-free grammars](https://en.wikipedia.org/wiki/Context-free_grammar)) is given by the concept of the EBNF ([extended Backus-Naur form](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_Form)).
The production rules given in the following table can be considered as a draft for formalising the syntactic properties of the [prove] language:
![A draft of the EBNF](https://github.com/g-regex/prove/blob/master/doc/EBNF_pic.jpg?raw=true)

A [prove]-source code file is process in a linear manner by the scanner (located in pscanner.c). During this process illegal characters are detected and group of legal character are grouped to tokens. Not every <`production rule`> corresponds to a token. Rather tokens can be thought of being the build blocks of the production rules. In the case of [prove] we have 8 different kinds of tokens:

| token        | description                                           |
| ------------ | ----------------------------------------------------- |
| `TOK_EOF`    | end of file token                                     |
| `TOK_LBRACK` | left square bracket                                   |
| `TOK_RBRACK` | right square bracket                                  |
| `TOK_VAR`    | variable identifier                                   |
| `TOK_NUM`    | non-negative integer number                           |
| `TOK_IMPLY`  | colon (indication implication)                        |
| `TOK_SYM`    | identifier of an operator symbol (different to colon) |
| `TOK_FALSE`  | reserved keyword for "false"                          |

The actual syntactic analysis is done by the [parser](https://en.wikipedia.org/wiki/Parsing) (located in proveparser.c). The parser recursively processes the source code token by token following the rules formulated in the EBNF.

This is an attempt to visualise the process of parsing a [prove] source file in a sequence diagram:

![sequence diagramm of the parsing process](https://github.com/g-regex/prove/blob/master/doc/sequence.jpg?raw=true)