catch2 = dependency('catch2-with-main', version: '>=3.0', required: get_option('tests'))

if catch2.found()
	tests = executable('tests',
		'build.cpp',
		'codecs.cpp',
		'headers.cpp',
		'misc.cpp',
		'multipart.cpp',
		'parameters.cpp',
		'parse.cpp',
		link_with: libmimesis_internal,
		include_directories: incdir,
		dependencies: [catch2, dependency('threads')],
		cpp_args: '-DTEST_DATA_DIR="' + meson.current_source_dir() / 'input-clean' + '"',
	)
	test('mimesis', tests, protocol: 'tap', args: ['--reporter', 'tap'])

	shared_library_tests = executable('shared-library-tests',
		'shared-library.cpp',
		link_with: libmimesis,
		include_directories: incdir,
		dependencies: catch2,
	)
	test('shared library', shared_library_tests, protocol: 'tap', args: ['--reporter', 'tap'])
endif
