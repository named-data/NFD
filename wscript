# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

"""
Copyright (c) 2014-2015,  Regents of the University of California,
                          Arizona Board of Regents,
                          Colorado State University,
                          University Pierre & Marie Curie, Sorbonne University,
                          Washington University in St. Louis,
                          Beijing Institute of Technology,
                          The University of Memphis.

This file is part of NFD (Named Data Networking Forwarding Daemon).
See AUTHORS.md for complete list of NFD authors and contributors.

NFD is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
"""

VERSION = "0.3.4"
APPNAME = "nfd"
BUGREPORT = "http://redmine.named-data.net/projects/nfd"
URL = "http://named-data.net/doc/NFD/"
GIT_TAG_PREFIX = "NFD-"

from waflib import Logs, Utils, Context
import os

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['boost', 'unix-socket', 'dependency-checker', 'websocket',
              'default-compiler-flags', 'coverage', 'pch', 'boost-kqueue',
              'doxygen', 'sphinx_build', 'type_traits', 'compiler-features'],
             tooldir=['.waf-tools'])

    nfdopt = opt.add_option_group('NFD Options')
    opt.addUnixOptions(nfdopt)
    opt.addWebsocketOptions(nfdopt)
    opt.addDependencyOptions(nfdopt, 'libpcap')
    nfdopt.add_option('--without-libpcap', action='store_true', default=False,
                      dest='without_libpcap',
                      help='''Disable libpcap (Ethernet face support will be disabled)''')

    opt.addDependencyOptions(nfdopt, 'librt',     '(optional)')
    opt.addDependencyOptions(nfdopt, 'libresolv', '(optional)')

    nfdopt.add_option('--with-tests', action='store_true', default=False,
                      dest='with_tests', help='''Build unit tests''')
    nfdopt.add_option('--with-other-tests', action='store_true', default=False,
                      dest='with_other_tests', help='''Build other tests''')

    nfdopt.add_option('--with-custom-logger', type='string', default=None,
                      dest='with_custom_logger',
                      help='''Path to custom-logger.hpp and custom-logger-factory.hpp '''
                           '''implementing Logger and LoggerFactory interfaces''')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'pch', 'boost-kqueue',
               'boost', 'dependency-checker', 'websocket',
               'doxygen', 'sphinx_build', 'type_traits', 'compiler-features'])

    conf.find_program('bash', var='BASH')

    if 'PKG_CONFIG_PATH' not in os.environ:
        os.environ['PKG_CONFIG_PATH'] = Utils.subst_vars('${LIBDIR}/pkgconfig', conf.env)
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.checkDependency(name='librt', lib='rt', mandatory=False)
    conf.checkDependency(name='libresolv', lib='resolv', mandatory=False)

    if not conf.check_cxx(msg='Checking if privilege drop/elevation is supported', mandatory=False,
                          define_name='HAVE_PRIVILEGE_DROP_AND_ELEVATE', fragment='''
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
int
main(int, char**)
{
  ::sysconf(_SC_GETGR_R_SIZE_MAX);
  group grp;
  getgrnam_r("nogroup", &grp, nullptr, 0, nullptr);
  passwd pwd;
  getpwnam_r("nobody", &pwd, nullptr, 0, nullptr);

  int ret = setegid(grp.gr_gid);
  ret = seteuid(pwd.pw_uid);
  (void)(ret);
  return 0;
}
'''):
      Logs.warn('Dropping privileges is not supported on this platform')

    conf.check_cxx(header_name='ifaddrs.h', mandatory=False)

    boost_libs = 'system chrono program_options random thread'
    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = 1
        conf.define('WITH_TESTS', 1);
        boost_libs += ' unit_test_framework'

    if conf.options.with_other_tests:
        conf.env['WITH_OTHER_TESTS'] = 1

    conf.check_boost(lib=boost_libs)
    if conf.env.BOOST_VERSION_NUMBER < 104800:
        Logs.error("Minimum required boost version is 1.48.0")
        Logs.error("Please upgrade your distribution or install custom boost libraries" +
                   " (http://redmine.named-data.net/projects/nfd/wiki/Boost_FAQ)")
        return

    conf.load('unix-socket')
    conf.checkWebsocket(mandatory=True)

    if not conf.options.without_libpcap:
        conf.check_asio_pcap_support()
        if conf.env['HAVE_ASIO_PCAP_SUPPORT']:
            conf.checkDependency(name='libpcap', lib='pcap', mandatory=True,
                                 errmsg='not found, but required for Ethernet face support. '
                                        'Specify --without-libpcap to disable Ethernet face support.')
        else:
            Logs.warn('Warning: Ethernet face support is not supported on this platform with Boost libraries version 1.56. '
                      'See http://redmine.named-data.net/issues/1877 for more details')
    if conf.env['HAVE_LIBPCAP']:
        conf.check_cxx(function_name='pcap_set_immediate_mode', header_name='pcap/pcap.h',
                       cxxflags='-Wno-error', use='LIBPCAP', mandatory=False)

    if conf.options.with_custom_logger:
        conf.define('HAVE_CUSTOM_LOGGER', 1)
        conf.env['INCLUDES_CUSTOM_LOGGER'] = [conf.options.with_custom_logger]
        conf.env['HAVE_CUSTOM_LOGGER'] = 1

    conf.load('coverage')

    conf.define('DEFAULT_CONFIG_FILE', '%s/ndn/nfd.conf' % conf.env['SYSCONFDIR'])

    # disable assertions in release builds
    if not conf.options.debug:
        conf.define('NDEBUG', 1)

    conf.write_config_header('config.hpp')

def build(bld):
    version(bld)

    bld(features="subst",
        name='version',
        source='version.hpp.in',
        target='version.hpp',
        install_path=None,
        VERSION_STRING=VERSION_BASE,
        VERSION_BUILD=VERSION,
        VERSION=int(VERSION_SPLIT[0]) * 1000000 +
                int(VERSION_SPLIT[1]) * 1000 +
                int(VERSION_SPLIT[2]),
        VERSION_MAJOR=VERSION_SPLIT[0],
        VERSION_MINOR=VERSION_SPLIT[1],
        VERSION_PATCH=VERSION_SPLIT[2],
        )

    core = bld(
        target='core-objects',
        name='core-objects',
        features='cxx pch',
        source=bld.path.ant_glob(['core/**/*.cpp'],
                                 excl=['core/logger*.cpp']),
        use='version BOOST NDN_CXX LIBRT',
        includes='. core',
        export_includes='. core',
        headers='common.hpp',
        )

    if bld.env['HAVE_CUSTOM_LOGGER']:
        core.use += " CUSTOM_LOGGER"
    else:
        core.source += bld.path.ant_glob(['core/logger*.cpp'])

    nfd_objects = bld(
        target='daemon-objects',
        name='daemon-objects',
        features='cxx',
        source=bld.path.ant_glob(['daemon/**/*.cpp'],
                                 excl=['daemon/face/ethernet-*.cpp',
                                       'daemon/face/unix-*.cpp',
                                       'daemon/face/websocket-*.cpp',
                                       'daemon/main.cpp']),
        use='core-objects WEBSOCKET',
        includes='daemon',
        export_includes='daemon',
        )

    if bld.env['HAVE_LIBPCAP']:
        nfd_objects.source += bld.path.ant_glob('daemon/face/ethernet-*.cpp')
        nfd_objects.use += ' LIBPCAP'

    if bld.env['HAVE_UNIX_SOCKETS']:
        nfd_objects.source += bld.path.ant_glob('daemon/face/unix-*.cpp')

    if bld.env['HAVE_WEBSOCKET']:
        nfd_objects.source += bld.path.ant_glob('daemon/face/websocket-*.cpp')

    rib_objects = bld(
        target='rib-objects',
        name='rib-objects',
        features='cxx',
        source=bld.path.ant_glob(['rib/**/*.cpp']),
        use='core-objects',
        )

    bld(target='bin/nfd',
        features='cxx cxxprogram',
        source='daemon/main.cpp',
        use='daemon-objects rib-objects',
        )

    bld.recurse("tools")
    bld.recurse("tests")

    bld(features="subst",
        source='nfd.conf.sample.in',
        target='nfd.conf.sample',
        install_path="${SYSCONFDIR}/ndn",
        IF_HAVE_LIBPCAP="" if bld.env['HAVE_LIBPCAP'] else "; ",
        IF_HAVE_WEBSOCKET="" if bld.env['HAVE_WEBSOCKET'] else "; ")

    if bld.env['SPHINX_BUILD']:
        bld(features="sphinx",
            builder="man",
            outdir="docs/manpages",
            config="docs/conf.py",
            source=bld.path.ant_glob('docs/manpages/**/*.rst'),
            install_path="${MANDIR}/",
            VERSION=VERSION)

def docs(bld):
    from waflib import Options
    Options.commands = ['doxygen', 'sphinx'] + Options.commands

def doxygen(bld):
    version(bld)

    if not bld.env.DOXYGEN:
        Logs.error("ERROR: cannot build documentation (`doxygen' is not found in $PATH)")
    else:
        bld(features="subst",
            name="doxygen-conf",
            source=["docs/doxygen.conf.in",
                    "docs/named_data_theme/named_data_footer-with-analytics.html.in"],
            target=["docs/doxygen.conf",
                    "docs/named_data_theme/named_data_footer-with-analytics.html"],
            VERSION=VERSION_BASE,
            HTML_FOOTER="../build/docs/named_data_theme/named_data_footer-with-analytics.html" \
                          if os.getenv('GOOGLE_ANALYTICS', None) \
                          else "../docs/named_data_theme/named_data_footer.html",
            GOOGLE_ANALYTICS=os.getenv('GOOGLE_ANALYTICS', ""),
            )

        bld(features="doxygen",
            doxyfile='docs/doxygen.conf',
            use="doxygen-conf")

def sphinx(bld):
    version(bld)

    if not bld.env.SPHINX_BUILD:
        bld.fatal("ERROR: cannot build documentation (`sphinx-build' is not found in $PATH)")
    else:
        bld(features="sphinx",
            outdir="docs",
            source=bld.path.ant_glob('docs/**/*.rst'),
            config="docs/conf.py",
            VERSION=VERSION_BASE)

def version(ctx):
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = [v for v in VERSION_BASE.split('.')]

    didGetVersion = False
    try:
        cmd = ['git', 'describe', '--always', '--match', '%s*' % GIT_TAG_PREFIX]
        p = Utils.subprocess.Popen(cmd, stdout=Utils.subprocess.PIPE,
                                   stderr=None, stdin=None)
        out = str(p.communicate()[0].strip())
        didGetVersion = (p.returncode == 0 and out != "")
        if didGetVersion:
            if out.startswith(GIT_TAG_PREFIX):
                Context.g_module.VERSION = out[len(GIT_TAG_PREFIX):]
            else:
                Context.g_module.VERSION = "%s-commit-%s" % (Context.g_module.VERSION_BASE, out)
    except OSError:
        pass

    versionFile = ctx.path.find_node('VERSION')

    if not didGetVersion and versionFile is not None:
        try:
            Context.g_module.VERSION = versionFile.read()
            return
        except (OSError, IOError):
            pass

    # version was obtained from git, update VERSION file if necessary
    if versionFile is not None:
        try:
            version = versionFile.read()
            if version == Context.g_module.VERSION:
                return # no need to update
        except (OSError, IOError):
            Logs.warn("VERSION file exists, but not readable")
    else:
        versionFile = ctx.path.make_node('VERSION')

    if versionFile is None:
        return

    try:
        versionFile.write(Context.g_module.VERSION)
    except (OSError, IOError):
        Logs.warn("VERSION file is not writeable")

def dist(ctx):
    version(ctx)

def distcheck(ctx):
    version(ctx)
