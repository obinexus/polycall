#!/bin/bash
# Component creation script

if [ $# -ne 2 ]; then
    echo "Usage: $0 <component_type> <component_name>"
    echo "Types: core, cli, ffi"
    exit 1
fi

COMPONENT_TYPE=$1
COMPONENT_NAME=$2
COMPONENT_DIR="src/${COMPONENT_TYPE}/${COMPONENT_NAME}"

mkdir -p "${COMPONENT_DIR}"

# Create CMakeLists.txt from template
sed "s/@COMPONENT_NAME@/${COMPONENT_NAME}/g" \
    scripts/templates/component_cmake.template > "${COMPONENT_DIR}/CMakeLists.txt"

# Create basic source files
echo "#include \"${COMPONENT_NAME}.h\"" > "${COMPONENT_DIR}/${COMPONENT_NAME}.c"
echo "#ifndef ${COMPONENT_NAME^^}_H" > "${COMPONENT_DIR}/${COMPONENT_NAME}.h"
echo "#define ${COMPONENT_NAME^^}_H" >> "${COMPONENT_DIR}/${COMPONENT_NAME}.h"
echo "#endif" >> "${COMPONENT_DIR}/${COMPONENT_NAME}.h"

echo "Component created: ${COMPONENT_DIR}"
