# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Options, Logs, Errors, Configure
import re

def addWebsocketOptions(self, opt):
    opt.add_option('--without-websocket', action='store_false', default=True,
                   dest='with_websocket', help='Disable WebSocket face support')

setattr(Options.OptionsContext, 'addWebsocketOptions', addWebsocketOptions)

@Configure.conf
def checkWebsocket(self, **kw):
    if not self.options.with_websocket:
        return

    isMandatory = kw.get('mandatory', True)

    self.start_msg('Checking for WebSocket++ includes')

    try:
        websocketDir = self.path.find_dir('websocketpp/websocketpp')
        if not websocketDir:
            raise Errors.WafError('Not found')

        versionFile = websocketDir.find_node('version.hpp')
        if not websocketDir:
            raise Errors.WafError('WebSocket++ version file not found')

        try:
            txt = versionFile.read()
        except (OSError, IOError):
            raise Errors.WafError('Cannot read WebSocket++ version file')

        version = [None, None, None]
        # Looking for the following:
        # static int const major_version = 0;
        # static int const minor_version = 7;
        # static int const patch_version = 0;
        majorVersion = re.compile('^static int const major_version = (\\d+);$', re.M)
        version[0] = majorVersion.search(txt)
        minorVersion = re.compile('^static int const minor_version = (\\d+);$', re.M)
        version[1] = minorVersion.search(txt)
        patchVersion = re.compile('^static int const patch_version = (\\d+);$', re.M)
        version[2] = patchVersion.search(txt)

        if not version[0] or not version[1] or not version[2]:
            raise Errors.WafError('Cannot detect WebSocket++ version')

        self.env.WEBSOCKET_VERSION = [i.group(1) for i in version]

        # todo: version checking, if necessary

        self.end_msg('.'.join(self.env.WEBSOCKET_VERSION))

        self.env.INCLUDES_WEBSOCKET = websocketDir.parent.abspath()
        self.env.HAVE_WEBSOCKET = True
        self.define('HAVE_WEBSOCKET', 1)
        self.define('_WEBSOCKETPP_CPP11_STL_', 1)

    except Errors.WafError as error:
        if isMandatory:
            self.end_msg(str(error), color='RED')
            Logs.warn('If you are using git NFD repository, checkout websocketpp submodule: ')
            Logs.warn('    git submodule init && git submodule update')
            Logs.warn('Otherwise, manually download and extract websocketpp library:')
            Logs.warn('    mkdir websocketpp')
            Logs.warn('    curl -L https://github.com/zaphoyd/websocketpp/archive/0.7.0.tar.gz > websocket.tar.gz')
            Logs.warn('    tar zxf websocket.tar.gz -C websocketpp/ --strip 1')
            Logs.warn('Alternatively, WebSocket support can be disabled with --without-websocket')
            self.fatal('The configuration failed')
        else:
            self.end_msg(str(error))
