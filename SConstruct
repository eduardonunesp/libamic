import os

libname = "libamic"
libs = [File('libuv/.libs/libuv.a')]
paths = ['.', 'include', 'libuv/include', 'c_hashmap']
build_path = 'build'

envlib = Environment(LIBS = libs, 
                     CPPPATH = paths, 
                     CPPFLAGS = '-g -ggdb -Wno-extra-tokens')

source = Split("""
c_hashmap/hashmap.c
src/conn.c
src/ast.c
src/loop.c
src/utils.c
""")

if not os.path.exists('libuv/configure'):
    os.chdir('libuv')
    os.system('./autogen.sh')
    os.system('./configure --enable-static')
    os.system('make')

envlib.StaticLibrary('%s/%s' % (build_path, libname), source)

envsample = Environment(LIBS = [File('build/libamic.a'), File('libuv/.libs/libuv.a')],
                        CPPPATH = paths, 
                        CPPFLAGS = '-g -ggdb -Wno-extra-tokens')

source = Split("""
samples/amic_test.c
""")

envsample.Program('%s/%s' % ('samples', 'amic_test'), source)
