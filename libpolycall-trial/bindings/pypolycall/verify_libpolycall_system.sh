#!/bin/bash

# LibPolyCall System Verification Script
# Checks if LibPolyCall core system is properly configured for PyPolyCall integration

set -e

echo "🔍 LibPolyCall System Status Verification"
echo "========================================="

# Auto-detect repository root and adjust paths
CURRENT_DIR=$(pwd)
echo "📍 Current directory: $CURRENT_DIR"

# Try to find libpolycall-trial directory by going up the directory tree
REPO_ROOT=""
SEARCH_DIR="$CURRENT_DIR"

for i in {1..5}; do
    if [ -d "$SEARCH_DIR/libpolycall-trial" ]; then
        REPO_ROOT="$SEARCH_DIR"
        break
    elif [ -d "$SEARCH_DIR/../libpolycall-trial" ]; then
        REPO_ROOT="$(dirname "$SEARCH_DIR")"
        break
    elif [ -d "$SEARCH_DIR/../../libpolycall-trial" ]; then
        REPO_ROOT="$(dirname "$(dirname "$SEARCH_DIR")")"
        break
    fi
    SEARCH_DIR="$(dirname "$SEARCH_DIR")"
done

if [ -n "$REPO_ROOT" ]; then
    echo "🔍 Found repository root: $REPO_ROOT"
    LIBPOLYCALL_ROOT="$REPO_ROOT/libpolycall-trial"
else
    echo "⚠️  Could not auto-detect libpolycall-trial directory"
    echo "Looking for relative paths..."
    LIBPOLYCALL_ROOT="./libpolycall-trial"
fi

echo "📁 Using LibPolyCall root: $LIBPOLYCALL_ROOT"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check counters
TOTAL_CHECKS=0
PASSED_CHECKS=0

check_status() {
    local description="$1"
    local status="$2"
    local message="$3"
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    if [ "$status" = "PASS" ]; then
        echo -e "${GREEN}✅ $description${NC}"
        [ -n "$message" ] && echo -e "   ${BLUE}ℹ️  $message${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
    elif [ "$status" = "WARN" ]; then
        echo -e "${YELLOW}⚠️  $description${NC}"
        [ -n "$message" ] && echo -e "   ${YELLOW}⚠️  $message${NC}"
    else
        echo -e "${RED}❌ $description${NC}"
        [ -n "$message" ] && echo -e "   ${RED}❌ $message${NC}"
    fi
}

# Check 1: LibPolyCall directory structure
echo -e "\n${BLUE}📁 Checking LibPolyCall Directory Structure${NC}"
if [ -d "$LIBPOLYCALL_ROOT" ]; then
    check_status "LibPolyCall root directory exists" "PASS" "$LIBPOLYCALL_ROOT found"
    
    if [ -d "$LIBPOLYCALL_ROOT/services/python" ]; then
        check_status "Python service directory exists" "PASS" "$LIBPOLYCALL_ROOT/services/python found"
    else
        check_status "Python service directory exists" "FAIL" "Run: ./setup_pypolycall_libpolycall.sh"
    fi
    
    if [ -f "$LIBPOLYCALL_ROOT/config.Polycallfile" ]; then
        check_status "Main configuration file exists" "PASS" "config.Polycallfile found"
    else
        check_status "Main configuration file exists" "FAIL" "Run: ./setup_pypolycall_libpolycall.sh"
    fi
else
    check_status "LibPolyCall root directory exists" "FAIL" "Run: ./setup_pypolycall_libpolycall.sh"
fi

# Check 2: Python binding configuration
echo -e "\n${BLUE}🐍 Checking PyPolyCall Configuration${NC}"
PYTHON_CONFIG="$LIBPOLYCALL_ROOT/services/python/.polycallrc"
if [ -f "$PYTHON_CONFIG" ]; then
    check_status "Python .polycallrc exists" "PASS" "$PYTHON_CONFIG found"
    
    # Check port mapping
    if grep -q "port=3001:8084" "$PYTHON_CONFIG"; then
        check_status "Port mapping configured correctly" "PASS" "3001:8084 mapping verified"
    else
        check_status "Port mapping configured correctly" "FAIL" "Expected port=3001:8084"
    fi
    
    # Check server type
    if grep -q "server_type=python" "$PYTHON_CONFIG"; then
        check_status "Server type configured correctly" "PASS" "server_type=python verified"
    else
        check_status "Server type configured correctly" "FAIL" "Expected server_type=python"
    fi
    
    # Check zero-trust settings
    if grep -q "strict_port_binding=true" "$PYTHON_CONFIG"; then
        check_status "Zero-trust security enabled" "PASS" "strict_port_binding=true"
    else
        check_status "Zero-trust security enabled" "WARN" "strict_port_binding not found"
    fi
else
    check_status "Python .polycallrc exists" "FAIL" "Run: ./setup_pypolycall_libpolycall.sh"
fi

# Check 3: Main configuration
echo -e "\n${BLUE}⚙️ Checking Main LibPolyCall Configuration${NC}"
MAIN_CONFIG="./libpolycall-trial/config.Polycallfile"
if [ -f "$MAIN_CONFIG" ]; then
    check_status "Main config.Polycallfile exists" "PASS" "$MAIN_CONFIG found"
    
    # Check Python server declaration
    if grep -q "server python 3001:8084" "$MAIN_CONFIG"; then
        check_status "Python server declared in main config" "PASS" "server python 3001:8084 found"
    else
        check_status "Python server declared in main config" "FAIL" "Add: server python 3001:8084"
    fi
    
    # Check network configuration
    if grep -q "network start" "$MAIN_CONFIG"; then
        check_status "Network configuration enabled" "PASS" "network start found"
    else
        check_status "Network configuration enabled" "WARN" "network start not found"
    fi
else
    check_status "Main config.Polycallfile exists" "FAIL" "Run: sudo ./setup_pypolycall_libpolycall.sh"
fi

# Check 4: LibPolyCall executable
echo -e "\n${BLUE}🚀 Checking LibPolyCall Executable${NC}"
if [ -f "./bin/polycall" ]; then
    check_status "LibPolyCall executable exists" "PASS" "./bin/polycall found"
    
    if [ -x "./bin/polycall" ]; then
        check_status "LibPolyCall executable is executable" "PASS" "Execute permissions verified"
    else
        check_status "LibPolyCall executable is executable" "FAIL" "Run: chmod +x ./bin/polycall"
    fi
else
    check_status "LibPolyCall executable exists" "FAIL" "Ensure ./bin/polycall is available"
fi

# Check 5: Network port availability
echo -e "\n${BLUE}🌐 Checking Network Ports${NC}"

# Check if port 8084 is available or in use by LibPolyCall
if command -v netstat >/dev/null 2>&1; then
    PORT_8084_STATUS=$(netstat -tuln 2>/dev/null | grep ":8084 " || echo "")
    if [ -n "$PORT_8084_STATUS" ]; then
        check_status "Port 8084 status" "PASS" "Port 8084 is in use (LibPolyCall may be running)"
    else
        check_status "Port 8084 status" "WARN" "Port 8084 is available (LibPolyCall not running)"
    fi
else
    check_status "Port 8084 status" "WARN" "netstat not available - cannot check port status"
fi

# Check if port 3001 is available
if command -v netstat >/dev/null 2>&1; then
    PORT_3001_STATUS=$(netstat -tuln 2>/dev/null | grep ":3001 " || echo "")
    if [ -n "$PORT_3001_STATUS" ]; then
        check_status "Port 3001 availability" "WARN" "Port 3001 is in use"
    else
        check_status "Port 3001 availability" "PASS" "Port 3001 is available"
    fi
fi

# Check 6: LibPolyCall process status
echo -e "\n${BLUE}📊 Checking LibPolyCall Process Status${NC}"
if pgrep -f "polycall" >/dev/null 2>&1; then
    POLYCALL_PID=$(pgrep -f "polycall")
    check_status "LibPolyCall process running" "PASS" "PID: $POLYCALL_PID"
else
    check_status "LibPolyCall process running" "WARN" "LibPolyCall not running - start with: ./bin/polycall -f ./libpolycall-trial/config.Polycallfile"
fi

# Check 7: PyPolyCall project structure
echo -e "\n${BLUE}🐍 Checking PyPolyCall Project${NC}"
if [ -d "pypolycall" ]; then
    check_status "PyPolyCall project directory exists" "PASS" "./pypolycall found"
    
    if [ -f "pypolycall/client/test_client_api.py" ]; then
        check_status "PyPolyCall test client exists" "PASS" "test_client_api.py found"
    else
        check_status "PyPolyCall test client exists" "FAIL" "Run setup script to create project"
    fi
    
    if [ -x "pypolycall/client/test_client_api.py" ]; then
        check_status "PyPolyCall test client is executable" "PASS" "Execute permissions verified"
    else
        check_status "PyPolyCall test client is executable" "FAIL" "Run: chmod +x pypolycall/client/test_client_api.py"
    fi
else
    check_status "PyPolyCall project directory exists" "FAIL" "Run: ./setup_pypolycall_libpolycall.sh"
fi

# Summary
echo -e "\n${BLUE}📋 Verification Summary${NC}"
echo "======================================"

if [ $PASSED_CHECKS -eq $TOTAL_CHECKS ]; then
    echo -e "${GREEN}🎉 All checks passed! ($PASSED_CHECKS/$TOTAL_CHECKS)${NC}"
    echo -e "${GREEN}✅ LibPolyCall system is ready for PyPolyCall integration${NC}"
    echo ""
    echo -e "${BLUE}🚀 Next Steps:${NC}"
    echo "   1. Start LibPolyCall (if not running):"
    echo "      ./bin/polycall -f ./libpolycall-trial/config.Polycallfile"
    echo ""
    echo "   2. Test PyPolyCall integration:"
    echo "      cd pypolycall"
    echo "      python client/test_client_api.py"
elif [ $PASSED_CHECKS -gt $((TOTAL_CHECKS * 3 / 4)) ]; then
    echo -e "${YELLOW}⚠️  Most checks passed ($PASSED_CHECKS/$TOTAL_CHECKS)${NC}"
    echo -e "${YELLOW}🔧 Fix the failing checks above before proceeding${NC}"
else
    echo -e "${RED}❌ Multiple checks failed ($PASSED_CHECKS/$TOTAL_CHECKS)${NC}"
    echo -e "${RED}🛠️  Run the setup script to configure the system:${NC}"
    echo "      ./setup_pypolycall_libpolycall.sh"
fi

echo ""
echo -e "${BLUE}🛡️  Zero-Trust Security Status:${NC}"
if grep -q "strict_port_binding=true" "./libpolycall-trial/services/python/.polycallrc" 2>/dev/null; then
    echo -e "${GREEN}✅ Strict port binding enforced${NC}"
else
    echo -e "${YELLOW}⚠️  Strict port binding not configured${NC}"
fi

if grep -q "server python 3001:8084" "./libpolycall-trial/config.Polycallfile" 2>/dev/null; then
    echo -e "${GREEN}✅ Python server binding declared${NC}"
else
    echo -e "${RED}❌ Python server binding not declared${NC}"
fi

echo -e "${BLUE}📊 Port Configuration:${NC}"
echo "   • Host Port: 3001 (External access)"
echo "   • Container Port: 8084 (LibPolyCall communication)"
echo "   • Zero-Trust: No fallback ports allowed"
