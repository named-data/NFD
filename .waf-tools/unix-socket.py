#!/usr/bin/env python
# encoding: utf-8

BOOST_ASIO_HAS_LOCAL_SOCKETS_CHECK = '''
#include <iostream>
#include <boost/asio.hpp>
int main() {
#ifdef BOOST_ASIO_HAS_LOCAL_SOCKETS
    std::cout << "yes";
#else
    std::cout << "no";
#endif
    return 0;
}
'''

def configure(conf):
    boost_asio_present = conf.check_cxx(msg='Checking if UNIX socket is supported',
                                        fragment=BOOST_ASIO_HAS_LOCAL_SOCKETS_CHECK,
                                        use='BOOST NDN_CPP RT',
                                        execute=True, define_ret=True)
    if boost_asio_present == "yes":
        conf.define('HAVE_UNIX_SOCKETS', 1)
        conf.env['HAVE_UNIX_SOCKETS'] = True
