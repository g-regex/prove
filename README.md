# [prove]

[prove] is a proof verification system using bracketed expressions.

This system is of experimental nature and is not being developed any further. The insights gained during the work on this project lead to a first [Python implementation](https://www.zurab.online/2022/02/lesson-1-python-based-introduction-to.html) of the SOFiA proof assistant and to a later [Haskell prototype](https://github.com/g-regex/sofia_haskell) thereof.

## Installation

In the Linux terminal, follow the following steps:

- Clone the repository:

```shell
git clone git@github.com:g-regex/prove.git
```
-  Change directory:

```shell
cd prove
```
- Compile with debug functionality (recommended):
```shell
make debug
```
- To compile without debug functionality type:
```shell
make
```

The path to the \[prove\] binary is now `./bin/proveparser`

## Usage

For information on the usage of \[prove\] refer to the [HELP.md](https://github.com/g-regex/prove/blob/main/HELP.md)

## Documentation

Refer to the [doc.pdf](https://github.com/g-regex/prove/blob/main/doc/doc.pdf) in the doc/ folder.

## License
[prove] is distributed under the [GPL 3.0 license](https://github.com/g-regex/prove/blob/master/LICENSE.md).
