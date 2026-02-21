
includes += ["./src"]

static_library(
    name = 'gnarl_lib',
    sources = [
        'src/gnarl/commandline.cc',
    ],
)

executable(
    name = 'gnarl',
    sources = [ 'src/gnarl/main.cc' ],
    deps = [ ':gnarl_lib' ],
)

