"""
PyPolyCall - Python Binding for LibPolyCall Trial
Official Python binding for polyglot system capabilities
"""

__version__ = "1.0.0"
__author__ = "OBINexusComputing"
__license__ = "MIT"

from .client import PolyCallClient
from .server import PolyCallServer

__all__ = ['PolyCallClient', 'PolyCallServer']
