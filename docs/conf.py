# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Queryosity'
copyright = '2024, Tae Hyoun Park'
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

# Doxygen
subprocess.call('doxygen Doxyfile', shell=True)

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
  'breathe'
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
    "tree_sitter_cpp",
    "rich"
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

highlight_language = 'c++'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_title = "Queryosity"
html_theme = "pydata_sphinx_theme"
html_static_path = ['_static']
html_context = {
    # "github_url": "https://github.com", # or your GitHub Enterprise site
    "github_user": "taehyounpark",
    "github_repo": "queryosity",
    "github_version": "docs",
    "doc_path": "docs/",
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
            "url": "https://github.com/taehyounpark/queryosity",  # required
            # Icon class (if "type": "fontawesome"), or path to local image (if "type": "local")
            "icon": "fa-brands fa-square-github",
            # The type of image to be used (see below for details)
            "type": "fontawesome",
        }
   ],
  "external_links": [
      {"name": "How to contribute", "url": "https://github.com/taehyounpark/queryosity/blob/master/CONTRIBUTING.md"},
  ]
#    "pygment_light_style": "solarized-light",
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
myst_substitutions = {
  "Callable": "[*Callable*](https://en.cppreference.com/w/cpp/named_req/Callable)",
  "DefaultConstructible": "[*DefaultConstructible*](https://en.cppreference.com/w/cpp/named_req/DefaultConstructible)",
  "CopyConstructible": "[*CopyConstructible*](https://en.cppreference.com/w/cpp/named_req/CopyConstructible)",
  "MoveConstructible": "[*MoveConstructible*](https://en.cppreference.com/w/cpp/named_req/MoveConstructible)",
  "CopyAssignable": "[*CopyAssignable*](https://en.cppreference.com/w/cpp/named_req/CopyAssignable)",
  "MoveAssignable": "[*MoveAssignable*](https://en.cppreference.com/w/cpp/named_req/MoveAssignable)"
                      }

# -- Breathe configuration -------------------------------------------------

import os, subprocess

docs_dir = os.path.dirname(__file__)

# Run Doxygen with the Doxyfile located in the parent docs dir
doxyfile_path = os.path.join(docs_dir, "Doxyfile")
subprocess.call(["doxygen", doxyfile_path])

# Point Breathe to the actual doxygen XML output directory
doxygen_xml_dir = os.path.join(docs_dir, "_build", "xml")
breathe_projects = {"queryosity": doxygen_xml_dir}
breathe_default_project = "queryosity"
breathe_default_members = ('members',)