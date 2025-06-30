#!/usr/bin/env python3
"""
Sinphasé Toolkit Setup Configuration
OBINexus Computing - Governance Framework
"""

from setuptools import setup, find_packages

setup(
    name="sinphase-toolkit",
    version="0.1.0",
    description="🔍 Sinphasé Governance Toolkit - OBINexus Computing Unified Framework",
    author="OBINexus Computing",
    author_email="governance@obinexuscomputing.com",
    packages=find_packages(exclude=["tests", "tests.*"]),
    include_package_data=True,
    install_requires=[
        "typer>=0.9.0",
        "rich>=13.0.0",
    ],
    entry_points={
        "console_scripts": [
            "sinphase=sinphase_toolkit.cli:main",
        ],
    },
    python_requires=">=3.8",
    zip_safe=False,
)
