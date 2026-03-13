
includes += ["./src"]

static_library(
    name = 'gnarl_lib',
    sources = [
        'src/workspace.cc',
        'src/error.cc',
        'src/assertions.cc',
        'src/commandline.cc',
        'src/lexer.cc',
        'src/parser.cc',
        'src/nodes.cc',
        'src/value.cc',
        'src/scope.cc',
        'src/file_scope.cc',
        'src/evaluate.cc',
        'src/literals.cc',
        'src/functions.cc',
        'src/input_file.cc',
        'src/format.cc',
        'src/integer.cc',
        'src/path_io.cc',
    ],
)

executable(
    name = 'gnarl',
    sources = [ 'src/main.cc' ],
    deps = [ ':gnarl_lib' ],
)

