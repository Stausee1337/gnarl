
includes += ["./src"]

static_library(
    name = 'gnarl_lib',
    sources = [
        'src/commandline.cc',
        'src/lexer.cc',
        'src/assertions.cc',
    ],
)

executable(
    name = 'gnarl',
    sources = [ 'src/main.cc' ],
    deps = [ ':gnarl_lib' ],
)

