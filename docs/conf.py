# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import importlib.util
import sys

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Named Data Networking Forwarding Daemon (NFD)'
copyright = 'Copyright © 2014-2023 Named Data Networking Project.'
author = 'Named Data Networking Project'

# The short X.Y version.
#version = ''

# The full version, including alpha/beta/rc tags.
#release = ''

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
#today = ''
# Else, today_fmt is used as the format for a strftime call.
today_fmt = '%Y-%m-%d'


# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

needs_sphinx = '4.0'
extensions = [
    'sphinx.ext.extlinks',
    'sphinx.ext.todo',
]

def addExtensionIfExists(extension: str):
    try:
        if importlib.util.find_spec(extension) is None:
            raise ModuleNotFoundError(extension)
    except (ImportError, ValueError):
        sys.stderr.write(f'WARNING: Extension {extension!r} not found. '
                         'Some documentation may not build correctly.\n')
    else:
        extensions.append(extension)

addExtensionIfExists('sphinxcontrib.doxylink')

templates_path = ['_templates']
exclude_patterns = ['Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'named_data_theme'
html_theme_path = ['.']

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_copy_source = False
html_show_sourcelink = False

# Disable syntax highlighting of code blocks by default.
highlight_language = 'none'


# -- Options for manual page output ------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-manual-page-output

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    ('manpages/nfd',            'nfd',              'the Named Data Networking Forwarding Daemon',          [], 1),
    ('manpages/nfdc',           'nfdc',             'interact with NFD management from the command line',   [], 1),
    ('manpages/nfdc-status',    'nfdc-status',      'show NFD status',                                      [], 1),
    ('manpages/nfdc-face',      'nfdc-face',        'show and manipulate NFD\'s faces',                     [], 1),
    ('manpages/nfdc-route',     'nfdc-route',       'show and manipulate NFD\'s routes',                    [], 1),
    ('manpages/nfdc-cs',        'nfdc-cs',          'show and manipulate NFD\'s Content Store',             [], 1),
    ('manpages/nfdc-strategy',  'nfdc-strategy',    'show and manipulate NFD\'s strategy choices',          [], 1),
    ('manpages/nfd-status',     'nfd-status',       'show a comprehensive report of NFD\'s status',         [], 1),
    ('manpages/nfd-status-http-server', 'nfd-status-http-server',   'NFD status HTTP server',               [], 1),
    ('manpages/ndn-autoconfig-server',  'ndn-autoconfig-server',    'auto-configuration server for NDN',    [], 1),
    ('manpages/ndn-autoconfig',         'ndn-autoconfig',           'auto-configuration client for NDN',    [], 1),
    ('manpages/ndn-autoconfig.conf',    'ndn-autoconfig.conf',      'configuration file for ndn-autoconfig',    [], 5),
    ('manpages/nfd-autoreg',            'nfd-autoreg',              'NFD automatic prefix registration daemon', [], 1),
    ('manpages/nfd-asf-strategy',       'nfd-asf-strategy',         'NFD ASF strategy',                     [], 7),
]


# -- Misc options ------------------------------------------------------------

doxylink = {
    'nfd': ('NFD.tag', 'doxygen/'),
}

extlinks = {
    'issue': ('https://redmine.named-data.net/issues/%s', 'issue #%s'),
}
