doxygen = find_program('doxygen', required: get_option('docs'))

if doxygen.found()
	doxyfile = configure_file(
		input: 'Doxyfile.in',
		output: 'Doxyfile',
		configuration: {
			'VERSION': meson.project_version(),
			'SOURCE_ROOT': meson.project_source_root(),
			'OUTPUT_DIRECTORY': meson.current_build_dir(),
			'HAVE_DOT': find_program('dot', required: false).found() ? 'YES' : 'NO',
		},
	)

	custom_target('docs',
		input: doxyfile,
		output: 'html',
		command: [doxygen, '@INPUT@'],
		depend_files: files('../src/mimesis.hpp', '../README.md'),
		build_by_default: get_option('docs').enabled(),
		install: get_option('docs').enabled(),
		install_dir: get_option('datadir') / 'doc' / meson.project_name().to_lower(),
	)
endif
