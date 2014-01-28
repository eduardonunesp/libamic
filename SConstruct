import os

libname = "libamic"
libs = [File('libuv/.libs/libuv.a')]
paths = ['.', 'include', 'libuv/include']
build_path = 'build'

env = Environment(LIBS = libs, CPPPATH = paths, CPPFLAGS = '-g -ggdb')

source = Split("""
src/conn.c
src/ast.c
src/loop.c
""")

if not os.path.exists('libuv/.libs/libuv.a'):
    os.chdir('libuv')
    os.system('./configure --enable-static')
    os.system('make')

env.StaticLibrary('%s/%s' % (build_path, libname), source)
