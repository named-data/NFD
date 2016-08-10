# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def options(opt):
    opt.add_option('--with-sanitizer', action='store', default='', dest='sanitizers',
                   help='Comma-separated list of compiler sanitizers to enable [default=none]')

def configure(conf):
    for san in conf.options.sanitizers.split(','):
        if not san:
            continue

        sanflag = '-fsanitize=%s' % san
        conf.start_msg('Checking if compiler supports %s' % sanflag)

        if conf.check_cxx(cxxflags=['-Werror', sanflag, '-fno-omit-frame-pointer'],
                          linkflags=[sanflag], mandatory=False):
            conf.end_msg('yes')
            conf.env.append_unique('CXXFLAGS', [sanflag, '-fno-omit-frame-pointer'])
            conf.env.append_unique('LINKFLAGS', [sanflag])
        else:
            conf.end_msg('no', color='RED')
            conf.fatal('%s sanitizer is not supported by the current compiler' % san)
