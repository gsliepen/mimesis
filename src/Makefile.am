lib_LTLIBRARIES = libmimesis.la

pkginclude_HEADERS = mimesis.hpp

pkgconfig_DATA = mimesis.pc

libmimesis_la_SOURCES = \
	base64.cpp base64.hpp \
	charset.cpp charset.hpp \
	mimesis.cpp mimesis.hpp \
	quoted-printable.cpp quoted-printable.hpp \
	string_view.hpp

libmimesis_la_LIBS = $(CODE_COVERAGE_LIBS)

AM_CXXFLAGS = $(CODE_COVERAGE_CXXFLAGS)
AM_CPPFLAGS = $(CODE_COVERAGE_CPPFLAGS)
