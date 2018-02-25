#!/usr/bin/env python3
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

"""
Copyright (c) 2014-2018,  Regents of the University of California,
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

from http.server import HTTPServer, SimpleHTTPRequestHandler
from socketserver import ThreadingMixIn
from urllib.parse import urlsplit
import argparse, ipaddress, os, socket, subprocess


class NfdStatusHandler(SimpleHTTPRequestHandler):
    """ The handler class to handle requests """
    def do_GET(self):
        path = urlsplit(self.path).path
        if path == "/":
            self.__serveReport()
        elif path == "/robots.txt" and self.server.allowRobots:
            self.send_error(404)
        else:
            super().do_GET()

    def __serveReport(self):
        """ Obtain XML-formatted NFD status report and send it back as response body """
        try:
            # enable universal_newlines to get output as string rather than byte sequence
            output = subprocess.check_output(["nfdc", "status", "report", "xml"], universal_newlines=True)
        except OSError as err:
            super().log_message("error invoking nfdc: {}".format(err))
            self.send_error(500)
        except subprocess.CalledProcessError as err:
            super().log_message("error invoking nfdc: command exited with status {}".format(err.returncode))
            self.send_error(504, "Cannot connect to NFD (code {})".format(err.returncode))
        else:
            # add stylesheet processing instruction after the XML document type declaration
            # (yes, this is a ugly hack)
            pos = output.index(">") + 1
            xml = output[:pos]\
                + '<?xml-stylesheet type="text/xsl" href="nfd-status.xsl"?>'\
                + output[pos:]
            self.send_response(200)
            self.send_header("Content-Type", "text/xml; charset=UTF-8")
            self.end_headers()
            self.wfile.write(xml.encode())

    # override
    def log_message(self, *args):
        if self.server.verbose:
            super().log_message(*args)


class ThreadingHttpServer(ThreadingMixIn, HTTPServer):
    """ Handle requests using threads """
    def __init__(self, bindAddr, port, handler, allowRobots=False, verbose=False):
        # socketserver.BaseServer defaults to AF_INET even if you provide an IPv6 address
        # see https://bugs.python.org/issue20215 and https://bugs.python.org/issue24209
        if bindAddr.version == 6:
            self.address_family = socket.AF_INET6
        self.allowRobots = allowRobots
        self.verbose = verbose
        super().__init__((str(bindAddr), port), handler)


def main():
    def ipAddress(arg):
        """ Validate IP address """
        try:
            value = ipaddress.ip_address(arg)
        except ValueError:
            raise argparse.ArgumentTypeError("{!r} is not a valid IP address".format(arg))
        return value

    def portNumber(arg):
        """ Validate port number """
        try:
            value = int(arg)
        except ValueError:
            value = -1
        if value < 0 or value > 65535:
            raise argparse.ArgumentTypeError("{!r} is not a valid port number".format(arg))
        return value

    parser = argparse.ArgumentParser(description="Serves NFD status page via HTTP")
    parser.add_argument("-V", "--version", action="version", version="@VERSION@")
    parser.add_argument("-a", "--address", default="127.0.0.1", type=ipAddress, metavar="ADDR",
                        help="bind to this IP address (default: %(default)s)")
    parser.add_argument("-p", "--port", default=8080, type=portNumber,
                        help="bind to this port number (default: %(default)s)")
    parser.add_argument("-f", "--workdir", default="@DATAROOTDIR@/ndn", metavar="DIR",
                        help="server's working directory (default: %(default)s)")
    parser.add_argument("-r", "--robots", action="store_true",
                        help="allow crawlers and other HTTP bots")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="turn on verbose logging")
    args = parser.parse_args()

    os.chdir(args.workdir)

    httpd = ThreadingHttpServer(args.address, args.port, NfdStatusHandler,
                                allowRobots=args.robots, verbose=args.verbose)

    if httpd.address_family == socket.AF_INET6:
        url = "http://[{}]:{}"
    else:
        url = "http://{}:{}"
    print("Server started at", url.format(*httpd.server_address))

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()


if __name__ == "__main__":
    main()
