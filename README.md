# cpp_coroutine

这是一个用C++实现的有栈协程。
它只是一个demo，只实现了上下文切换部分。但是非常欢迎大家改良和使用这个库，至少在它的基础上您不需要再写烦人的汇编了。

因为用了msvc风格的内联汇编。本程序只能用clang编译，编译的命令可以在 xxx_relase xxx_debug 文件夹中找到。

cy_coroutine.cpp 是您需要编译的文件，而demo.cpp演示了这个库的用法。
