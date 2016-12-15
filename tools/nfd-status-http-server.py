#!/usr/bin/env python2.7
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

"""
Copyright (c) 2014-2016,  Regents of the University of California,
                          Arizona Board of Regents,
                          Colorado State University,
                          University Pierre & Marie Curie, Sorbonne University,
                          Washington University in St. Louis,
                          Beijing Institute of Technology,
                          The University of Memphis.

This file is part of NFD (Named Data Networking Forwarding Daemon).
See AUTHORS.md for complete list of NFD authors and contributors.

NFD is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
"""

from BaseHTTPServer import HTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
from SocketServer import ThreadingMixIn
import sys
import subprocess
import urlparse
import logging
import argparse
import socket
import os


class StatusHandler(SimpleHTTPRequestHandler):
    """ The handler class to handle requests."""
    def do_GET(self):
        # get the url info to decide how to respond
        parsedPath = urlparse.urlparse(self.path)
        if parsedPath.path == "/":
            # get current nfd status, and use it as result message
            (httpCode, contentType, body) = self.getNfdStatus()
            self.send_response(httpCode)
            self.send_header("Content-Type", contentType)
            self.end_headers()
            self.wfile.write(body)
        elif parsedPath.path == "/robots.txt" and self.server.robots == True:
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
        else:
            SimpleHTTPRequestHandler.do_GET(self)

    def log_message(self, format, *args):
        if self.server.verbose:
            logging.info("%s - %s\n" % (self.address_string(), format % args))

    def makeErrorResponseHtml(self, text):
        return '<!DOCTYPE html><title>NFD status</title><p>%s</p>' % text

    def getNfdStatus(self):
        """ Obtain XML-formatted NFD status report """
        try:
            sp = subprocess.Popen(['nfdc', 'status', 'report', 'xml'], stdout=subprocess.PIPE, close_fds=True)
            output = sp.communicate()[0]
        except OSError as e:
            self.log_message('error invoking nfdc: %s', e)
            html = self.makeErrorResponseHtml('Internal error')
            return (500, "text/html; charset=UTF-8", html)

        if sp.returncode == 0:
            # add stylesheet processing instruction after the XML document type declaration
            pos = output.index('>') + 1
            xml = output[:pos]\
                + '<?xml-stylesheet type="text/xsl" href="nfd-status.xsl"?>'\
                + output[pos:]
            return (200, 'text/xml; charset=UTF-8', xml)
        else:
            html = self.makeErrorResponseHtml('Cannot connect to NFD, Code = %d' % sp.returncode)
            return (504, "text/html; charset=UTF-8", html)

class ThreadHttpServer(ThreadingMixIn, HTTPServer):
    """ Handle requests using threads """
    def __init__(self, server, handler, verbose=False, robots=False):
        serverAddr = server[0]
        # socket.AF_UNSPEC is not supported, check whether it is v6 or v4
        ipType = self.getIpType(serverAddr)
        if ipType == socket.AF_INET6:
            self.address_family = socket.AF_INET6
        elif ipType == socket.AF_INET:
            self.address_family == socket.AF_INET
        else:
            logging.error("The input IP address is neither IPv6 nor IPv4")
            sys.exit(2)

        try:
            HTTPServer.__init__(self, server, handler)
        except Exception as e:
            logging.error(str(e))
            sys.exit(2)
        self.verbose = verbose
        self.robots = robots

    def getIpType(self, ipAddr):
        """ Get ipAddr's address type """
        # if ipAddr is an IPv6 addr, return AF_INET6
        try:
            socket.inet_pton(socket.AF_INET6, ipAddr)
            return socket.AF_INET6
        except socket.error:
            pass
        # if ipAddr is an IPv4 addr return AF_INET, if not, return None
        try:
            socket.inet_pton(socket.AF_INET, ipAddr)
            return socket.AF_INET
        except socket.error:
            return None


# main function to start
def httpServer():
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", type=int, metavar="port number",
                        help="Specify the HTTP server port number, default is 8080.",
                        dest="port", default=8080)
    # if address is not specified, use 127.0.0.1
    parser.add_argument("-a", default="127.0.0.1", metavar="IP address", dest="addr",
                        help="Specify the HTTP server IP address.")
    parser.add_argument("-r", default=False, dest="robots", action="store_true",
                        help="Enable HTTP robots to crawl; disabled by default.")
    parser.add_argument("-f", default="@DATAROOTDIR@/ndn", metavar="Server Directory", dest="serverDir",
                        help="Specify the working directory of nfd-status-http-server, default is @DATAROOTDIR@/ndn.")
    parser.add_argument("-v", default=False, dest="verbose", action="store_true",
                        help="Verbose mode.")
    parser.add_argument("--version", default=False, dest="version", action="store_true",
                        help="Show version and exit")

    args = vars(parser.parse_args())

    if args['version']:
        print "@VERSION@"
        return

    localPort = args["port"]
    localAddr = args["addr"]
    verbose = args["verbose"]
    robots = args["robots"]
    serverDirectory = args["serverDir"]

    os.chdir(serverDirectory)

    # setting log message format
    logging.basicConfig(format='%(asctime)s [%(levelname)s] %(message)s',
                        level=logging.INFO)

    # if port is invalid, exit
    if localPort <= 0 or localPort > 65535:
        logging.error("Specified port number is invalid")
        sys.exit(2)

    httpd = ThreadHttpServer((localAddr, localPort), StatusHandler,
                             verbose, robots)
    httpServerAddr = ""
    if httpd.address_family == socket.AF_INET6:
        httpServerAddr = "http://[%s]:%s" % (httpd.server_address[0],
                                             httpd.server_address[1])
    else:
        httpServerAddr = "http://%s:%s" % (httpd.server_address[0],
                                           httpd.server_address[1])

    logging.info("Server started - at %s" % httpServerAddr)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass

    httpd.server_close()

    logging.info("Server stopped")


if __name__ == '__main__':
    httpServer()
