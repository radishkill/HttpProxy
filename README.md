# HttpProxy
HttpProxy is my graduation project of undergraduate.
It is a efficient HTTP&SSL proxy, is developed by C++ programming language and Asio network library.

# 目录结构
./doc 毕设设计答辩相关材料。
./src 程序源代码
./HttpProxy.tar.gz 程序可执行程序压缩包
./proxy.cfg 程序基本配置文件

# 编译环境
系统使用Linux
编译器为GCC (版本需大于7.1.0)
编译工具CMake
系统需安装有Boost基本库

# 编译方法
进入程序根目录
mkdir build
cd build
cmake ../src

# 运行环境及运行方法
可执行程序需要在Linux系统控制台运行
运行时可执行程序同级目录需要保护proxy.cfg基本配置文件，且内容正确。

