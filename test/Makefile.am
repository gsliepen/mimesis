TESTS = \
	parse-reproduce.test \
	describe.test \
	build-lowlevel.test \
	build-highlevel.test \
	headers \
	multipart

dist_check_SCRIPTS = $(TESTS)

AM_CPPFLAGS = -I../src

check_PROGRAMS = \
	parse-reproduce \
	describe \
	build-lowlevel \
	build-highlevel \
	headers \
	multipart

clean-local:
	-rm -rf reproduced described built-lowlevel built-highlevel idempotent

parse_reproduce_SOURCES = parse-reproduce.cpp
parse_reproduce_LDADD = ../src/libmimesis.la

describe_SOURCES = describe.cpp
describe_LDADD = ../src/libmimesis.la

build_lowlevel_SOURCES = build-lowlevel.cpp
build_lowlevel_LDADD = ../src/libmimesis.la

build_highlevel_SOURCES = build-highlevel.cpp
build_highlevel_LDADD = ../src/libmimesis.la

headers_SOURCES = headers.cpp
headers_LDADD = ../src/libmimesis.la

multipart_SOURCES = multipart.cpp
multipart_LDADD = ../src/libmimesis.la
