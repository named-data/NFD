# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Build, Logs, Utils, Task, TaskGen, Configure
import os

def options(opt):
    opt.load('compiler_cxx gnu_dirs')
    opt.load('flags boost doxygen coverage', tooldir=['.waf-tools'])
    
    nrdopt = opt.add_option_group('NRD Options')
    nrdopt.add_option('--debug',action='store_true',default=False,dest='debug',help='''Compile library debugging mode without all optimizations (-O0)''')
    nrdopt.add_option('--with-tests', action='store_true',default=False,dest='with_tests',help='''Build unit tests''')

def configure(conf):
    conf.load("compiler_cxx gnu_dirs boost flags")

    conf.check_cfg(package='libndn-cpp-dev', args=['--cflags', '--libs'], uselib_store='NDN_CPP', mandatory=True)

    boost_libs='system'
    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = 1
        conf.define('WITH_TESTS', 1);
        boost_libs+=' unit_test_framework'

    conf.check_boost(lib=boost_libs)
    if conf.env.BOOST_VERSION_NUMBER < 104800:
        Logs.error ("Minimum required boost version is 1.48")
        Logs.error ("Please upgrade your distribution or install custom boost libraries" +
                    " (http://redmine.named-data.net/projects/nfd/wiki/Boost_FAQ)")
        return

    # conf.load('coverage')

    # try:
    #     conf.load('doxygen')
    # except:
    #     pass

    conf.write_config_header('config.hpp')

def build (bld):
    bld (
        features=['cxx', 'cxxprogram'],
        target="nrd",
        source = bld.path.ant_glob('*.cpp'),
        use = 'NDN_CPP BOOST',
        )

