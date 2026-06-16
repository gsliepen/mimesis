关于Mimesis
-------------
Mimesis 是一个C++ 11编写的邮件消息解析和创建库.
支持C++11 ~ 17标准。
使用它只需要编译lib文件，或者把src下的源码文件放到你的项目中，然后所有的.cpp文件参与编译即可。
原作者： https://github.com/gsliepen/mimesis
我的修改：
- 支持Windows编码，因为Windows上的iconv库不是很好编译
- 修正了部分可能导致异常的错误
- 通过宏定义支持wstring，因为Windows上最好用utf16le编码。
- 添加了解析邮件中的附件的代码示例(load-parse-test.cpp)
## 编译
    mkdir build
    cd build
    cmake ..




About Mimesis
-------------

Mimesis is a C++ library for RFC2822 message parsing and creation. It was born
out of a frustration with existing email libraries for C++. The goals of
Mimesis are:

- Equally good at parsing and building RFC2822 messages.
- Easy API without unnecessary abstraction layers.
- Make good use of C++11 features.

In particular, applications that parse/build emails want to treat them just
like a user would treat emails in a mail user agent (MUA). Users don't see
emails as a tree of MIME structures; instead they see them as a collection of
headers, bodies and attachments. Users also don't see details such as MIME
types and content encodings, MUAs handle those details automatically.

Building and installing
-----------------------

Mimesis uses the Meson build system. Ensure you have this installed. Use your
operating system's package manager if available, for example on Debian and its
derivatives, use `sudo apt install meson`. If that is not possible, download
and install Meson from https://mesonbuild.com/.

To build Mimesis, run these commands in the root directory of the source code:

    meson build
    ninja -C build

To run the test suite, run:

    ninja -C build test

To install the binaries on your system, run:

    ninja -C build install

The latter command might ask for a root password if it detects the installation
directory is not writable by the current user.

By default, a debug version will be built, and the resulting libraries and
header files will be install inside `/usr/local/`. If you want to create a
release build and install it in `/usr/`, use the following commands:

    meson --buildtype=release --prefix=/usr build-release
    ninja -C build-release install

Distribution packagers that want to override all compiler flags should use
`--buildtype=plain`. The `DESTDIR` environment variable is supported by Meson.
