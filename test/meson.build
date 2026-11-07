catch2 = dependency('catch2-with-main', version: '>=3.0', required: get_option('tests'))

if catch2.found()
	tests = executable('tests',
		'build.cpp',
		'headers.cpp',
		'multipart.cpp',
		'parse.cpp',
		link_with: libmimesis,
		include_directories: incdir,
		dependencies: [catch2, dependency('threads')],
		cpp_args: '-DTEST_DATA_DIR="' + meson.current_source_dir() / 'input-clean' + '"',
	)
	test('mimesis', tests, protocol: 'tap', args: ['--reporter', 'tap'])
endif
