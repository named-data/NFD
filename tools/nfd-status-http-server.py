#!/usr/bin/env python2.7
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

"""
Copyright (c) 2014  Regents of the University of California,
                    Arizona Board of Regents,
                    Colorado State University,
                    University Pierre & Marie Curie, Sorbonne University,
                    Washington University in St. Louis,
                    Beijing Institute of Technology

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

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from SocketServer import ThreadingMixIn
import sys
import commands
import StringIO
import urlparse
import logging
import cgi
import argparse
import socket


class StatusHandler(BaseHTTPRequestHandler):
    """ The handler class to handle requests."""
    def do_GET(self):
        # get the url info to decide how to respond
        parsedPath = urlparse.urlparse(self.path)
        resultMessage = ""
        if parsedPath.path == "/robots.txt":
            if self.server.robots == False:
                # return User-agent: * Disallow: / to disallow robots
                resultMessage = "User-agent: * \nDisallow: /\n"
            self.send_response(200)
            self.send_header("Content-type", "text/plain")
        elif parsedPath.path == "/":
            # get current nfd status, and use it as result message
            resultMessage = self.getNfdStatus()
            self.send_response(200)
            self.send_header("Content-type", "text/html; charset=UTF-8")
        else:
            # non-existing content
            resultMessage = '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 '\
                   + 'Transitional//EN" '\
                   + '"http://www.w3.org/TR/html4/loose.dtd">\n'\
                   + '<html><head><title>Object not '\
                   + 'found!</title><meta http-equiv="Content-type" ' \
                   + 'content="text/html;charset=UTF-8"></head>\n'\
                   + '<body><h1>Object not found!</h1>'\
                   + '<p>The requested URL was not found on this server.</p>'\
                   + '<h2>Error 404</h2>'\
                   + '</body></html>'
            self.send_response(404)
            self.send_header("Content-type", "text/html; charset=UTF-8")

        self.end_headers()
        self.wfile.write(resultMessage)

    def log_message(self, format, *args):
        if self.server.verbose:
            logging.info("%s - %s\n" % (self.address_string(),
                         format % args))

    def getNfdStatus(self):
        """
        This function tries to call nfd-status command
        to get nfd's current status, after convert it
        to html format, return it to the caller
        """
        (res, output) = commands.getstatusoutput('nfd-status')

        htmlStr = '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional'\
                + '//EN" "http://www.w3.org/TR/html4/loose.dtd">\n'\
                + '<html><head><title>NFD status</title>'\
                + '<meta http-equiv="Content-type" content="text/html;'\
                + 'charset=UTF-8"></head>\n<body>'
        if res == 0:
            # parse the output
            buf = StringIO.StringIO(output)
            firstLine = 0
            for line in buf:
                if line.endswith(":\n"):
                    if firstLine != 0:
                        htmlStr += "</ul>\n"
                    firstLine += 1
                    htmlStr += "<b>" + cgi.escape(line.strip()) + "</b><br>\n"\
                             + "<ul>\n"
                    continue
                line = line.strip()
                htmlStr += "<li>" + cgi.escape(line) + "</li>\n"
            buf.close()
            htmlStr += "</ul>\n"
        else:
            # return connection error code
            htmlStr = htmlStr + "<p>Cannot connect to NFD,"\
                    + " Code = " + str(res) + "</p>\n"
        htmlStr = htmlStr + "</body></html>"
        return htmlStr


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
