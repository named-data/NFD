# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# http://www.sphinx-doc.org/en/master/config

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = u'Named Data Networking Forwarding Daemon (NFD)'
copyright = u'Copyright © 2014-2019 Named Data Networking Project.'
author = u'Named Data Networking Project'

# The short X.Y version
#version = ''

# The full version, including alpha/beta/rc tags
#release = ''

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
#today = ''
# Else, today_fmt is used as the format for a strftime call.
today_fmt = '%Y-%m-%d'


# -- General configuration ---------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
needs_sphinx = '1.1'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.extlinks',
    'sphinx.ext.todo',
]

def addExtensionIfExists(extension):
    try:
        __import__(extension)
        extensions.append(extension)
    except ImportError:
        sys.stderr.write("Extension '%s' not found. "
                         "Some documentation may not build correctly.\n" % extension)

if sys.version_info[0] >= 3:
    addExtensionIfExists('sphinxcontrib.doxylink')

# sphinxcontrib.googleanalytics is currently not working with the latest version of Sphinx
# if os.getenv('GOOGLE_ANALYTICS', None):
#     addExtensionIfExists('sphinxcontrib.googleanalytics')

# The master toctree document.
master_doc = 'index'

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'named_data_theme'

# Add any paths that contain custom themes here, relative to this directory.
html_theme_path = ['.']

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']


# -- Options for LaTeX output ------------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    #
    # 'papersize': 'letterpaper',

    # The font size ('10pt', '11pt' or '12pt').
    #
    # 'pointsize': '10pt',

    # Additional stuff for the LaTeX preamble.
    #
    # 'preamble': '',

    # Latex figure (float) alignment
    #
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    ('index', 'nfd-docs.tex', u'Named Data Networking Forwarding Daemon (NFD)',
     author, 'manual'),
]


# -- Options for manual page output ------------------------------------------

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

# If true, show URL addresses after external links.
#man_show_urls = True


# -- Custom options ----------------------------------------------------------

doxylink = {
    'nfd': ('NFD.tag', 'doxygen/'),
}

extlinks = {
    'issue': ('https://redmine.named-data.net/issues/%s', 'issue #'),
}

if os.getenv('GOOGLE_ANALYTICS', None):
    googleanalytics_id = os.environ['GOOGLE_ANALYTICS']
    googleanalytics_enabled = True
