# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
#
# Copyright (c) 2014, Regents of the University of California
#
# GPL 3.0 license, see the COPYING.md file for more information

from waflib import Configure

IS_DEFAULT_CONSTRUCTIBLE_CHECK = '''
#include <type_traits>
static_assert(std::is_default_constructible<int>::value, "");
'''

IS_MOVE_CONSTRUCTIBLE_CHECK = '''
#include <type_traits>
static_assert(std::is_move_constructible<int>::value, "");
'''

def configure(conf):
    if conf.check_cxx(msg='Checking for std::is_default_constructible',
                      fragment=IS_DEFAULT_CONSTRUCTIBLE_CHECK,
                      features='cxx', mandatory=False):
        conf.define('HAVE_IS_DEFAULT_CONSTRUCTIBLE', 1)
        conf.env['HAVE_IS_DEFAULT_CONSTRUCTIBLE'] = True

    if conf.check_cxx(msg='Checking for std::is_move_constructible',
                      fragment=IS_MOVE_CONSTRUCTIBLE_CHECK,
                      features='cxx', mandatory=False):
        conf.define('HAVE_IS_MOVE_CONSTRUCTIBLE', 1)
        conf.env['HAVE_IS_MOVE_CONSTRUCTIBLE'] = True
