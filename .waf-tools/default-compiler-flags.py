# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Configure, Logs, Utils

def options(opt):
    opt.add_option('--debug', '--with-debug', action='store_true', default=False,
                   help='Compile in debugging mode with minimal optimizations (-O0 or -Og)')

def configure(conf):
    conf.start_msg('Checking C++ compiler version')

    cxx = conf.env.CXX_NAME # generic name of the compiler
    ccver = tuple(int(i) for i in conf.env.CC_VERSION)
    ccverstr = '.'.join(conf.env.CC_VERSION)
    errmsg = ''
    warnmsg = ''
    if cxx == 'gcc':
        if ccver < (4, 8, 2):
            errmsg = ('The version of gcc you are using is too old.\n'
                      'The minimum supported gcc version is 4.8.2.')
        conf.flags = GccFlags()
    elif cxx == 'clang':
        if ccver < (3, 4, 0):
            errmsg = ('The version of clang you are using is too old.\n'
                      'The minimum supported clang version is 3.4.0.')
        conf.flags = ClangFlags()
    else:
        warnmsg = 'Note: %s compiler is unsupported' % cxx
        conf.flags = CompilerFlags()

    if errmsg:
        conf.end_msg(ccverstr, color='RED')
        conf.fatal(errmsg)
    elif warnmsg:
        conf.end_msg(ccverstr, color='YELLOW')
        Logs.warn(warnmsg)
    else:
        conf.end_msg(ccverstr)

    conf.areCustomCxxflagsPresent = (len(conf.env.CXXFLAGS) > 0)

    # General flags are always applied (e.g., selecting C++11 mode)
    generalFlags = conf.flags.getGeneralFlags(conf)
    conf.add_supported_cxxflags(generalFlags['CXXFLAGS'])
    conf.add_supported_linkflags(generalFlags['LINKFLAGS'])
    conf.env.DEFINES += generalFlags['DEFINES']

@Configure.conf
def check_compiler_flags(conf):
    # Debug or optimized CXXFLAGS and LINKFLAGS are applied only if the
    # corresponding environment variables are not set.
    # DEFINES are always applied.
    if conf.options.debug:
        extraFlags = conf.flags.getDebugFlags(conf)
        if conf.areCustomCxxflagsPresent:
            missingFlags = [x for x in extraFlags['CXXFLAGS'] if x not in conf.env.CXXFLAGS]
            if missingFlags:
                Logs.warn('Selected debug mode, but CXXFLAGS is set to a custom value "%s"'
                          % ' '.join(conf.env.CXXFLAGS))
                Logs.warn('Default flags "%s" will not be used' % ' '.join(missingFlags))
    else:
        extraFlags = conf.flags.getOptimizedFlags(conf)

    if not conf.areCustomCxxflagsPresent:
        conf.add_supported_cxxflags(extraFlags['CXXFLAGS'])
        conf.add_supported_linkflags(extraFlags['LINKFLAGS'])

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
    for flags in cxxflags:
        flags = Utils.to_list(flags)
        if self.check_cxx(cxxflags=['-Werror'] + flags, mandatory=False):
            supportedFlags += flags

    self.end_msg(' '.join(supportedFlags))
    self.env.prepend_value('CXXFLAGS', supportedFlags)

@Configure.conf
def add_supported_linkflags(self, linkflags):
    """
    Check which linkflags are supported by compiler and add them to env.LINKFLAGS variable
    """
    if len(linkflags) == 0:
        return

    self.start_msg('Checking supported LINKFLAGS')

    supportedFlags = []
    for flags in linkflags:
        flags = Utils.to_list(flags)
        if self.check_cxx(linkflags=['-Werror'] + flags, mandatory=False):
            supportedFlags += flags

    self.end_msg(' '.join(supportedFlags))
    self.env.prepend_value('LINKFLAGS', supportedFlags)


class CompilerFlags(object):
    def getCompilerVersion(self, conf):
        return tuple(int(i) for i in conf.env.CC_VERSION)

    def getGeneralFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are always needed"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': []}

    def getDebugFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are needed only in debug mode"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': ['_DEBUG']}

    def getOptimizedFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are needed only in optimized mode"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': ['NDEBUG']}

class GccBasicFlags(CompilerFlags):
    """
    This class defines basic flags that work for both gcc and clang compilers
    """
    def getGeneralFlags(self, conf):
        flags = super(GccBasicFlags, self).getGeneralFlags(conf)
        flags['CXXFLAGS'] += ['-std=c++11']
        return flags

    def getDebugFlags(self, conf):
        flags = super(GccBasicFlags, self).getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-O0',
                              '-Og', # gcc >= 4.8, clang >= 4.0
                              '-g3',
                              '-pedantic',
                              '-Wall',
                              '-Wextra',
                              '-Werror',
                              '-Wnon-virtual-dtor',
                              '-Wno-error=deprecated-declarations', # Bug #3795
                              '-Wno-error=maybe-uninitialized', # Bug #1615
                              '-Wno-unused-parameter',
                              ]
        flags['LINKFLAGS'] += ['-fuse-ld=gold', '-Wl,-O1']
        return flags

    def getOptimizedFlags(self, conf):
        flags = super(GccBasicFlags, self).getOptimizedFlags(conf)
        flags['CXXFLAGS'] += ['-O2',
                              '-g',
                              '-pedantic',
                              '-Wall',
                              '-Wextra',
                              '-Wnon-virtual-dtor',
                              '-Wno-unused-parameter',
                              ]
        flags['LINKFLAGS'] += ['-fuse-ld=gold', '-Wl,-O1']
        return flags

class GccFlags(GccBasicFlags):
    def getDebugFlags(self, conf):
        flags = super(GccFlags, self).getDebugFlags(conf)
        if self.getCompilerVersion(conf) < (5, 1, 0):
            flags['CXXFLAGS'] += ['-Wno-missing-field-initializers']
        flags['CXXFLAGS'] += ['-fdiagnostics-color'] # gcc >= 4.9
        return flags

    def getOptimizedFlags(self, conf):
        flags = super(GccFlags, self).getOptimizedFlags(conf)
        if self.getCompilerVersion(conf) < (5, 1, 0):
            flags['CXXFLAGS'] += ['-Wno-missing-field-initializers']
        flags['CXXFLAGS'] += ['-fdiagnostics-color'] # gcc >= 4.9
        return flags

class ClangFlags(GccBasicFlags):
    def getGeneralFlags(self, conf):
        flags = super(ClangFlags, self).getGeneralFlags(conf)
        if Utils.unversioned_sys_platform() == 'darwin' and self.getCompilerVersion(conf) >= (9, 0, 0):
            # Bug #4296
            flags['CXXFLAGS'] += [['-isystem', '/usr/local/include'], # for Homebrew
                                  ['-isystem', '/opt/local/include']] # for MacPorts
        return flags

    def getDebugFlags(self, conf):
        flags = super(ClangFlags, self).getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-fcolor-diagnostics',
                              '-Wextra-semi',
                              '-Wundefined-func-template',
                              '-Wno-error=deprecated-register',
                              '-Wno-error=infinite-recursion', # Bug #3358
                              '-Wno-error=keyword-macro', # Bug #3235
                              '-Wno-error=unneeded-internal-declaration', # Bug #1588
                              '-Wno-unused-local-typedef', # Bugs #2657 and #3209
                              ]
        version = self.getCompilerVersion(conf)
        if version < (3, 9, 0) or (Utils.unversioned_sys_platform() == 'darwin' and version < (8, 1, 0)):
            flags['CXXFLAGS'] += ['-Wno-unknown-pragmas']
        return flags

    def getOptimizedFlags(self, conf):
        flags = super(ClangFlags, self).getOptimizedFlags(conf)
        flags['CXXFLAGS'] += ['-fcolor-diagnostics',
                              '-Wextra-semi',
                              '-Wundefined-func-template',
                              '-Wno-unused-local-typedef', # Bugs #2657 and #3209
                              ]
        version = self.getCompilerVersion(conf)
        if version < (3, 9, 0) or (Utils.unversioned_sys_platform() == 'darwin' and version < (8, 1, 0)):
            flags['CXXFLAGS'] += ['-Wno-unknown-pragmas']
        return flags
