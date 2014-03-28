# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
#
# Copyright (c) 2014, Regents of the University of California
#
# GPL 3.0 license, see the COPYING.md file for more information

from waflib import Logs, Configure

def options(opt):
    opt.add_option('--debug', '--with-debug', action='store_true', default=False, dest='debug',
                   help='''Compile in debugging mode without all optimizations (-O0)''')

def configure(conf):
    areCustomCxxflagsPresent = (len(conf.env.CXXFLAGS) > 0)
    if conf.options.debug:
        conf.define('_DEBUG', 1)
        defaultFlags = ['-O0', '-g3',
                        '-Werror',
                        '-Wall',
                        '-fcolor-diagnostics', # only clang supports
                        ]

        if areCustomCxxflagsPresent:
            missingFlags = [x for x in defaultFlags if x not in conf.env.CXXFLAGS]
            if len(missingFlags) > 0:
                Logs.warn("Selected debug mode, but CXXFLAGS is set to a custom value '%s'"
                          % " ".join(conf.env.CXXFLAGS))
                Logs.warn("Default flags '%s' are not activated" % " ".join(missingFlags))
        else:
            conf.add_supported_cxxflags(cxxflags=defaultFlags)
    else:
        defaultFlags = ['-O2', '-g', '-Wall']
        if not areCustomCxxflagsPresent:
            conf.add_supported_cxxflags(cxxflags=defaultFlags)

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
    self.env.CXXFLAGS = supportedFlags + self.env.CXXFLAGS
