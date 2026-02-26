
includes += ["./src"]

static_library(
    name = 'gnarl_lib',
    sources = [
        'src/error.cc',
        'src/assertions.cc',
        'src/commandline.cc',
        'src/lexer.cc',
        'src/parser.cc',
        'src/nodes.cc',
        'src/value.cc',
        'src/scope.cc',
        'src/evaluate.cc',
        'src/literals.cc',
    ],
)

executable(
    name = 'gnarl',
    sources = [ 'src/main.cc' ],
    deps = [ ':gnarl_lib' ],
)

