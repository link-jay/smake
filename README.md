# smake
Simple make but with message.  

## Feature
  * [x] print message when rules finished.
  * [x] run specified rule.
  * [x] track modified files
  * [x] different message in different status.

## Quick Start
run command `c++ smake.cpp -o smake` and prepare a `Makefile`, then run `./smake`.

## Template
```Makefile
[#?]
target:[dependence[:message]]
	[commands]
```
You can use `#?` at the top of file to close the outputs of commands. Notice that the text after the `#?` are not readable. Maybe you can do something via it.  

## Process
![process.svg](process.svg)
