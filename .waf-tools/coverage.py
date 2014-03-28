# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
#
# Copyright (c) 2014, Regents of the University of California
#
# GPL 3.0 license, see the COPYING.md file for more information

from waflib import TaskGen

def options(opt):
    opt.add_option('--with-coverage', action='store_true', default=False, dest='with_coverage',
                   help='''Set compiler flags for gcc to enable code coverage information''')

def configure(conf):
    if conf.options.with_coverage:
        conf.check_cxx(cxxflags=['-fprofile-arcs', '-ftest-coverage', '-fPIC'],
                       linkflags=['-fprofile-arcs'], uselib_store='GCOV', mandatory=True)

@TaskGen.feature('cxx','cc')
@TaskGen.after('process_source')
def add_coverage(self):
    if getattr(self, 'use', ''):
        self.use += ' GCOV'
    else:
        self.use = 'GCOV'
