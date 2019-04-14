libmimesis = library('mimesis',
	'base64.cpp',
	'charset.cpp',
	'mimesis.cpp',
	'quoted-printable.cpp',
	install: true
)

incdir = include_directories('.')
install_headers('mimesis.hpp', subdir: '')

pkg = import('pkgconfig')
pkg.generate(libmimesis, subdirs: '', description: 'C++ library for RFC2822 message parsing and creation')
