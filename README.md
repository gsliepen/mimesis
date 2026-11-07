About Mimesis
-------------

Mimesis is a C++ library for RFC2822 message parsing and creation. It was born
out of a frustration with existing email libraries for C++. The goals of
Mimesis are:

- Equally good at parsing and building RFC2822 messages.
- Easy API without unnecessary abstraction layers.
- Make good use of modern C++ (C++20) features.

In particular, applications that parse/build emails want to treat them just
like a user would treat emails in a mail user agent (MUA). Users don't see
emails as a tree of MIME structures; instead they see them as a collection of
headers, bodies and attachments. Users also don't see details such as MIME
types and content encodings, MUAs handle those details automatically.

Building and installing
-----------------------

Mimesis requires a C++20 compiler (tested with GCC 14 and Clang 19) and uses the
Meson build system. Ensure you have this installed. Use your operating system's
package manager if available, for example on Debian and its derivatives, use
`sudo apt install meson`. If that is not possible, download and install Meson
from https://mesonbuild.com/.

To build Mimesis, run these commands in the root directory of the source code:

    meson setup build
    ninja -C build

The test suite uses Catch2 v3. If it is not installed on your system (on Debian:
`sudo apt install catch2`), Meson will download it automatically. To run the
test suite, run:

    meson test -C build

To build without the test suite, add `-Dtests=disabled` to the setup command.

If Doxygen is installed, the API documentation can be generated with:

    meson compile -C build docs

The HTML documentation will be in `build/doc/html/`. To build and install it
by default, add `-Ddocs=enabled` to the setup command.

To install the binaries on your system, run:

    ninja -C build install

The latter command might ask for a root password if it detects the installation
directory is not writable by the current user.

By default, a debug version will be built, and the resulting libraries and
header files will be install inside `/usr/local/`. If you want to create a
release build and install it in `/usr/`, use the following commands:

    meson setup --buildtype=release --prefix=/usr build-release
    ninja -C build-release install

Distribution packagers that want to override all compiler flags should use
`--buildtype=plain`. The `DESTDIR` environment variable is supported by Meson.
