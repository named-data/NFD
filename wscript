# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
"""
Copyright (c) 2014-2025,  Regents of the University of California,
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

import os
import subprocess
from waflib import Context, Logs, Utils

VERSION = '24.07'
APPNAME = 'nfd'
GIT_TAG_PREFIX = 'NFD-'

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags', 'pch',
              'coverage', 'sanitizers', 'boost',
              'dependency-checker', 'unix-socket', 'websocket',
              'doxygen', 'sphinx'],
             tooldir=['.waf-tools'])

    optgrp = opt.add_option_group('NFD Options')

    opt.addUnixOptions(optgrp)
    opt.addDependencyOptions(optgrp, 'libresolv')
    opt.addDependencyOptions(optgrp, 'librt')
    opt.addDependencyOptions(optgrp, 'libpcap')
    optgrp.add_option('--without-libpcap', action='store_true', default=False,
                      help='Disable libpcap (Ethernet face support will be disabled)')
    optgrp.add_option('--without-systemd', action='store_true', default=False,
                      help='Disable systemd integration')
    opt.addWebsocketOptions(optgrp)

    optgrp.add_option('--with-tests', action='store_true', default=False,
                      help='Build unit tests')
    optgrp.add_option('--with-other-tests', action='store_true', default=False,
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
               'default-compiler-flags', 'pch',
               'boost', 'dependency-checker', 'websocket',
               'doxygen', 'sphinx'])

    conf.env.WITH_TESTS = conf.options.with_tests
    conf.env.WITH_OTHER_TESTS = conf.options.with_other_tests

    conf.find_program('bash')
    conf.find_program('dot', mandatory=False)

    # Prefer pkgconf if it's installed, because it gives more correct results
    # on Fedora/CentOS/RHEL/etc. See https://bugzilla.redhat.com/show_bug.cgi?id=1953348
    # Store the result in env.PKGCONFIG, which is the variable used inside check_cfg()
    conf.find_program(['pkgconf', 'pkg-config'], var='PKGCONFIG')

    pkg_config_path = os.environ.get('PKG_CONFIG_PATH', f'{conf.env.LIBDIR}/pkgconfig')
    conf.check_cfg(package='libndn-cxx', args=['libndn-cxx >= 0.9.0', '--cflags', '--libs'],
                   uselib_store='NDN_CXX', pkg_config_path=pkg_config_path)

    if not conf.options.without_systemd:
        conf.check_cfg(package='libsystemd', args=['--cflags', '--libs'],
                       uselib_store='SYSTEMD', mandatory=False)

    conf.checkDependency(name='librt', lib='rt', mandatory=False)
    conf.checkDependency(name='libresolv', lib='resolv', mandatory=False)

    conf.check_cxx(msg='Checking if privilege drop/elevation is supported', mandatory=False,
                   define_name='HAVE_PRIVILEGE_DROP_AND_ELEVATE', fragment=PRIVILEGE_CHECK_CODE)

    conf.check_cxx(header_name='valgrind/valgrind.h', define_name='HAVE_VALGRIND', mandatory=False)

    conf.check_boost(lib='program_options', mt=True)
    if conf.env.BOOST_VERSION_NUMBER < 107400:
        conf.fatal('The minimum supported version of Boost is 1.74.0.\n'
                   'Please upgrade your distribution or manually install a newer version of Boost.\n'
                   'For more information, see https://redmine.named-data.net/projects/nfd/wiki/Boost')

    if conf.env.WITH_TESTS or conf.env.WITH_OTHER_TESTS:
        conf.check_boost(lib='unit_test_framework', mt=True, uselib_store='BOOST_TESTS')

    conf.load('unix-socket')

    if not conf.options.without_libpcap:
        conf.checkDependency(name='libpcap', lib='pcap',
                             errmsg='not found, but required for Ethernet face support. '
                                    'Specify --without-libpcap to disable Ethernet face support.')

    # WebSocket++ is incompatible with Boost 1.87.0
    # https://github.com/zaphoyd/websocketpp/issues/1157
    if conf.env.BOOST_VERSION_NUMBER < 108700:
        conf.checkWebsocket()

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')
    conf.load('sanitizers')

    conf.define_cond('WITH_TESTS', conf.env.WITH_TESTS)
    conf.define_cond('WITH_OTHER_TESTS', conf.env.WITH_OTHER_TESTS)
    conf.define('DEFAULT_CONFIG_FILE', f'{conf.env.SYSCONFDIR}/ndn/nfd.conf')
    # The config header will contain all defines that were added using conf.define()
    # or conf.define_cond().  Everything that was added directly to conf.env.DEFINES
    # will not appear in the config header, but will instead be passed directly to the
    # compiler on the command line.
    conf.write_config_header('core/config.hpp', define_prefix='NFD_')

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
        source=bld.path.find_dir('core').ant_glob('*.cpp') + ['core/version.cpp'],
        use='version.cpp version.hpp BOOST NDN_CXX LIBRT',
        includes='.',
        export_includes='.')

    nfd_objects = bld.objects(
        target='daemon-objects',
        source=bld.path.ant_glob('daemon/**/*.cpp',
                                 excl=['daemon/face/*ethernet*.cpp',
                                       'daemon/face/pcap*.cpp',
                                       'daemon/face/unix*.cpp',
                                       'daemon/face/websocket*.cpp',
                                       'daemon/main.cpp']),
        features='pch',
        headers='daemon/nfd-pch.hpp',
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

    bld.recurse('tests')
    bld.recurse('tools')

    # Install sample configs
    bld(features='subst',
        source='nfd.conf.sample.in',
        target='nfd.conf.sample',
        install_path='${SYSCONFDIR}/ndn',
        IF_HAVE_LIBPCAP='' if bld.env.HAVE_LIBPCAP else '; ',
        IF_HAVE_WEBSOCKET='' if bld.env.HAVE_WEBSOCKET else '; ',
        UNIX_SOCKET_PATH='/run/nfd/nfd.sock' if Utils.unversioned_sys_platform() == 'linux' else '/var/run/nfd/nfd.sock')
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

    vmajor = int(VERSION_SPLIT[0])
    vminor = int(VERSION_SPLIT[1]) if len(VERSION_SPLIT) >= 2 else 0
    vpatch = int(VERSION_SPLIT[2]) if len(VERSION_SPLIT) >= 3 else 0

    bld(features='subst',
        name='version.hpp',
        source='core/version.hpp.in',
        target='core/version.hpp',
        install_path=None,
        VERSION=vmajor * 1000000 + vminor * 1000 + vpatch,
        VERSION_MAJOR=str(vmajor),
        VERSION_MINOR=str(vminor),
        VERSION_PATCH=str(vpatch))

def docs(bld):
    from waflib import Options
    Options.commands = ['doxygen', 'sphinx'] + Options.commands

def doxygen(bld):
    versionhpp(bld)

    if not bld.env.DOXYGEN:
        bld.fatal('Cannot build documentation ("doxygen" not found in PATH)')

    bld(features='subst',
        name='doxyfile',
        source='docs/Doxyfile.in',
        target='docs/Doxyfile',
        HAVE_DOT='YES' if bld.env.DOT else 'NO',
        VERSION=VERSION)

    bld(features='doxygen',
        doxyfile='docs/Doxyfile',
        use='doxyfile version.hpp')

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
    version_from_git = ''
    try:
        cmd = ['git', 'describe', '--abbrev=8', '--always', '--match', f'{GIT_TAG_PREFIX}*']
        version_from_git = subprocess.run(cmd, capture_output=True, check=True, text=True).stdout.strip()
        if version_from_git:
            if GIT_TAG_PREFIX and version_from_git.startswith(GIT_TAG_PREFIX):
                Context.g_module.VERSION = version_from_git[len(GIT_TAG_PREFIX):]
            elif not GIT_TAG_PREFIX and ('.' in version_from_git or '-' in version_from_git):
                Context.g_module.VERSION = version_from_git
            else:
                # no tags matched (or we are in a shallow clone)
                Context.g_module.VERSION = f'{VERSION_BASE}+git.{version_from_git}'
    except (OSError, subprocess.SubprocessError):
        pass

    # fallback to the VERSION.info file, if it exists and is not empty
    version_from_file = ''
    version_file = ctx.path.find_node('VERSION.info')
    if version_file is not None:
        try:
            version_from_file = version_file.read().strip()
        except OSError as e:
            Logs.warn(f'{e.filename} exists but is not readable ({e.strerror})')
    if version_from_file and not version_from_git:
        Context.g_module.VERSION = version_from_file
        return

    # update VERSION.info if necessary
    if version_from_file == Context.g_module.VERSION:
        # already up-to-date
        return
    if version_file is None:
        version_file = ctx.path.make_node('VERSION.info')
    try:
        version_file.write(Context.g_module.VERSION)
    except OSError as e:
        Logs.warn(f'{e.filename} is not writable ({e.strerror})')

def dist(ctx):
    ctx.algo = 'tar.xz'
    version(ctx)

def distcheck(ctx):
    ctx.algo = 'tar.xz'
    version(ctx)
