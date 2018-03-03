# encoding: utf-8

from waflib import Options, Logs
from waflib.Configure import conf

def addDependencyOptions(self, opt, name, extraHelp=''):
    opt.add_option('--with-%s' % name, metavar='PATH',
                   help='Path to %s, e.g., /usr/local %s' % (name, extraHelp))

setattr(Options.OptionsContext, 'addDependencyOptions', addDependencyOptions)

@conf
def checkDependency(self, name, **kw):
    root = kw.get('path', getattr(Options.options, 'with_%s' % name))
    kw['msg'] = kw.get('msg', 'Checking for %s library' % name)
    kw['uselib_store'] = kw.get('uselib_store', name.upper())
    kw['define_name'] = kw.get('define_name', 'HAVE_%s' % kw['uselib_store'])
    kw['mandatory'] = kw.get('mandatory', True)

    if root:
        isOk = self.check_cxx(includes='%s/include' % root,
                              libpath='%s/lib' % root,
                              **kw)
    else:
        isOk = self.check_cxx(**kw)

    if isOk:
        self.env[kw['define_name']] = True
