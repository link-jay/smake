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
[#?flag]
target:[dependence[:message]]
	[commands]
```
You can use `#?flag` at the top of file to change the behavior of smake.  
`#?/#?OUTPUT`can close the outputs of commands.  
`#?ALL` only keeps the [INFO] and [ERROR].  

## Process
![process.svg](process.svg)
