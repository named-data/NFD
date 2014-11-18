# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
#
# Copyright (c) 2014, Regents of the University of California
#
# GPL 3.0 license, see the COPYING.md file for more information

from waflib import Configure

BOOST_ASIO_HAS_KQUEUE_CHECK = '''
#include <boost/asio.hpp>
#if defined(BOOST_ASIO_HAS_KQUEUE) and BOOST_VERSION == 105600
#error "Ethernet face support cannot be enabled on this platform with boost 1.56"
#endif
'''

@Configure.conf
def check_asio_pcap_support(conf):
    # https://svn.boost.org/trac/boost/ticket/10367
    if conf.check_cxx(msg='Checking if Ethernet face support can be enabled',
                      fragment=BOOST_ASIO_HAS_KQUEUE_CHECK,
                      features='cxx', use='BOOST', mandatory=False):
        conf.define('HAVE_ASIO_PCAP_SUPPORT', 1)
        conf.env['HAVE_ASIO_PCAP_SUPPORT'] = True
