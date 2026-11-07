# Part of the C library on glibc, a separate library on other systems.
iconv = dependency('iconv')

# Only symbols marked with MIMESIS_EXPORT are visible in the shared library.
# The tests link the static library, so they can use the internal functions as well.
libmimesis_internal = static_library('mimesis-internal',
	'base64.cpp',
	'charset.cpp',
	'encoded-words.cpp',
	'mimesis.cpp',
	'parameters.cpp',
	'quoted-printable.cpp',
	dependencies: iconv,
	gnu_symbol_visibility: 'hidden',
	pic: true,
)

libmimesis = library('mimesis',
	link_whole: libmimesis_internal,
	# The ABI is not stable before version 1.0, so every release gets its own soname.
	version: meson.project_version(),
	soversion: meson.project_version(),
	install: true,
)

incdir = include_directories('.')
install_headers('mimesis.hpp')

pkg = import('pkgconfig')
pkg.generate(libmimesis, description: 'C++ library for RFC2822 message parsing and creation')
