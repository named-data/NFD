# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'

from waflib import Build, Logs, Utils, Task, TaskGen, Configure

def options(opt):
    opt.load('compiler_cxx')
    opt.load('boost', tooldir=['.waf-tools'])

    nfdopt = opt.add_option_group('NFD Options')
    nfdopt.add_option('--debug',action='store_true',default=False,dest='debug',help='''Compile library debugging mode without all optimizations (-O0)''')
    nfdopt.add_option('--with-tests', action='store_true',default=False,dest='with_tests',help='''Build unit tests''')
    nfdopt.add_option('--with-ndn-cpp',action='store',type='string',default=None,dest='ndn_cpp_dir',
                      help='''Use NDN-CPP library from the specified path''')
    
def configure(conf):
    conf.load("compiler_cxx boost")

    if conf.options.debug:
        conf.define ('_DEBUG', 1)
        conf.add_supported_cxxflags (cxxflags = ['-O0',
                                                 '-Wall',
                                                 '-Wno-unused-variable',
                                                 '-g3',
                                                 '-Wno-unused-private-field', # only clang supports
                                                 '-fcolor-diagnostics',       # only clang supports
                                                 '-Qunused-arguments',        # only clang supports
                                                 '-Wno-tautological-compare', # suppress warnings from CryptoPP
                                                 '-Wno-unused-function',      # suppress warnings from CryptoPP
                                                 ])
    else:
        conf.add_supported_cxxflags (cxxflags = ['-O3', '-g', '-Wno-tautological-compare', '-Wno-unused-function'])

    if not conf.options.ndn_cpp_dir:
        conf.check_cfg(package='libndn-cpp-dev', args=['--cflags', '--libs'], uselib_store='NDN_CPP', mandatory=True)
    else:
        conf.check_cxx(lib='ndn-cpp-dev', uselib_store='NDN_CPP', 
                       cxxflags="-I%s/include" % conf.options.ndn_cpp_dir,
                       linkflags="-L%s/lib" % conf.options.ndn_cpp_dir,
                       mandatory=True)
        
    boost_libs='system'
    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = 1
        boost_libs+=' unit_test_framework'

    conf.check_boost(lib=boost_libs)
        
    boost_version = conf.env.BOOST_VERSION.split('_')
    if int(boost_version[0]) < 1 or int(boost_version[1]) < 42:
        Logs.error ("Minumum required boost version is 1.42")
        return
    
    conf.check_cxx(lib='rt', uselib_store='RT', define_name='HAVE_RT', mandatory=False)

    conf.write_config_header('daemon/config.hpp')

def build (bld):
    bld(target = "nfd-objects",
        features = "cxx",
        source = bld.path.ant_glob(['daemon/**/*.cpp'], excl=['daemon/main.cpp']),
        use = 'BOOST NDN_CPP RT',
        includes = [".", "daemon"],
        )

    bld(target = "nfd",
        features = "cxx cxxprogram",
        source = 'daemon/main.cpp',
        use = 'nfd-objects',
        )        
    
    # Unit tests
    if bld.env['WITH_TESTS']:
      unittests = bld.program (
          target="unit-tests",
          features = "cxx cxxprogram",
          source = bld.path.ant_glob(['tests/**/*.cpp']),
          use = 'nfd-objects',
          includes = [".", "daemon"],
          install_prefix = None,
          )

@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by compiler and add them to env.CXXFLAGS variable
    """
    self.start_msg('Checking allowed flags for c++ compiler')

    supportedFlags = []
    for flag in cxxflags:
        if self.check_cxx (cxxflags=[flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg (' '.join (supportedFlags))
    self.env.CXXFLAGS += supportedFlags
