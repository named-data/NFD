# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'

from waflib import Build, Logs, Utils, Task, TaskGen, Configure
import os

def options(opt):
    opt.load('compiler_cxx gnu_dirs')
    opt.load('boost doxygen coverage unix-socket', tooldir=['.waf-tools'])

    nfdopt = opt.add_option_group('NFD Options')
    nfdopt.add_option('--debug',action='store_true',default=False,dest='debug',help='''Compile library debugging mode without all optimizations (-O0)''')
    nfdopt.add_option('--with-tests', action='store_true',default=False,dest='with_tests',help='''Build unit tests''')
    nfdopt.add_option('--with-ndn-cpp',action='store',type='string',default=None,dest='ndn_cpp_dir',
                      help='''Use NDN-CPP library from the specified path''')

def configure(conf):
    conf.load("compiler_cxx boost gnu_dirs")
    try:
        conf.load("doxygen")
    except:
        pass

    if conf.options.debug:
        conf.define('_DEBUG', 1)
        conf.add_supported_cxxflags(cxxflags = ['-O0',
                                                '-Wall',
                                                '-Wno-unused-variable',
                                                '-g3',
                                                '-Wno-unused-private-field', # only clang supports
                                                '-fcolor-diagnostics',       # only clang supports
                                                '-Qunused-arguments',        # only clang supports
                                                '-Wno-tautological-compare', # suppress warnings from CryptoPP
                                                '-Wno-unused-function',      # suppress warnings from CryptoPP
                                                '-fno-inline',
                                                ])
    else:
        conf.add_supported_cxxflags(cxxflags = ['-O3', '-g', '-Wno-tautological-compare', '-Wno-unused-function'])

    if not conf.options.ndn_cpp_dir:
        conf.check_cfg(package='libndn-cpp-dev', args=['--cflags', '--libs'], uselib_store='NDN_CPP', mandatory=True)
    else:
        conf.check_cxx(lib='ndn-cpp-dev', uselib_store='NDN_CPP',
                       cxxflags="-I%s/include" % conf.options.ndn_cpp_dir,
                       linkflags="-L%s/lib" % conf.options.ndn_cpp_dir,
                       mandatory=True)

    boost_libs = 'system chrono program_options'
    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = 1
        conf.define('WITH_TESTS', 1);
        boost_libs+=' unit_test_framework'

    conf.check_boost(lib=boost_libs)

    if conf.env.BOOST_VERSION_NUMBER < 104800:
        Logs.error("Minimum required boost version is 1.48.0")
        Logs.error("Please upgrade your distribution or install custom boost libraries" +
                   " (http://redmine.named-data.net/projects/nfd/wiki/Boost_FAQ)")
        return

    conf.load('unix-socket')

    conf.check_cxx(lib='rt', uselib_store='RT', define_name='HAVE_RT', mandatory=False)
    if conf.check_cxx(lib='pcap', uselib_store='PCAP', define_name='HAVE_PCAP', mandatory=False):
        conf.env['HAVE_PCAP'] = True
    
    conf.check_cxx(lib='resolv', uselib_store='RESOLV', mandatory=True)
    
    conf.load('coverage')

    conf.define('DEFAULT_CONFIG_FILE', '%s/ndn/nfd.conf' % conf.env['SYSCONFDIR'])

    conf.write_config_header('daemon/config.hpp')

def build(bld):
    nfd_objects = bld(
        target = "nfd-objects",
        features = "cxx",
        source = bld.path.ant_glob(['daemon/**/*.cpp'],
                                   excl=['daemon/face/ethernet-*.cpp',
                                         'daemon/face/unix-*.cpp',
                                         'daemon/main.cpp']),
        use = 'BOOST NDN_CPP RT',
        includes = [".", "daemon"],
        )

    if bld.env['HAVE_PCAP']:
        nfd_objects.source += bld.path.ant_glob('daemon/face/ethernet-*.cpp')
        nfd_objects.use += ' PCAP'

    if bld.env['HAVE_UNIX_SOCKETS']:
        nfd_objects.source += bld.path.ant_glob('daemon/face/unix-*.cpp')

    bld(target = "nfd",
        features = "cxx cxxprogram",
        source = 'daemon/main.cpp',
        use = 'nfd-objects',
        includes = [".", "daemon"],
        )

    for app in bld.path.ant_glob('tools/*.cpp'):
        bld(features=['cxx', 'cxxprogram'],
            target = 'bin/%s' % (str(app.change_ext(''))),
            source = ['tools/%s' % (str(app))],
            includes = [".", "daemon"],
            use = 'nfd-objects RESOLV',
            )

    # Unit tests
    if bld.env['WITH_TESTS']:
        unit_tests = bld.program(
            target="unit-tests",
            features = "cxx cxxprogram",
            source = bld.path.ant_glob(['tests/**/*.cpp'],
                                       excl=['tests/face/ethernet.cpp',
                                             'tests/face/unix-*.cpp']),
            use = 'nfd-objects',
            includes = [".", "daemon"],
            install_prefix = None,
          )

        if bld.env['HAVE_PCAP']:
            unit_tests.source += bld.path.ant_glob('tests/face/ethernet.cpp')

        if bld.env['HAVE_UNIX_SOCKETS']:
            unit_tests.source += bld.path.ant_glob('tests/face/unix-*.cpp')

    bld(features = "subst",
        source = 'nfd.conf.sample.in',
        target = 'nfd.conf.sample',
        install_path = "${SYSCONFDIR}/ndn")

@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by compiler and add them to env.CXXFLAGS variable
    """
    self.start_msg('Checking allowed flags for c++ compiler')

    supportedFlags = []
    for flag in cxxflags:
        if self.check_cxx(cxxflags=[flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg(' '.join (supportedFlags))
    self.env.CXXFLAGS += supportedFlags

# doxygen docs
from waflib.Build import BuildContext
class doxy(BuildContext):
    cmd = "doxygen"
    fun = "doxygen"

def doxygen(bld):
    if not bld.env.DOXYGEN:
        bld.fatal("ERROR: cannot build documentation (`doxygen' is not found in $PATH)")
    bld(features="doxygen",
        doxyfile='docs/doxygen.conf')
