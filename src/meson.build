libmimesis = library('mimesis',
	'base64.cpp',
	'charset.cpp',
	'encoded-words.cpp',
	'mimesis.cpp',
	'parameters.cpp',
	'quoted-printable.cpp',
	# The ABI is not stable before version 1.0, so every release gets its own soname.
	version: meson.project_version(),
	soversion: meson.project_version(),
	install: true,
)

incdir = include_directories('.')
install_headers('mimesis.hpp')

pkg = import('pkgconfig')
pkg.generate(libmimesis, description: 'C++ library for RFC2822 message parsing and creation')
