# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Logs, Configure

def configure(conf):
    areCustomCxxflagsPresent = (len(conf.env.CXXFLAGS) > 0)
    if conf.options.debug:
        conf.define('_DEBUG', 1)
        defaultFlags = ['-O0', '-g3',
                        '-Werror',
                        '-Wall',
                        '-fcolor-diagnostics', # only clang supports

                        # # to disable known warnings
                        # '-Wno-unused-variable', # cryptopp
                        # '-Wno-unused-function',
                        # '-Wno-deprecated-declarations',
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
        defaultFlags = ['-O2', '-g', '-Wall',

                        # # to disable known warnings
                        # '-Wno-unused-variable', # cryptopp
                        # '-Wno-unused-function',
                        # '-Wno-deprecated-declarations',
                        ]
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
        if self.check_cxx (cxxflags=[flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg (' '.join (supportedFlags))
    self.env.CXXFLAGS += supportedFlags
