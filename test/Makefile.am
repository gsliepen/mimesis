TESTS = \
	parse-reproduce.test \
	describe.test \
	build-lowlevel.test \
	build-highlevel.test \
	headers.test \
	multipart.test \
	load-save.test

dist_check_SCRIPTS = $(TESTS)

AM_CPPFLAGS = -I../src
LDADD = ../src/libmimesis.la

check_PROGRAMS = \
	parse-reproduce \
	describe \
	build-lowlevel \
	build-highlevel \
	headers \
	multipart \
	load-save

parse_reproduce_SOURCES = parse-reproduce.cpp
describe_SOURCES = describe.cpp
build_lowlevel_SOURCES = build-lowlevel.cpp
build_highlevel_SOURCES = build-highlevel.cpp
headers_SOURCES = headers.cpp
multipart_SOURCES = multipart.cpp
load_save_SOURCES = load-save.cpp

clean-local:
	-rm -rf reproduced described built-lowlevel built-highlevel load-saved idempotent
