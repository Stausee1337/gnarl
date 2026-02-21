import os
import sys
import shlex
import functools
import subprocess

from argparse import ArgumentParser, _StoreAction, _StoreTrueAction

SCRIPT_DIR = os.path.dirname(__file__)

# TODO:
# X take commandline options
# X use environment (CFLAGS, LDFLAGS)
# X correctly rebase paths (context.rebase_escape_path())
# X allow different compilers (CXX, AR)
# X compile commands database
# X automatic regen on builfile change
# _ multiple plaforms

T_STATIC_LIB = 0
T_SHARED_LIB = 1
T_EXECUTABLE = 2

def with_context(f):
    @functools.wraps(f)
    def inner(**kwargs):
        frame = sys._getframe(1)
        assert isinstance(context := frame.f_globals, Context)
        return f(context, **kwargs)
    return inner

def ensure_string(name, v):
    if not isinstance(v, str):
        raise TypeError(f"option {name!r} requires string")
    return v

def ensure_string_list(name, lst):
    if not isinstance(lst, list):
        raise TypeError(f"option {name!r} requires string list")
    out = []
    for v in lst:
        if not isinstance(v, str):
            raise TypeError(f"option {name!r} requires string list")
        out.append(v)
    return out

@with_context
def static_library(context, *, name, sources=[], ldflags=[]):
    name = ensure_string("name", name)
    sources = ensure_string_list("sources", sources)
    ldflags = ensure_string_list("ldflags", ldflags)

    context.define_target(
        T_STATIC_LIB, name, sources, [],
        env={'ldflags': ldflags}
    )

@with_context
def shared_library(context, *, name, sources=[], deps=[], ldflags=[]):
    name = ensure_string("name", name)
    sources = ensure_string_list("sources", sources)
    deps = ensure_string_list("deps", deps)
    ldflags = ensure_string_list("ldflags", ldflags)

    if len(deps) > 0:
        deps = context.resolve_deps(deps)

    context.define_target(
        T_SHARED_LIB, name, sources, deps,
        env={'ldflags': ldflags}
    )

@with_context
def executable(context, *, name, sources=[], deps=[], cflags=[]):
    name = ensure_string("name", name)
    sources = ensure_string_list("sources", sources)
    deps = ensure_string_list("deps", deps)
    cflags = ensure_string_list("cflags", cflags)

    if len(deps) > 0:
        deps = context.resolve_deps(deps)

    context.define_target(
        T_EXECUTABLE, name, sources, deps,
        env={'cflags': cflags}
    )

class Target:
    __slots__ = ('context', 'kind', 'name', 'sources', 'deps', 'env')

    def __init__(self, context, kind, name, sources, deps, env):
        self.context = context
        self.kind = kind
        self.name = name
        self.sources = sources
        self.deps = deps
        self.env = env

    def output_name(self):
        if self.kind == T_STATIC_LIB:
            return f"{self.name}{self.context.library_ext}"
        elif self.kind == T_SHARED_LIB:
            return f"{self.name}{self.context.shared_ext}"
        elif self.kind == T_EXECUTABLE:
            return f"{self.name}{self.context.executable_ext}"

class Context(dict):
    __slots__ = ('deps')

    def __init__(self):
        self["__builtins__"] = {
            "print": print,
            "static_library": static_library,
            "executable": executable,
        }
        self.deps = {}

    def __getattr__(self, name):
        _missing = object()
        if (value := self.get(name, _missing)) is not _missing:
            return value
        return self.__getattribute__(name)

    def resolve_deps(self, deps):
        resolved = []
        for dep in deps:
            if not dep.startswith(":"):
                raise ValueError("dependencies must be formatted ':depname'")
            if (target := self.deps.get(dep[1:])) is None:
                raise ValueError(f"unknown dependency {dep!r}")
            if target.kind == T_EXECUTABLE:
                raise TypeError(f"target {dep!r} of type executable is not a valid dependency")
            resolved.append(target)
        return resolved

    def define_target(self, kind, name, sources, deps, *, env):
        if name in self.deps.keys():
            raise NameError(f"redifinition of target {name!r}")
        if not name.isidentifier():
            raise NameError(f"invalid name of target {name!r}")
        self.deps[name] = Target(self, kind, name, sources, deps, env)

    def collect_targets(self):
        builder = BuildBuilder()
        for target in self.deps.values():
            objfiles = iterate_sources(self, builder, target.sources)
            if target.kind == T_STATIC_LIB:
                builder.ar(
                    target.output_name(),
                    objfiles,
                    [t.output_name() for t in target.deps],
                    {'libflags': self.libflags}
                )
            elif target.kind == T_SHARED_LIB or target.kind == T_EXECUTABLE:
                builder.link(
                    target.output_name(),
                    objfiles,
                    [t.output_name() for t in target.deps if t.kind == T_STATIC_LIB],
                    [t.output_name() for t in target.deps if t.kind == T_SHARED_LIB],
                    {'ldflags': self.ldflags}
                )
        return builder.stmts

    def rebase_escape_path(self, path, *, is_out):
        if not is_out:
            path = os.path.relpath(os.path.join(self.root, path), self.output_dir)
        return path.replace("$", "$$").replace(" ", "$ ").replace(":", "$:")

    def source_to_object(self, path):
        return f"{os.path.splitext(path)[0]}{self.object_ext}"

def iterate_sources(context, builder, sources):
    objfiles = []
    for sourcefile in sources:
        objfile = context.rebase_escape_path(context.source_to_object(sourcefile), is_out=True)
        sourcefile = context.rebase_escape_path(sourcefile, is_out=False)
        builder.cxx(sourcefile, objfile, {
            'includes': [f"-I{context.rebase_escape_path(i, is_out=False)}" for i in context.includes],
            'cflags': context.cflags,
        })
        objfiles.append(objfile)
    return objfiles

class BuildStmt:
    __slots__ = ('rule', 'inputs', 'outfile', 'filedeps', 'vars')

    def __init__(self, rule, inputs, outfile, filedeps, vars):
        self.rule = rule
        self.inputs = inputs
        self.outfile = outfile
        self.filedeps = filedeps
        self.vars = vars

    def write_into(self, file):
        inputs = " ".join(self.inputs) if isinstance(self.inputs, list) else self.inputs
        print(f"build {self.outfile}: {self.rule} {inputs}", end="", file=file)
        if len(self.filedeps) > 0:
            filedeps = " ".join(self.filedeps)
            print(f" | {filedeps}", end="", file=file)
        print(file=file)

        for key, value in self.vars.items():
            strvalue = " ".join(value) if isinstance(value, list) else value
            print(f"  {key} = {strvalue}", file=file)
        print(file=file)

class BuildBuilder:
    def __init__(self):
        self.stmts = []

    def build(self, rule, inputs, outfile, filedeps, vars):
        self.stmts.append(BuildStmt(rule, inputs, outfile, filedeps, vars))

    def cxx(self, sourcefile, objfile, vars):
        self.build("cxx", sourcefile, objfile, [], vars)

    def ar(self, name, objfiles, libs, vars):
        vars = {**vars, 'libs': libs}
        self.build("ar", objfiles, name, libs, vars)

    def link(self, name, objfiles, libs, solibs, vars):
        vars = {**vars, 'libs': libs, 'solibs': solibs}
        self.build("link", objfiles, name, libs + solibs, vars)

def setup_build_flags(context, options):
    cflags = os.environ.get('CFLAGS', ' ').split()
    ldflags = os.environ.get('LDFLAGS', ' ').split()
    libflags = os.environ.get('LIBFLAGS', ' ').split()

    if context.is_debug:
        cflags.extend(["-O0", "-g", "-D_LIBCPP_DEBUG=1", "-D_GLBCXX_DEBUG=1"])
    else:
        cflags.extend([
            "-DNDEBUG",
            "-O3",
            "-fdata-sections", "-ffunction-sections",
            "-Werror"
        ])

        ldflags.extend(["-fdata-sections", "-ffunction-sections"])
        if not options.no_strip:
            ldflags.extend(["-Wl,-strip-all"])

        if options.with_icf:
            ldflags.extend(["-Wl,--icf=all"])

        if options.with_lto:
            cflags.extend(["-flto", "-fwhole-program-vtables"])
            ldflags.extend(["-flto", "-fwhole-program-vtables"])

    if options.with_asan:
        cflags.extend(["-fsanitize=address", "-DASAN_ENABLED"])
        ldflags.extend(["-fsanitize=address"])

    if options.with_ubsan:
        cflags.extend(["-fsanitize=undefined"])
        ldflags.extend(["-fsanitize=undefined"])

    cflags.extend([
        "-D_FILE_OFFSET_BITS=64",
        "-D__STDC_CONSTANT_MACROS", "-D__STDC_FORMAT_MACROS",
        "-pthread",
        "-pipe",
        "-fno-exceptions",
        "-fno-rtti",
        "-fdiagnostics-color",
        "-Wall",
        "-Wextra",
        "-Wno-unused-parameter",

        "-Wextra-semi",
        "-Wundef",

        "-std=c++20",

        # "-Wno-deprecated-copy",
        # "-Wno-implicit-fallthrough",
        # "-Wno-redundant-move",
        # "-Wno-unused-variable",

        # dubious
        # "-Wno-format",
        # "-Wno-strict-aliasing",
        # "-Wno-cast-function-type"

        # gcc
        # "-Wno-restrict"
    ])

    ldflags.extend([
        "-Wl,--as-needed",
        "-static-libstdc++",
        "-pthread"
    ])

    context.update({
        "cflags": cflags,
        "ldflags": ldflags,
        "libflags": libflags
    })

def setup_context(context, options):
    context["is_debug"] = options.debug
    root = context["root"] = options.root
    output_dir = options.out_path
    if output_dir.startswith('//'):
        output_dir = os.path.join(root, output_dir[2:])
    elif not output_dir.startswith('/'):
        output_dir = os.path.abspath(output_dir)

    context["output_dir"] = output_dir

    context["includes"] = []

    cxx = context["cxx"] = os.environ.get("CXX", "clang++")
    context["ld"] = cxx
    context["ar"] = os.environ.get("AR", "ar")

    context["object_ext"] = ".o"
    context["library_ext"] = ".a"
    context["shared_ext"] = ".so"
    context["executable_ext"] = ""

def load_buildfile(root):
    try:
        filepath = os.path.join(root, "Buildfile")
        with open(filepath) as f_in:
            contents = f_in.read()
    except OSError as e:
        print(f"Invalid Root: Could not open Buildfile: {str(e)}", file=sys.stderr)
        return None

    try:
        code = compile(contents, filepath, "exec")
    except SyntaxError as err:
        print(f"Buildfile:{err.lineno}: SyntaxError: {str(err)}", file=sys.stderr)
        return None

    def inner(context):
        try:
            exec(code, context, {})
        except:
            exc_ty, exc, tb = sys.exc_info()

            stack = []
            while tb is not None:
                stack.append(tb)
                tb = tb.tb_next

            for tb in reversed(stack):
                if tb.tb_frame.f_code == code:
                    break
            else:
                raise

            
            print(f"Buildfile:{tb.tb_lineno}: {exc_ty.__name__}: {str(exc)}", file=sys.stderr)
            return None

        return context.collect_targets()

    return inner

def write_output(stmts, output_dir, root, rebuild_args, cxx, ld, ar, generate_compilation_database):
    with open(os.path.join(SCRIPT_DIR, "linux.ninja.tmpl")) as f_in:
        template = f_in.read()

    filepath = os.path.join(output_dir, "build.ninja")
    script_path = os.path.relpath(__file__, output_dir)

    with open(filepath, "w") as f_out:
        print(f"cxx = {cxx}", file=f_out)
        print(f"ld = {ld}", file=f_out)
        print(f"ar = {ar}", file=f_out)
        print(file=f_out)

        print("rule regen\n"
             f"  command = {sys.executable} {script_path}{rebuild_args}\n"
              "  description = Regenerating ninja files", file=f_out)
        f_out.write(template)

        buildfile_path = os.path.relpath(os.path.join(root, 'Buildfile'), output_dir)
        print(f"build build.ninja: regen | {buildfile_path}\n"
               "  generator = 1\n"
               "  depfile = build.ninja.d\n", file=f_out)

        for stmt in stmts:
            stmt.write_into(f_out)

    with open(f"{filepath}.d", "w") as f_out:
        f_out.write(f"build.ninja: {script_path}")

    if generate_compilation_database:
        with open(os.path.join(output_dir, "compile_commands.json"), "w") as f_out:
            subprocess.run(["ninja", "-C", output_dir, "-t", "compdb"], stdout=f_out, check=True)

def build_command_line_args(parser, options):
    args = []
    for action in parser._actions:
        dest = action.dest
        option = action.option_strings[-1]

        if (value := getattr(options, dest, None)) is None:
            continue
        
        match action:
            case _StoreTrueAction():
                if value:
                    args.append(option)
            case _StoreAction():
                args.append(f"{option}={value}")
            case _:
                assert False, f"unknown action {action}"

    args = " ".join(shlex.quote(a) for a in args)
    if len(args) > 0:
        return f" {args}"
    return ""

def main(argv):
    parser = ArgumentParser()
    parser.add_argument("-d", "--debug", action="store_true", help="Do a debug build")
    parser.add_argument("--with-lto", action="store_true", help="Enable LTO")
    parser.add_argument("--with-icf", action="store_true", help="Enable Identical Code Folding")
    parser.add_argument("--with-asan", action="store_true", help="Enable AddressSanitizer")
    parser.add_argument("--with-ubsan", action="store_true", help="Enable Undefined Behaviour Sanitizer")
    parser.add_argument("--root", type=str, default=os.getcwd(), help="Projects root directory")
    parser.add_argument("--out-path", type=str, default="//out",
                            help="The path to store build files in")
    parser.add_argument("--no-strip", action="store_true", help="Don't strip release build")
    parser.add_argument("--generate-compilation-database", action="store_true", help="Generate compile_commands.json")

    options = parser.parse_args(argv)


    context = Context()

    setup_context(context, options)
    setup_build_flags(context, options)

    build = load_buildfile(context.root)
    if build is None:
        exit(1)

    stmts = build(context)

    if not os.path.isdir(context.output_dir):
        os.makedirs(context.output_dir)

    args = build_command_line_args(parser, options)
    write_output(stmts, context.output_dir, context.root, args,
                    context.cxx, context.ld, context.ar,
                    options.generate_compilation_database)

if __name__ == '__main__':
    main(sys.argv[1:])

