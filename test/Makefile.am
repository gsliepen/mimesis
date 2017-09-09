TESTS = \
	parse-reproduce.test \
	describe.test \
	build-lowlevel.test

dist_check_SCRIPTS = $(TESTS)

AM_CPPFLAGS = -I../src

check_PROGRAMS = \
	parse-reproduce \
	describe \
	build-lowlevel

parse_reproduce_SOURCES = parse-reproduce.cpp
parse_reproduce_LDADD = ../src/libmimesis.la

describe_SOURCES = describe.cpp
describe_LDADD = ../src/libmimesis.la

build_lowlevel_SOURCES = build-lowlevel.cpp
build_lowlevel_LDADD = ../src/libmimesis.la
