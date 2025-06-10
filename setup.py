#!/usr/bin/env python3
"""Sinphasé Governance Framework Setup"""

from setuptools import setup, find_packages

setup(
    name="sinphase-governance",
    version="2.1.0",
    description="Enterprise governance framework",
    packages=find_packages(),
    python_requires=">=3.8",
    entry_points={
        "console_scripts": [
            "sinphase=sinphase_governance.cli.main:main",
        ],
    },
)
