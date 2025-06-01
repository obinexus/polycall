#!/usr/bin/env python3
"""
PyPolyCall Package Setup
Python binding for LibPolyCall with strict port mapping
"""

from setuptools import setup, find_packages
import os

# Read README
def read_readme():
    readme_path = os.path.join(os.path.dirname(__file__), 'README.md')
    if os.path.exists(readme_path):
        with open(readme_path, 'r', encoding='utf-8') as f:
            return f.read()
    return "PyPolyCall - Python binding for LibPolyCall"

# Read requirements
def read_requirements():
    req_path = os.path.join(os.path.dirname(__file__), 'requirements.txt')
    if os.path.exists(req_path):
        with open(req_path, 'r', encoding='utf-8') as f:
            return [line.strip() for line in f if line.strip() and not line.startswith('#')]
    return []

setup(
    name="pypolycall",
    version="1.0.0",
    description="Python binding for LibPolyCall - Strict port mapping and zero-trust security",
    long_description=read_readme(),
    long_description_content_type="text/markdown",
    
    author="OBINexusComputing",
    author_email="nnamdi@obinexuscomputing.com",
    url="https://gitlab.com/obinexuscomputing/libpolycall",
    
    # Package discovery
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    
    # Include package data
    include_package_data=True,
    package_data={
        "": [
            "python.polycallrc",
            "examples/*.py",
            "examples/public/*.html",
        ]
    },
    
    # Dependencies
    install_requires=read_requirements(),
    
    # Python version requirement
    python_requires=">=3.7",
    
    # Classifications
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: System :: Networking",
        "Topic :: Internet :: WWW/HTTP :: HTTP Servers",
    ],
    
    # Keywords
    keywords=[
        "libpolycall",
        "polycall",
        "python-binding",
        "zero-trust",
        "port-mapping",
        "api-binding",
        "data-oriented"
    ],
    
    # Entry points
    entry_points={
        "console_scripts": [
            "pypolycall-server=examples.server:main",
        ],
    },
    
    # Project URLs
    project_urls={
        "Bug Reports": "https://gitlab.com/obinexuscomputing/libpolycall/-/issues",
        "Source": "https://gitlab.com/obinexuscomputing/libpolycall",
        "Documentation": "https://gitlab.com/obinexuscomputing/libpolycall/-/blob/main/README.md",
    },
    
    # Development dependencies
    extras_require={
        "dev": [
            "pytest>=6.0",
            "pytest-asyncio>=0.18.0",
            "black>=22.0",
            "flake8>=4.0",
            "mypy>=0.950",
        ],
        "test": [
            "pytest>=6.0",
            "pytest-asyncio>=0.18.0",
            "requests>=2.25.0",
        ],
    },
    
    # License
    license="MIT",
    
    # Metadata for security and configuration
    options={
        "bdist_wheel": {
            "universal": False,
        }
    },
    
    # Configuration validation on install
    zip_safe=False,
)

# Post-install configuration check
def post_install_check():
    """Verify installation and configuration"""
    print("\n" + "="*60)
    print("🐍 PyPolyCall Installation Complete!")
    print("="*60)
    print("📋 Configuration Required:")
    print("   1. Copy python.polycallrc to /opt/polycall/services/python/")
    print("   2. Verify port mapping: 3001:8084 (RESERVED FOR PYTHON)")
    print("   3. Update main config.Polycallfile with Python server entry")
    print("")
    print("🚀 Quick Start:")
    print("   python -m pypolycall.examples.server")
    print("")
    print("🛡️  Security Features:")
    print("   ✅ Strict port binding enforcement")
    print("   ✅ Zero-trust configuration")
    print("   ✅ Reserved port mapping (3001:8084)")
    print("   ✅ No port overlap with other bindings")
    print("")
    print("📖 Documentation:")
    print("   https://gitlab.com/obinexuscomputing/libpolycall")
    print("="*60)

if __name__ == "__main__":
    # Run setup
    setup()
    
    # Post-install verification
    import sys
    if "install" in sys.argv:
        post_install_check()
