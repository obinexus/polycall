#!/usr/bin/env python3
"""
PyPolyCall Basic Usage Example
Demonstrates how to use PyPolyCall binding
"""

import sys
import os

# Add parent directory to path to import pypolycall
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from client.test_client_api import main

if __name__ == "__main__":
    print("🚀 Running PyPolyCall basic usage example...")
    main()
