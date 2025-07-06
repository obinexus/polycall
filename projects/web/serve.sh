#!/bin/bash
echo "Starting PolyCall demo server on http://localhost:8080"
python3 -m http.server 8080 --directory . 2>/dev/null || python -m SimpleHTTPServer 8080
