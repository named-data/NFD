# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
#
# Copyright (c) 2014, Regents of the University of California
#
# GPL 3.0 license, see the COPYING.md file for more information

from waflib import Logs, Configure

def options(opt):
    opt.add_option('--debug', '--with-debug', action='store_true', default=False, dest='debug',
                   help='''Compile in debugging mode without optimizations (-O0 or -Og)''')

def configure(conf):
    areCustomCxxflagsPresent = (len(conf.env.CXXFLAGS) > 0)
    defaultFlags = ['-std=c++0x', '-std=c++11',
                    '-stdlib=libc++',   # clang on OSX < 10.9 by default uses gcc's
                                        # libstdc++, which is not C++11 compatible
                    '-pedantic', '-Wall']

    if conf.options.debug:
        conf.define('_DEBUG', 1)
        defaultFlags += ['-O0',
                         '-Og', # gcc >= 4.8
                         '-g3',
                         '-fcolor-diagnostics', # clang
                         '-fdiagnostics-color', # gcc >= 4.9
                         '-Werror',
                         '-Wno-error=maybe-uninitialized', # Bug #1560
                         '-Wno-error=unneeded-internal-declaration', # Bug #1588
                        ]
        if areCustomCxxflagsPresent:
            missingFlags = [x for x in defaultFlags if x not in conf.env.CXXFLAGS]
            if len(missingFlags) > 0:
                Logs.warn("Selected debug mode, but CXXFLAGS is set to a custom value '%s'"
                          % " ".join(conf.env.CXXFLAGS))
                Logs.warn("Default flags '%s' are not activated" % " ".join(missingFlags))
        else:
            conf.add_supported_cxxflags(defaultFlags)
    else:
        defaultFlags += ['-O2', '-g']
        if not areCustomCxxflagsPresent:
            conf.add_supported_cxxflags(defaultFlags)

    # clang on OSX < 10.9 by default uses gcc's libstdc++, which is not C++11 compatible
    conf.add_supported_linkflags(['-stdlib=libc++'])

@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by compiler and add them to env.CXXFLAGS variable
    """
    self.start_msg('Checking supported CXXFLAGS')

    supportedFlags = []
    for flag in cxxflags:
        if self.check_cxx(cxxflags=['-Werror', flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg(' '.join(supportedFlags))
    self.env.CXXFLAGS = supportedFlags + self.env.CXXFLAGS

@Configure.conf
def add_supported_linkflags(self, linkflags):
    """
    Check which linkflags are supported by compiler and add them to env.LINKFLAGS variable
    """
    self.start_msg('Checking supported LINKFLAGS')

    supportedFlags = []
    for flag in linkflags:
        if self.check_cxx(linkflags=['-Werror', flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg(' '.join(supportedFlags))
    self.env.LINKFLAGS = supportedFlags + self.env.LINKFLAGS
