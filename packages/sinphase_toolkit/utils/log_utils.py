"""Logging Utilities"""
import logging

def setup_logging(level: str = "INFO") -> None:
    """Setup logging configuration"""
    logging.basicConfig(level=getattr(logging, level.upper()))
