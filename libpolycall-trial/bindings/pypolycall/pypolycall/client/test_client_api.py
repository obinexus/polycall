#!/usr/bin/env python3
"""
PyPolyCall Test Client
Tests HTTP communication with LibPolyCall system on port 8084
"""

import json
import http.client
import sys
from datetime import datetime

def log_message(message, level="INFO"):
    """Log message with timestamp"""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] [{level}] {message}")

def test_post():
    """Test POST /books endpoint"""
    log_message("Testing POST /books")
    
    try:
        conn = http.client.HTTPConnection("localhost", 8084)
        headers = {'Content-type': 'application/json'}
        post_data = json.dumps({
            'title': 'Python Test Book', 
            'author': 'PyPolyCall Client'
        })
        
        conn.request('POST', '/books', post_data, headers)
        response = conn.getresponse()
        
        if response.status == 200:
            data = response.read().decode()
            result = json.loads(data)
            log_message(f"✅ Book created successfully: {result}")
            return True
        else:
            log_message(f"❌ POST failed with status {response.status}", "ERROR")
            return False
            
    except ConnectionRefusedError:
        log_message("❌ Connection refused. Is LibPolyCall server running on port 8084?", "ERROR")
        return False
    except Exception as e:
        log_message(f"❌ POST request failed: {str(e)}", "ERROR")
        return False
    finally:
        conn.close()

def test_get():
    """Test GET /books endpoint"""
    log_message("Testing GET /books")
    
    try:
        conn = http.client.HTTPConnection("localhost", 8084)
        conn.request('GET', '/books')
        response = conn.getresponse()
        
        if response.status == 200:
            data = response.read().decode()
            result = json.loads(data)
            log_message(f"✅ Books retrieved successfully: {result}")
            return True
        else:
            log_message(f"❌ GET failed with status {response.status}", "ERROR")
            return False
            
    except ConnectionRefusedError:
        log_message("❌ Connection refused. Is LibPolyCall server running on port 8084?", "ERROR")
        return False
    except Exception as e:
        log_message(f"❌ GET request failed: {str(e)}", "ERROR")
        return False
    finally:
        conn.close()

def verify_polycall_connection():
    """Verify LibPolyCall system is accessible"""
    log_message("🔍 Verifying LibPolyCall connection...")
    
    try:
        conn = http.client.HTTPConnection("localhost", 8084)
        conn.request('HEAD', '/')
        response = conn.getresponse()
        
        if response.status in [200, 404]:  # Server responding
            log_message("✅ LibPolyCall server is responding")
            return True
        else:
            log_message(f"⚠️  Server responded with status {response.status}")
            return False
    except:
        log_message("❌ No LibPolyCall server detected on port 8084", "ERROR")
        return False
    finally:
        conn.close()

def main():
    """Main test execution"""
    log_message("🐍 PyPolyCall Test Client - LibPolyCall Trial")
    log_message("=" * 50)
    
    # Verify connection first
    if not verify_polycall_connection():
        log_message("❌ Cannot connect to LibPolyCall. Please start the server first.")
        log_message("Run: ./bin/polycall -f config.Polycallfile")
        sys.exit(1)
    
    # Run tests
    success_count = 0
    
    if test_post():
        success_count += 1
    
    if test_get():
        success_count += 1
    
    # Results
    log_message("=" * 50)
    if success_count == 2:
        log_message("🎉 All tests passed! PyPolyCall binding is working correctly.")
    else:
        log_message(f"⚠️  {success_count}/2 tests passed. Check LibPolyCall server status.")
        sys.exit(1)

if __name__ == "__main__":
    main()
