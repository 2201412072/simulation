# simulation
西交体系结构的模拟五段流水线程序


simulation.cpp是主要的模拟程序，它模拟了shell界面进行操作。
当运行它时，可以输入help指令，以了解它能够支持的所有操作。

同时提交的rubbish.txt用于存放指令的二进制表示，该指令是我自己设计的，只支持7条指令。
code.txt是为了方便观察，写的汇编代码。
指令生成.cpp用于生成指令，可以写字符串，然后由它跑出来二进制代码。
它的格式如下：
load a b num
store a b num
addi a b num
add a b c
sub a b c
beqz a addr
trap

