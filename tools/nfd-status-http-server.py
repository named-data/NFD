#!/usr/bin/env python3
"""
Copyright (c) 2014-2023,  Regents of the University of California,
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

from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlsplit
import argparse, ipaddress, os, socket, subprocess


class NfdStatusHandler(SimpleHTTPRequestHandler):
    """The handler class to handle HTTP requests."""

    def do_GET(self):
        path = urlsplit(self.path).path
        if path == "/":
            self.__serve_report()
        elif path == "/robots.txt" and self.server.allow_robots:
            self.send_error(404)
        else:
            super().do_GET()

    def __serve_report(self):
        """Obtain XML-formatted NFD status report and send it back as response body."""
        try:
            proc = subprocess.run(
                ["nfdc", "status", "report", "xml"], capture_output=True, check=True, text=True, timeout=10
            )
            output = proc.stdout
        except OSError as err:
            super().log_message(f"error invoking nfdc: {err}")
            self.send_error(500)
        except subprocess.SubprocessError as err:
            super().log_message(f"error invoking nfdc: {err}")
            self.send_error(504, "Cannot connect to NFD")
        else:
            # add stylesheet processing instruction after the XML document type declaration
            # (yes, this is a ugly hack)
            if (pos := output.find(">") + 1) != 0:
                xml = (
                    output[:pos]
                    + '<?xml-stylesheet type="text/xsl" href="nfd-status.xsl"?>'
                    + output[pos:]
                )
                self.send_response(200)
                self.send_header("Content-Type", "text/xml; charset=UTF-8")
                self.end_headers()
                self.wfile.write(xml.encode())
            else:
                super().log_message(f"malformed nfdc output: {output}")
                self.send_error(500)

    # override
    def log_message(self, *args):
        if self.server.verbose:
            super().log_message(*args)


class NfdStatusHttpServer(ThreadingHTTPServer):
    def __init__(self, addr, port, *, robots=False, verbose=False):
        # socketserver.BaseServer defaults to AF_INET even if you provide an IPv6 address
        # see https://bugs.python.org/issue20215 and https://bugs.python.org/issue24209
        if addr.version == 6:
            self.address_family = socket.AF_INET6
        self.allow_robots = robots
        self.verbose = verbose
        super().__init__((str(addr), port), NfdStatusHandler)


def main():
    def ip_address(arg, /):
        """Validate IP address."""
        try:
            value = ipaddress.ip_address(arg)
        except ValueError:
            raise argparse.ArgumentTypeError(f"{arg!r} is not a valid IP address")
        return value

    def port_number(arg, /):
        """Validate port number."""
        try:
            value = int(arg)
        except ValueError:
            value = -1
        if value < 0 or value > 65535:
            raise argparse.ArgumentTypeError(f"{arg!r} is not a valid port number")
        return value

    parser = argparse.ArgumentParser(description="Serves NFD status page via HTTP")
    parser.add_argument("-V", "--version", action="version", version="@VERSION@")
    parser.add_argument("-a", "--address", default="127.0.0.1", type=ip_address, metavar="ADDR",
                        help="bind to this IP address (default: %(default)s)")
    parser.add_argument("-p", "--port", default=8080, type=port_number,
                        help="bind to this port number (default: %(default)s)")
    parser.add_argument("-f", "--workdir", default="@DATAROOTDIR@/ndn", metavar="DIR",
                        help="server's working directory (default: %(default)s)")
    parser.add_argument("-r", "--robots", action="store_true",
                        help="allow crawlers and other HTTP bots")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="turn on verbose logging")
    args = parser.parse_args()

    os.chdir(args.workdir)

    with NfdStatusHttpServer(args.address, args.port, robots=args.robots, verbose=args.verbose) as httpd:
        if httpd.address_family == socket.AF_INET6:
            url = "http://[{}]:{}"
        else:
            url = "http://{}:{}"
        print("Server started at", url.format(*httpd.server_address))

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
