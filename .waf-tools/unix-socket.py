#!/usr/bin/env python
# encoding: utf-8

BOOST_ASIO_HAS_LOCAL_SOCKETS_CHECK = '''
#include <iostream>
#include <boost/asio.hpp>
int main() {
#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    return 0;
#else
#error "Unix sockets are not available on this platform"
#endif
    return 0;
}
'''

def options(opt):
    opt.add_option('--force-unix-socket', action='store_true', default=False,
                   dest='force_unix_socket',help='''Forcefully enable UNIX sockets support''')

def configure(conf):
    def boost_asio_has_local_sockets():
        return conf.check_cxx(msg='Checking if UNIX sockets are supported',
                              fragment=BOOST_ASIO_HAS_LOCAL_SOCKETS_CHECK,
                              use='BOOST NDN_CPP RT', mandatory=False)

    if conf.options.force_unix_socket or boost_asio_has_local_sockets():
        conf.define('HAVE_UNIX_SOCKETS', 1)
        conf.env['HAVE_UNIX_SOCKETS'] = True
