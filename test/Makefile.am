TESTS = \
	parse-reproduce.test

dist_check_SCRIPTS = $(TESTS)

AM_CPPFLAGS = -I../src

check_PROGRAMS = \
	parse-reproduce

parse_reproduce_SOURCES = parse-reproduce.cpp
parse_reproduce_LDADD = ../src/libmimesis.la
