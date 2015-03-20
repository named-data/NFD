# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Logs, Configure, Utils

def options(opt):
    opt.add_option('--debug', '--with-debug', action='store_true', default=False, dest='debug',
                   help='''Compile in debugging mode without optimizations (-O0 or -Og)''')

def configure(conf):
    cxx = conf.env['CXX_NAME'] # CXX_NAME represents generic name of the compiler
    if cxx == 'gcc':
        flags = GccFlags()
    elif cxx == 'clang':
        flags = ClangFlags()
    else:
        flags = CompilerFlags()
        Logs.warn('The code has not been yet tested with %s compiler' % cxx)

    areCustomCxxflagsPresent = (len(conf.env.CXXFLAGS) > 0)

    # General flags will alway be applied (e.g., selecting C++11 mode)
    generalFlags = flags.getGeneralFlags(conf)
    conf.add_supported_cxxflags(generalFlags['CXXFLAGS'])
    conf.add_supported_linkflags(generalFlags['LINKFLAGS'])
    conf.env.DEFINES += generalFlags['DEFINES']

    # Debug or optimization CXXFLAGS and LINKFLAGS  will be applied only if the
    # corresponding environment variables are not set.
    # DEFINES will be always applied
    if conf.options.debug:
        extraFlags = flags.getDebugFlags(conf)

        if areCustomCxxflagsPresent:
            missingFlags = [x for x in extraFlags['CXXFLAGS'] if x not in conf.env.CXXFLAGS]
            if len(missingFlags) > 0:
                Logs.warn("Selected debug mode, but CXXFLAGS is set to a custom value '%s'"
                          % " ".join(conf.env.CXXFLAGS))
                Logs.warn("Default flags '%s' are not activated" % " ".join(missingFlags))
    else:
        extraFlags = flags.getOptimizedFlags(conf)

    if not areCustomCxxflagsPresent:
        conf.add_supported_cxxflags(extraFlags['CXXFLAGS'])
        conf.add_supported_cxxflags(extraFlags['LINKFLAGS'])

    conf.env.DEFINES += extraFlags['DEFINES']

@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by compiler and add them to env.CXXFLAGS variable
    """
    if len(cxxflags) == 0:
        return

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
    if len(linkflags) == 0:
        return

    self.start_msg('Checking supported LINKFLAGS')

    supportedFlags = []
    for flag in linkflags:
        if self.check_cxx(linkflags=['-Werror', flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg(' '.join(supportedFlags))
    self.env.LINKFLAGS = supportedFlags + self.env.LINKFLAGS


class CompilerFlags(object):
    def getGeneralFlags(self, conf):
        """Get dict {'CXXFLAGS':[...], LINKFLAGS:[...], DEFINES:[...]} that are always needed"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': []}

    def getDebugFlags(self, conf):
        """Get tuple {CXXFLAGS, LINKFLAGS, DEFINES} that are needed in debug mode"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': ['_DEBUG']}

    def getOptimizedFlags(self, conf):
        """Get tuple {CXXFLAGS, LINKFLAGS, DEFINES} that are needed in optimized mode"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': ['NDEBUG']}

class GccBasicFlags(CompilerFlags):
    """This class defines base flags that work for gcc and clang compiler"""
    def getDebugFlags(self, conf):
        flags = super(GccBasicFlags, self).getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-pedantic', '-Wall',
                              '-O0',
                              '-g3',
                              '-Werror',
                              '-Wno-error=maybe-uninitialized', # Bug #1615
                             ]
        return flags

    def getOptimizedFlags(self, conf):
        flags = super(GccBasicFlags, self).getOptimizedFlags(conf)
        flags['CXXFLAGS'] += ['-pedantic', '-Wall', '-O2', '-g']
        return flags

class GccFlags(GccBasicFlags):
    def getGeneralFlags(self, conf):
        flags = super(GccFlags, self).getGeneralFlags(conf)
        version = tuple(int(i) for i in conf.env['CC_VERSION'])
        if version < (4, 6, 0):
            conf.fatal('The version of gcc you are using (%s) is too old.\n' %
                       '.'.join(conf.env['CC_VERSION']) +
                       'The minimum supported gcc version is 4.6.0.')
        elif version < (4, 7, 0):
            flags['CXXFLAGS'] += ['-std=c++0x']
        else:
            flags['CXXFLAGS'] += ['-std=c++11']
        return flags

    def getDebugFlags(self, conf):
        flags = super(GccFlags, self).getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-Og', # gcc >= 4.8
                              '-fdiagnostics-color', # gcc >= 4.9
                             ]
        return flags

class ClangFlags(GccBasicFlags):
    def getGeneralFlags(self, conf):
        flags = super(ClangFlags, self).getGeneralFlags(conf)
        flags['CXXFLAGS'] += ['-std=c++11',
                              '-Wno-error=unneeded-internal-declaration', # Bug #1588
                              '-Wno-error=deprecated-register',
                              '-Wno-error=unused-local-typedef', # Bug #2657
                              ]
        if Utils.unversioned_sys_platform() == "darwin":
            flags['CXXFLAGS'] += ['-stdlib=libc++']
            flags['LINKFLAGS'] += ['-stdlib=libc++']
        return flags

    def getDebugFlags(self, conf):
        flags = super(ClangFlags, self).getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-fcolor-diagnostics']
        return flags
