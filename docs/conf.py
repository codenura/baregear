# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'baregear'
copyright = '2026, First Person'
author = 'First Person'
release = '1.0.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = []

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# Register a lexer for the 'baregear' language so that
# ``.. code-block:: baregear`` blocks build without warnings.
from pygments.lexers.special import TextLexer
from sphinx.highlighting import lexers as sphinx_lexers


class BaregearLexer(TextLexer):
    name = 'baregear'
    aliases = ['baregear']
    filenames = ['*.br']


sphinx_lexers['baregear'] = BaregearLexer()

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'
