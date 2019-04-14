binaries = [
	'parse-reproduce',
	'describe',
	'build-lowlevel',
	'build-highlevel',
	'headers',
	'multipart',
	'load-save',
]

input_clean = [
	'input-clean/01-simple',
	'input-clean/02-multipart-alternative',
	'input-clean/03-multipart-mixed',
	'input-clean/04-nested-multipart',
	'input-clean/05-nested-message-rfc822',
]

incdir = include_directories('../src')

test('parse-reproduce', executable('parse-reproduce', 'parse-reproduce.cpp', link_with: libmimesis, include_directories: incdir), args: files(input_clean))
test('describe', executable('describe', 'describe.cpp', link_with: libmimesis, include_directories: incdir), args: files(input_clean))
test('build-highlevel', executable('build-highlevel', 'build-highlevel.cpp', link_with: libmimesis, include_directories: incdir))
test('build-lowlevel', executable('build-lowlevel', 'build-lowlevel.cpp', link_with: libmimesis, include_directories: incdir))
test('headers', executable('headers', 'headers.cpp', link_with: libmimesis, include_directories: incdir))
test('multipart', executable('multipart', 'multipart.cpp', link_with: libmimesis, include_directories: incdir))
test('load-save', executable('load-save', 'load-save.cpp', link_with: libmimesis, include_directories: incdir), args: files(input_clean))
