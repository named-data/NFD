# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
"""
Copyright (c) 2014-2019,  Regents of the University of California,
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

VERSION = '0.6.6'
APPNAME = 'nfd'
BUGREPORT = 'https://redmine.named-data.net/projects/nfd'
URL = 'https://named-data.net/doc/NFD/'
GIT_TAG_PREFIX = 'NFD-'

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags', 'compiler-features',
              'coverage', 'pch', 'sanitizers', 'boost',
              'dependency-checker', 'unix-socket', 'websocket',
              'doxygen', 'sphinx_build'],
             tooldir=['.waf-tools'])

    nfdopt = opt.add_option_group('NFD Options')

    opt.addUnixOptions(nfdopt)
    opt.addDependencyOptions(nfdopt, 'libresolv')
    opt.addDependencyOptions(nfdopt, 'librt')
    opt.addDependencyOptions(nfdopt, 'libpcap')
    nfdopt.add_option('--without-libpcap', action='store_true', default=False,
                      help='Disable libpcap (Ethernet face support will be disabled)')
    nfdopt.add_option('--without-systemd', action='store_true', default=False,
                      help='Disable systemd integration')
    opt.addWebsocketOptions(nfdopt)

    nfdopt.add_option('--with-tests', action='store_true', default=False,
                      help='Build unit tests')
    nfdopt.add_option('--with-other-tests', action='store_true', default=False,
                      help='Build other tests')

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
               'default-compiler-flags', 'compiler-features',
               'pch', 'boost', 'dependency-checker', 'websocket',
               'doxygen', 'sphinx_build'])

    conf.env.WITH_TESTS = conf.options.with_tests
    conf.env.WITH_OTHER_TESTS = conf.options.with_other_tests

    conf.find_program('bash', var='BASH')

    if 'PKG_CONFIG_PATH' not in os.environ:
        os.environ['PKG_CONFIG_PATH'] = Utils.subst_vars('${LIBDIR}/pkgconfig', conf.env)
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'], uselib_store='NDN_CXX')

    if not conf.options.without_systemd:
        conf.check_cfg(package='libsystemd', args=['--cflags', '--libs'],
                       uselib_store='SYSTEMD', mandatory=False)

    conf.checkDependency(name='librt', lib='rt', mandatory=False)
    conf.checkDependency(name='libresolv', lib='resolv', mandatory=False)

    conf.check_cxx(msg='Checking if privilege drop/elevation is supported', mandatory=False,
                   define_name='HAVE_PRIVILEGE_DROP_AND_ELEVATE', fragment=PRIVILEGE_CHECK_CODE)

    conf.check_cxx(header_name='valgrind/valgrind.h', define_name='HAVE_VALGRIND', mandatory=False)

    boost_libs = ['system', 'program_options', 'filesystem']
    if conf.env.WITH_TESTS or conf.env.WITH_OTHER_TESTS:
        boost_libs.append('unit_test_framework')

    conf.check_boost(lib=boost_libs, mt=True)
    if conf.env.BOOST_VERSION_NUMBER < 105800:
        conf.fatal('Minimum required Boost version is 1.58.0\n'
                   'Please upgrade your distribution or manually install a newer version of Boost'
                   ' (https://redmine.named-data.net/projects/nfd/wiki/Boost_FAQ)')

    conf.load('unix-socket')

    if not conf.options.without_libpcap:
        conf.checkDependency(name='libpcap', lib='pcap',
                             errmsg='not found, but required for Ethernet face support. '
                                    'Specify --without-libpcap to disable Ethernet face support.')

    conf.checkWebsocket()

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')
    conf.load('sanitizers')

    conf.define_cond('WITH_TESTS', conf.env.WITH_TESTS)
    conf.define_cond('WITH_OTHER_TESTS', conf.env.WITH_OTHER_TESTS)
    conf.define('DEFAULT_CONFIG_FILE', '%s/ndn/nfd.conf' % conf.env.SYSCONFDIR)
    # The config header will contain all defines that were added using conf.define()
    # or conf.define_cond().  Everything that was added directly to conf.env.DEFINES
    # will not appear in the config header, but will instead be passed directly to the
    # compiler on the command line.
    conf.write_config_header('core/config.hpp')

def build(bld):
    versionhpp(bld)

    bld(features='subst',
        name='version.cpp',
        source='core/version.cpp.in',
        target='core/version.cpp',
        install_path=None,
        VERSION_STRING=VERSION_BASE,
        VERSION_BUILD=VERSION)

    bld.objects(
        target='core-objects',
        features='pch',
        source=bld.path.find_node('core').ant_glob('*.cpp') + ['core/version.cpp'],
        use='version.cpp version.hpp NDN_CXX BOOST LIBRT',
        includes='.',
        export_includes='.',
        headers='core/common.hpp')

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

    bld.program(name='nfd',
                target='bin/nfd',
                source='daemon/main.cpp',
                use='daemon-objects SYSTEMD')

    bld.recurse('tools')
    bld.recurse('tests')

    bld(features='subst',
        source='nfd.conf.sample.in',
        target='nfd.conf.sample',
        install_path='${SYSCONFDIR}/ndn',
        IF_HAVE_LIBPCAP='' if bld.env.HAVE_LIBPCAP else '; ',
        IF_HAVE_WEBSOCKET='' if bld.env.HAVE_WEBSOCKET else '; ')

    bld.install_files('${SYSCONFDIR}/ndn', 'autoconfig.conf.sample')

    if bld.env.HAVE_SYSTEMD:
        systemd_units = bld.path.ant_glob('systemd/*.in')
        bld(features='subst',
            name='systemd-units',
            source=systemd_units,
            target=[u.change_ext('') for u in systemd_units])

    if bld.env.SPHINX_BUILD:
        bld(features='sphinx',
            name='manpages',
            builder='man',
            config='docs/conf.py',
            outdir='docs/manpages',
            source=bld.path.ant_glob('docs/manpages/*.rst'),
            install_path='${MANDIR}',
            version=VERSION_BASE,
            release=VERSION)
        bld.symlink_as('${MANDIR}/man1/nfdc-channel.1', 'nfdc-face.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-fib.1', 'nfdc-route.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-register.1', 'nfdc-route.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-unregister.1', 'nfdc-route.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-set-strategy.1', 'nfdc-strategy.1')
        bld.symlink_as('${MANDIR}/man1/nfdc-unset-strategy.1', 'nfdc-strategy.1')

def versionhpp(bld):
    version(bld)

    bld(features='subst',
        name='version.hpp',
        source='core/version.hpp.in',
        target='core/version.hpp',
        install_path=None,
        VERSION=int(VERSION_SPLIT[0]) * 1000000 +
                int(VERSION_SPLIT[1]) * 1000 +
                int(VERSION_SPLIT[2]),
        VERSION_MAJOR=VERSION_SPLIT[0],
        VERSION_MINOR=VERSION_SPLIT[1],
        VERSION_PATCH=VERSION_SPLIT[2])

def docs(bld):
    from waflib import Options
    Options.commands = ['doxygen', 'sphinx'] + Options.commands

def doxygen(bld):
    versionhpp(bld)

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
        use='doxygen.conf version.hpp')

def sphinx(bld):
    version(bld)

    if not bld.env.SPHINX_BUILD:
        bld.fatal('Cannot build documentation ("sphinx-build" not found in PATH)')

    bld(features='sphinx',
        config='docs/conf.py',
        outdir='docs',
        source=bld.path.ant_glob('docs/**/*.rst'),
        version=VERSION_BASE,
        release=VERSION)

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
    except (OSError, subprocess.CalledProcessError):
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
