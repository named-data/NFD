# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
"""
Copyright (c) 2014-2018,  Regents of the University of California,
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

from waflib import Context, Logs, Utils
import os, subprocess

VERSION = '0.6.2'
APPNAME = 'nfd'
BUGREPORT = 'https://redmine.named-data.net/projects/nfd'
URL = 'https://named-data.net/doc/NFD/'
GIT_TAG_PREFIX = 'NFD-'

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags', 'compiler-features', 'type_traits',
              'coverage', 'pch', 'sanitizers', 'boost', 'boost-kqueue',
              'dependency-checker', 'unix-socket', 'websocket',
              'doxygen', 'sphinx_build'],
             tooldir=['.waf-tools'])

    nfdopt = opt.add_option_group('NFD Options')
    opt.addUnixOptions(nfdopt)
    opt.addWebsocketOptions(nfdopt)

    opt.addDependencyOptions(nfdopt, 'libpcap')
    nfdopt.add_option('--without-libpcap', action='store_true', default=False,
                      help='Disable libpcap (Ethernet face support will be disabled)')

    opt.addDependencyOptions(nfdopt, 'librt')
    opt.addDependencyOptions(nfdopt, 'libresolv')

    nfdopt.add_option('--with-tests', action='store_true', default=False,
                      help='Build unit tests')
    nfdopt.add_option('--with-other-tests', action='store_true', default=False,
                      help='Build other tests')

    nfdopt.add_option('--with-custom-logger', metavar='PATH',
                      help='Path to custom-logger.hpp and custom-logger-factory.hpp '
                           'implementing Logger and LoggerFactory interfaces')

PRIVILEGE_CHECK_CODE = '''
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
int main()
{
  sysconf(_SC_GETGR_R_SIZE_MAX);

  char buffer[100];
  group grp;
  group* grpRes;
  getgrnam_r("nogroup", &grp, buffer, 100, &grpRes);
  passwd pwd;
  passwd* pwdRes;
  getpwnam_r("nobody", &pwd, buffer, 100, &pwdRes);

  int ret = setegid(grp.gr_gid);
  ret = seteuid(pwd.pw_uid);
  (void)(ret);

  getegid();
  geteuid();
}
'''

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'compiler-features', 'type_traits',
               'pch', 'boost', 'boost-kqueue', 'dependency-checker', 'websocket',
               'doxygen', 'sphinx_build'])

    conf.find_program('bash', var='BASH')

    if 'PKG_CONFIG_PATH' not in os.environ:
        os.environ['PKG_CONFIG_PATH'] = Utils.subst_vars('${LIBDIR}/pkgconfig', conf.env)
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.checkDependency(name='librt', lib='rt', mandatory=False)
    conf.checkDependency(name='libresolv', lib='resolv', mandatory=False)

    if not conf.check_cxx(msg='Checking if privilege drop/elevation is supported', mandatory=False,
                          define_name='HAVE_PRIVILEGE_DROP_AND_ELEVATE', fragment=PRIVILEGE_CHECK_CODE):
        Logs.warn('Dropping privileges is not supported on this platform')

    conf.check_cxx(header_name='valgrind/valgrind.h', define_name='HAVE_VALGRIND', mandatory=False)

    if conf.options.with_tests:
        conf.env.WITH_TESTS = True
        conf.define('WITH_TESTS', 1)

    if conf.options.with_other_tests:
        conf.env.WITH_OTHER_TESTS = True
        conf.define('WITH_OTHER_TESTS', 1)

    boost_libs = 'system chrono program_options thread log log_setup'
    if conf.options.with_tests or conf.options.with_other_tests:
        boost_libs += ' unit_test_framework'

    conf.check_boost(lib=boost_libs, mt=True)
    if conf.env.BOOST_VERSION_NUMBER < 105400:
        conf.fatal('Minimum required Boost version is 1.54.0\n'
                   'Please upgrade your distribution or manually install a newer version of Boost'
                   ' (https://redmine.named-data.net/projects/nfd/wiki/Boost_FAQ)')

    if conf.env.CXX_NAME == 'clang' and conf.env.BOOST_VERSION_NUMBER < 105800:
        conf.define('BOOST_ASIO_HAS_STD_ARRAY', 1) # Workaround for https://redmine.named-data.net/issues/3360#note-14
        conf.define('BOOST_ASIO_HAS_STD_CHRONO', 1) # Solution documented at https://redmine.named-data.net/issues/3588#note-10

    conf.load('unix-socket')
    conf.checkWebsocket(mandatory=True)

    if not conf.options.without_libpcap:
        conf.check_asio_pcap_support()
        if conf.env.HAVE_ASIO_PCAP_SUPPORT:
            conf.checkDependency(name='libpcap', lib='pcap', mandatory=True,
                                 errmsg='not found, but required for Ethernet face support. '
                                        'Specify --without-libpcap to disable Ethernet face support.')
        else:
            Logs.warn('Warning: Ethernet face is not supported on this platform with Boost libraries version 1.56. '
                      'See https://redmine.named-data.net/issues/1877 for more details')
    if conf.env.HAVE_LIBPCAP:
        conf.check_cxx(msg='Checking if libpcap supports pcap_set_immediate_mode', mandatory=False,
                       define_name='HAVE_PCAP_SET_IMMEDIATE_MODE', use='LIBPCAP',
                       fragment='''#include <pcap/pcap.h>
                                   int main() { pcap_t* p; pcap_set_immediate_mode(p, 1); }''')

    if conf.options.with_custom_logger:
        conf.define('HAVE_CUSTOM_LOGGER', 1)
        conf.env.HAVE_CUSTOM_LOGGER = True
        conf.env.INCLUDES_CUSTOM_LOGGER = [conf.options.with_custom_logger]

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')
    conf.load('sanitizers')

    conf.define('DEFAULT_CONFIG_FILE', '%s/ndn/nfd.conf' % conf.env.SYSCONFDIR)
    # disable assertions in release builds
    if not conf.options.debug:
        conf.define('NDEBUG', 1)
    conf.write_config_header('core/config.hpp')

def build(bld):
    version(bld)

    bld(features='subst',
        name='version.hpp',
        source='core/version.hpp.in',
        target='core/version.hpp',
        install_path=None,
        VERSION_STRING=VERSION_BASE,
        VERSION_BUILD=VERSION,
        VERSION=int(VERSION_SPLIT[0]) * 1000000 +
                int(VERSION_SPLIT[1]) * 1000 +
                int(VERSION_SPLIT[2]),
        VERSION_MAJOR=VERSION_SPLIT[0],
        VERSION_MINOR=VERSION_SPLIT[1],
        VERSION_PATCH=VERSION_SPLIT[2])

    core = bld.objects(
        target='core-objects',
        features='pch',
        source=bld.path.ant_glob('core/**/*.cpp', excl=['core/logger*.cpp']),
        use='version.hpp NDN_CXX BOOST LIBRT',
        includes='.',
        export_includes='.',
        headers='core/common.hpp')

    if bld.env.HAVE_CUSTOM_LOGGER:
        core.use += ' CUSTOM_LOGGER'
    else:
        core.source += bld.path.ant_glob('core/logger*.cpp')

    nfd_objects = bld.objects(
        target='daemon-objects',
        source=bld.path.ant_glob('daemon/**/*.cpp',
                                 excl=['daemon/face/*ethernet*.cpp',
                                       'daemon/face/pcap*.cpp',
                                       'daemon/face/unix*.cpp',
                                       'daemon/face/websocket*.cpp',
                                       'daemon/main.cpp']),
        use='core-objects',
        includes='daemon',
        export_includes='daemon')

    if bld.env.HAVE_LIBPCAP:
        nfd_objects.source += bld.path.ant_glob('daemon/face/*ethernet*.cpp')
        nfd_objects.source += bld.path.ant_glob('daemon/face/pcap*.cpp')
        nfd_objects.use += ' LIBPCAP'

    if bld.env.HAVE_UNIX_SOCKETS:
        nfd_objects.source += bld.path.ant_glob('daemon/face/unix*.cpp')

    if bld.env.HAVE_WEBSOCKET:
        nfd_objects.source += bld.path.ant_glob('daemon/face/websocket*.cpp')
        nfd_objects.use += ' WEBSOCKET'

    if bld.env.WITH_OTHER_TESTS:
        nfd_objects.source += bld.path.ant_glob('tests/other/fw/*.cpp')

    bld.objects(target='rib-objects',
                source=bld.path.ant_glob('rib/**/*.cpp'),
                use='core-objects')

    bld.program(name='nfd',
                target='bin/nfd',
                source='daemon/main.cpp',
                use='daemon-objects rib-objects')

    bld.recurse('tools')
    bld.recurse('tests')

    bld(features='subst',
        source='nfd.conf.sample.in',
        target='nfd.conf.sample',
        install_path='${SYSCONFDIR}/ndn',
        IF_HAVE_LIBPCAP='' if bld.env.HAVE_LIBPCAP else '; ',
        IF_HAVE_WEBSOCKET='' if bld.env.HAVE_WEBSOCKET else '; ')

    if bld.env.SPHINX_BUILD:
        bld(features='sphinx',
            name='manpages',
            builder='man',
            config='docs/conf.py',
            outdir='docs/manpages',
            source=bld.path.ant_glob('docs/manpages/**/*.rst'),
            install_path='${MANDIR}',
            VERSION=VERSION)
        bld.symlink_as('${MANDIR}/man1/nfdc-channel.1', 'nfdc-face.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-fib.1', 'nfdc-route.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-register.1', 'nfdc-route.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-unregister.1', 'nfdc-route.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-set-strategy.1', 'nfdc-strategy.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-unset-strategy.1', 'nfdc-strategy.1')

    bld.install_files('${SYSCONFDIR}/ndn', 'autoconfig.conf.sample')

def docs(bld):
    from waflib import Options
    Options.commands = ['doxygen', 'sphinx'] + Options.commands

def doxygen(bld):
    version(bld)

    if not bld.env.DOXYGEN:
        bld.fatal('Cannot build documentation ("doxygen" not found in PATH)')

    bld(features='subst',
        name='doxygen.conf',
        source=['docs/doxygen.conf.in',
                'docs/named_data_theme/named_data_footer-with-analytics.html.in'],
        target=['docs/doxygen.conf',
                'docs/named_data_theme/named_data_footer-with-analytics.html'],
        VERSION=VERSION,
        HTML_FOOTER='../build/docs/named_data_theme/named_data_footer-with-analytics.html' \
                        if os.getenv('GOOGLE_ANALYTICS', None) \
                        else '../docs/named_data_theme/named_data_footer.html',
        GOOGLE_ANALYTICS=os.getenv('GOOGLE_ANALYTICS', ''))

    bld(features='doxygen',
        doxyfile='docs/doxygen.conf',
        use='doxygen.conf')

def sphinx(bld):
    version(bld)

    if not bld.env.SPHINX_BUILD:
        bld.fatal('Cannot build documentation ("sphinx-build" not found in PATH)')

    bld(features='sphinx',
        config='docs/conf.py',
        outdir='docs',
        source=bld.path.ant_glob('docs/**/*.rst'),
        VERSION=VERSION)

def version(ctx):
    # don't execute more than once
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = VERSION_BASE.split('.')

    # first, try to get a version string from git
    gotVersionFromGit = False
    try:
        cmd = ['git', 'describe', '--always', '--match', '%s*' % GIT_TAG_PREFIX]
        out = subprocess.check_output(cmd, universal_newlines=True).strip()
        if out:
            gotVersionFromGit = True
            if out.startswith(GIT_TAG_PREFIX):
                Context.g_module.VERSION = out.lstrip(GIT_TAG_PREFIX)
            else:
                # no tags matched
                Context.g_module.VERSION = '%s-commit-%s' % (VERSION_BASE, out)
    except subprocess.CalledProcessError:
        pass

    versionFile = ctx.path.find_node('VERSION')
    if not gotVersionFromGit and versionFile is not None:
        try:
            Context.g_module.VERSION = versionFile.read()
            return
        except EnvironmentError:
            pass

    # version was obtained from git, update VERSION file if necessary
    if versionFile is not None:
        try:
            if versionFile.read() == Context.g_module.VERSION:
                # already up-to-date
                return
        except EnvironmentError as e:
            Logs.warn('%s exists but is not readable (%s)' % (versionFile, e.strerror))
    else:
        versionFile = ctx.path.make_node('VERSION')

    try:
        versionFile.write(Context.g_module.VERSION)
    except EnvironmentError as e:
        Logs.warn('%s is not writable (%s)' % (versionFile, e.strerror))

def dist(ctx):
    version(ctx)

def distcheck(ctx):
    version(ctx)
