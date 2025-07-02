#!/usr/bin/env python3
"""
Sinphasé Toolkit Setup Configuration
OBINexus Computing - Governance Framework

Setup.py for backward compatibility with legacy build systems
Modern packaging uses pyproject.toml as primary configuration
"""

from setuptools import setup, find_packages
from pathlib import Path

# Read README for long description
this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text(encoding='utf-8') if (this_directory / "README.md").exists() else ""

# Version management
VERSION = "0.1.0"

# Core dependencies for runtime
INSTALL_REQUIRES = [
    "typer>=0.9.0",
    "rich>=13.0.0",
]

# Development dependencies
EXTRAS_REQUIRE = {
    "dev": [
        "pytest>=7.0.0",
        "black>=22.0.0",
        "flake8>=4.0.0",
        "mypy>=0.900",
        "coverage>=6.0.0",
    ],
    "ci": [
        "pytest-xdist>=3.0.0",
        "pytest-cov>=4.0.0",
    ],
}

# Package metadata
CLASSIFIERS = [
    "Development Status :: 4 - Beta",
    "Intended Audience :: Developers",
    "License :: OSI Approved :: MIT License",
    "Operating System :: OS Independent",
    "Programming Language :: Python :: 3",
    "Programming Language :: Python :: 3.8",
    "Programming Language :: Python :: 3.9",
    "Programming Language :: Python :: 3.10",
    "Programming Language :: Python :: 3.11",
    "Programming Language :: Python :: 3.12",
    "Topic :: Software Development :: Quality Assurance",
    "Topic :: Software Development :: Libraries :: Python Modules",
]

# Entry points for CLI commands
ENTRY_POINTS = {
    "console_scripts": [
        "sinphase=sinphase_toolkit.cli:main",
    ],
}

if __name__ == "__main__":
    setup(
        name="sinphase-toolkit",
        version=VERSION,
        description="🔍 Sinphasé Governance Toolkit - OBINexus Computing Unified Framework",
        long_description=long_description,
        long_description_content_type="text/markdown",
        author="OBINexus Computing",
        author_email="governance@obinexuscomputing.com",
        url="https://github.com/obinexuscomputing/sinphase-toolkit",
        project_urls={
            "Homepage": "https://github.com/obinexuscomputing/sinphase-toolkit",
            "Repository": "https://github.com/obinexuscomputing/sinphase-toolkit",
            "Documentation": "https://sinphase-toolkit.readthedocs.io",
            "Issues": "https://github.com/obinexuscomputing/sinphase-toolkit/issues",
        },
        packages=find_packages(exclude=["tests", "tests.*"]),
        include_package_data=True,
        package_data={
            "sinphase_toolkit": ["*.txt", "*.md", "*.yml", "*.yaml"],
        },
        install_requires=INSTALL_REQUIRES,
        extras_require=EXTRAS_REQUIRE,
        entry_points=ENTRY_POINTS,
        classifiers=CLASSIFIERS,
        python_requires=">=3.8",
        license="MIT",
        keywords="governance, compliance, code-quality, static-analysis, obinexus",
        zip_safe=False,
        # Enable automatic discovery of namespace packages
        # Important for proper module resolution
        setup_requires=["setuptools>=45"], )
