# Copyright (c) 2014-2019, Regents of the University of California
#
# GPL 3.0 license, see the COPYING.md file for more information

from waflib import Options

BOOST_ASIO_HAS_LOCAL_SOCKETS_CHECK = '''
#include <boost/asio/local/basic_endpoint.hpp>
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

    conf.env.HAVE_UNIX_SOCKETS = conf.options.force_unix_socket or boost_asio_has_local_sockets()
    conf.define_cond('HAVE_UNIX_SOCKETS', conf.env.HAVE_UNIX_SOCKETS)
