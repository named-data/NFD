#!/usr/bin/env python2.7

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from SocketServer import ThreadingMixIn
import sys
import commands
import StringIO
import urlparse
import logging
import cgi
import argparse


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

    def do_HEAD(self):
        self.send_response(405)
        self.send_header("Content-type", "text/plain")
        self.end_headers()

    def do_POST(self):
        self.send_response(405)
        self.send_header("Content-type", "text/plain")
        self.end_headers()

    def log_message(self, format, *args):
        if self.server.verbose:
            logging.info("%s - [%s] %s\n" % (self.address_string(),
                         self.log_date_time_string(), format % args))

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


class ThreadHTTPServer(ThreadingMixIn, HTTPServer):
    """ Handle requests using threads"""
    def __init__(self, server, handler, verbose=False, robots=False):
        try:
            HTTPServer.__init__(self, server, handler)
        except Exception as e:
            logging.error(str(e))
            sys.exit(2)
        self.verbose = verbose
        self.robots = robots


# main function to start
def httpServer():
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", type=int, metavar="port number",
                        help="Specify the HTTP server port number, default is 8080.",
                        dest="port", default=8080)
    # if address is not specified, using empty str
    # so it can bind to local address
    parser.add_argument("-a", default="localhost", metavar="IP address", dest="addr",
                        help="Specify the HTTP server IP address.")
    parser.add_argument("-r", default=False, dest="robots", action="store_true",
                        help="Enable HTTP robots to crawl; disabled by default.")
    parser.add_argument("-v", default=False, dest="verbose", action="store_true",
                        help="Verbose mode.")

    args = vars(parser.parse_args())
    localPort = args["port"]
    localAddr = args["addr"]
    verbose = args["verbose"]
    robots = args["robots"]

    # setting log message format
    logging.basicConfig(format='%(asctime)s [%(levelname)s] %(message)s',
                        level=logging.INFO)

    # if port is not valid, exit
    if localPort <= 0 or localPort > 65535:
        logging.error("Specified port number is invalid")
        sys.exit(2)

    httpd = ThreadHTTPServer((localAddr, localPort), StatusHandler,
                             verbose, robots)

    logging.info("Server Starts - at http://%s:%s" % httpd.server_address)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass

    logging.info("Server stopping ...")
    httpd.server_close()

    logging.info("Server Stopped - at http://%s:%s" % httpd.server_address)


if __name__ == '__main__':
    httpServer()
