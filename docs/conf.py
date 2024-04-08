# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'queryosity'
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
    'sphinx_sitemap',
    'sphinx.ext.inheritance_diagram',
    'breathe'
              ]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

highlight_language = 'c++'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# html_theme = "sphinx_book_theme"
html_theme = "pydata_sphinx_theme"
html_static_path = ['_static']
html_context = {
    # "github_url": "https://github.com", # or your GitHub Enterprise site
    "github_user": "taehyounpark",
    "github_repo": "queryosity",
    "github_version": "master",
    "doc_path": "docs",
}
html_theme_options = {
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
#    "pygment_light_style": "solarized-light",
#    "pygment_dark_style": "solarized-dark"
}

html_title = "Queryosity"

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

breathe_projects = {
	"queryosity": "_build/xml/"
}
breathe_default_project = "queryosity"
breathe_default_members = ('members',)