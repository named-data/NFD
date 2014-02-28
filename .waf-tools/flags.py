# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Configure

def configure(conf):
    if conf.options.debug:
        conf.define ('_DEBUG', 1)
        flags = ['-O0',
                 '-Wall',
                 '-Wno-unused-variable',
                 '-g3',
                 '-Wno-unused-private-field', # only clang supports
                 '-fcolor-diagnostics',       # only clang supports
                 '-Qunused-arguments',        # only clang supports
                 '-Wno-tautological-compare', # suppress warnings from CryptoPP
                 '-Wno-unused-function',      # another annoying warning from CryptoPP

                 '-Wno-deprecated-declarations',
                 ]

        conf.add_supported_cxxflags (cxxflags = flags)
    else:
        flags = ['-O3', '-g', '-Wno-tautological-compare','-Wno-unused-variable',
                         '-Wno-unused-function', '-Wno-deprecated-declarations']
        conf.add_supported_cxxflags (cxxflags = flags)

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

