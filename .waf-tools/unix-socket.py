# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
#
# Copyright (c) 2014-2018, Regents of the University of California
#
# GPL 3.0 license, see the COPYING.md file for more information

from waflib import Options

BOOST_ASIO_HAS_LOCAL_SOCKETS_CHECK = '''
#include <boost/asio.hpp>
#ifndef BOOST_ASIO_HAS_LOCAL_SOCKETS
#error "Unix sockets are not available on this platform"
#endif
'''

def addUnixOptions(self, opt):
    opt.add_option('--force-unix-socket', action='store_true', default=False,
                   help='Forcefully enable Unix sockets support')

setattr(Options.OptionsContext, 'addUnixOptions', addUnixOptions)

def configure(conf):
    def boost_asio_has_local_sockets():
        return conf.check_cxx(msg='Checking if Unix sockets are supported',
                              fragment=BOOST_ASIO_HAS_LOCAL_SOCKETS_CHECK,
                              features='cxx', use='BOOST', mandatory=False)

    if conf.options.force_unix_socket or boost_asio_has_local_sockets():
        conf.define('HAVE_UNIX_SOCKETS', 1)
        conf.env.HAVE_UNIX_SOCKETS = True
