#!/usr/bin/env python3
"""
Sinphasé Governance Framework Setup Configuration
"""

from setuptools import setup, find_packages

setup(
    name="sinphase-governance",
    version="2.1.0",
    description="Enterprise-grade governance framework for software architecture compliance",
    author="OBINexus Computing",
    author_email="governance@obinexuscomputing.com",
    packages=find_packages(),
    python_requires=">=3.8",
    entry_points={
        "console_scripts": [
            "sinphase=sinphase_governance.cli.main:main",
        ],
    },
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
    ],
)
