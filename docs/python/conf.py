# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'queryosity-py'
copyright = '2025, Tae Hyoun Park'
author = 'Tae Hyoun Park'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))
from sphinx.builders.html import StandaloneHTMLBuilder
import subprocess, os

extensions = [
  "myst_parser",
  'sphinx_design',
  'sphinx.ext.autodoc',
  'sphinx.ext.intersphinx',
  'sphinx.ext.autosectionlabel',
  'sphinx.ext.todo',
  'sphinx.ext.coverage',
  'sphinx.ext.mathjax',
  'sphinx.ext.ifconfig',
  'sphinx.ext.viewcode',
  'sphinx.ext.autodoc',	
  'sphinx.ext.napoleon',
  'sphinx.ext.inheritance_diagram',
  'sphinx_sitemap',
  ]

import sys
from pathlib import Path
sys.path.insert(0, str(Path('..').resolve()))

napoleon_google_docstring = False   # Turn off googledoc strings
napoleon_numpy_docstring = True     # Turn on numpydoc strings
napoleon_use_ivar = True 	     # For maths symbology

autodoc_mock_imports = [
    "ROOT",
    "cppyy",
    "numpy",
    "tree_sitter",
    "tree_sitter_cpp"
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

highlight_language = 'python'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_title = "qtypy"
html_theme = "pydata_sphinx_theme"
# html_static_path = ['_static']
html_context = {
    # "github_url": "https://github.com", # or your GitHub Enterprise site
    "github_user": "taehyounpark",
    "github_repo": "qtypy",
    "github_version": "docs",
    "doc_path": "docs",
    "default_mode": "light"
}
html_theme_options = {
    # "github_url": "https://github.com", # or your GitHub Enterprise site
    "use_edit_page_button": True,
    "icon_links": [
        {
            # Label for this link
            "name": "GitHub",
            # URL where the link will redirect
            "url": "https://github.com/taehyounpark/qtypy",  # required
            # Icon class (if "type": "fontawesome"), or path to local image (if "type": "local")
            "icon": "fa-brands fa-square-github",
            # The type of image to be used (see below for details)
            "type": "fontawesome",
        }
   ],
#    "pygment_dark_style": "solarized-dark"
}
html_show_sourcelink = False

myst_enable_extensions = [
    "amsmath",
    "colon_fence",
    "deflist",
    "dollarmath",
    "fieldlist",
    "html_admonition",
    "html_image",
    "linkify",
    "replacements",
    "smartquotes",
    "strikethrough",
    "substitution",
    "tasklist",
    "attrs_block",
    "attrs_inline"
]

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}