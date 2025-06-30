"""
LibPolyCall - OBINexus Polyglot Call Framework
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A hybrid C/Python framework for cross-language function calls
with Sinphasé governance enforcement.

:copyright: (c) 2025 OBINexus Team
:license: MIT, see LICENSE for more details.
"""
# Core exports
from .core import PolyCall, CallContext, CallResult
from .exceptions import PolyCallError, ValidationError, GovernanceError
# Lazy loading for optional components
from typing import Any


__version__ = "0.1.0"
__author__ = "OBINexus Team"
__email__ = "engineering@obinexus.com"


__all__ = [
    "PolyCall",
    "CallContext", 
    "CallResult",
    "PolyCallError",
    "ValidationError",
    "GovernanceError",
    "__version__",
]



def __getattr__(name: str) -> Any:
    if name == "ffi":
        from . import ffi
        return ffi
    elif name == "governance":
        from . import governance
        return governance
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")