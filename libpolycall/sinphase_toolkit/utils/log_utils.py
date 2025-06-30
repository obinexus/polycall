"""Logging Utilities"""
import logging

def setup_logging(level: str = "INFO") -> logging.Logger:
    """Setup logging configuration"""
    logging.basicConfig(level=getattr(logging, level.upper()))
    return logging.getLogger('sinphase')
