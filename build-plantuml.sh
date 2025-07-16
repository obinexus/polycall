#!/bin/bash
# PolyBuild PlantUML Integration Script for OBINexus
# Handles PlantUML compilation and Markdown integration with Pandoc
# Part of the OBINexus build orchestration stack: nlink → polybuild
set -euo pipefail

# Configuration
PROJECT_ROOT="$(pwd)"
PLANTUML_JAR="${PROJECT_ROOT}/tools/plantuml.jar"
PLANTUML_URL="https://github.com/plantuml/plantuml/releases/download/v1.2024.0/plantuml-1.2024.0.jar"
DOCS_DIR="${PROJECT_ROOT}/docs"
BUILD_DIR="${PROJECT_ROOT}/build"
ASSETS_DIR="${DOCS_DIR}/assets"
FILTER_SCRIPT="${PROJECT_ROOT}/tools/plantuml-filter.py"
TOPOLOGY_OPTION="${TOPOLOGY:-mesh}"  # Default to mesh topology if not specified

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Logo
display_logo() {
    echo -e "${CYAN}"
    cat << "LOGO"
 ____       _       ____        _ _     _
|  _ \ ___ | |_   _| __ ) _   _(_) | __| |
| |_) / _ \| | | | |  _ \| | | | | |/ _` |
|  __/ (_) | | |_| | |_) | |_| | | | (_| |
|_|   \___/|_|\__, |____/ \__,_|_|_|\__,_|
              |___/
LOGO
    echo -e "${NC}"
    echo -e "${YELLOW}OBINexus Build Orchestration System${NC}"
    echo -e "${BLUE}PlantUML Documentation Generator${NC}\n"
    echo -e "${GREEN}Using ${TOPOLOGY_OPTION} topology for build coordination${NC}"
}

# Create directories
create_directories() {
    echo -e "${BLUE}Creating project directories...${NC}"
    mkdir -p "${DOCS_DIR}" "${BUILD_DIR}" "${ASSETS_DIR}" "${PROJECT_ROOT}/tools"
    echo -e "${GREEN}Directories created successfully${NC}"
}

# Download PlantUML if not exists
download_plantuml() {
    if [[ ! -f "${PLANTUML_JAR}" ]]; then
        echo -e "${YELLOW}Downloading PlantUML...${NC}"
        curl -L "${PLANTUML_URL}" -o "${PLANTUML_JAR}"
        echo -e "${GREEN}PlantUML downloaded successfully${NC}"
    else
        echo -e "${GREEN}PlantUML jar already exists at ${PLANTUML_JAR}${NC}"
    fi
}

# Create Pandoc filter for PlantUML
create_pandoc_filter() {
    if [[ ! -f "${FILTER_SCRIPT}" ]]; then
        echo -e "${YELLOW}Creating PlantUML filter for Pandoc...${NC}"
        cat > "${FILTER_SCRIPT}" << 'EOF'
#!/usr/bin/env python3
"""
Pandoc filter to process code blocks with class "plantuml" into
plant-generated images.
Needs plantuml.jar from http://plantuml.com/
"""

import os
import sys
import subprocess
from pandocfilters import toJSONFilter, Para, Image, Str, get_filename4code
from pandocfilters import get_caption, get_extension, get_value

def get_plantuml_path():
    """Find plantuml.jar in common locations."""
    script_dir = os.path.dirname(os.path.realpath(__file__))
    project_root = os.path.dirname(script_dir)
    
    # Search in common locations
    potential_paths = [
        os.path.join(script_dir, "plantuml.jar"),
        os.path.join(project_root, "tools", "plantuml.jar"),
        os.path.join(project_root, "plantuml.jar"),
        "/usr/share/plantuml/plantuml.jar",
        "/usr/local/share/plantuml/plantuml.jar"
    ]
    
    for path in potential_paths:
        if os.path.isfile(path):
            return path
    
    # Default fallback
    return os.path.join(project_root, "tools", "plantuml.jar")

def plantuml(key, value, format, meta):
    if key == 'CodeBlock':
        [[ident, classes, keyvals], code] = value

        if "plantuml" in classes:
            caption, typef, keyvals = get_caption(keyvals)
            filename = get_filename4code("plantuml", code)
            filetype = get_extension(format, "png", html="svg", latex="pdf")

            # Directory structure for images
            assets_dir = os.path.join(os.getcwd(), "docs", "assets")
            os.makedirs(assets_dir, exist_ok=True)
            
            src = filename + '.puml'
            dest = os.path.join(assets_dir, filename + '.' + filetype)

            # Write the PlantUML source file
            with open(src, "w") as f:
                f.write(code)

            # Call PlantUML to generate the image
            plantuml_path = get_plantuml_path()
            cmd = ["java", "-jar", plantuml_path, "-t" + filetype, src, "-o", assets_dir]
            
            try:
                subprocess.check_call(cmd)
                sys.stderr.write("Generated image " + dest + "\n")
            except subprocess.CalledProcessError as e:
                sys.stderr.write("Error generating PlantUML image: " + str(e) + "\n")
                return None
            finally:
                if os.path.exists(src):
                    os.remove(src)

            # Return the image
            image_path = "assets/" + filename + "." + filetype
            
            if format == "html":
                alt = Str("PlantUML diagram")
                return Para([Image([ident, [], keyvals], [alt], [image_path, ""])])
            else:
                return Para([Image([ident, [], keyvals], caption, [image_path, typef])])
            
    return None

if __name__ == "__main__":
    toJSONFilter(plantuml)
EOF
        chmod +x "${FILTER_SCRIPT}"
        echo -e "${GREEN}PlantUML filter created successfully${NC}"
    else
        echo -e "${GREEN}PlantUML filter already exists at ${FILTER_SCRIPT}${NC}"
    fi
}

# Generate PlantUML diagrams
generate_diagrams() {
    echo -e "${BLUE}Generating PlantUML diagrams...${NC}"
    
    # Find all .puml files
    find "${PROJECT_ROOT}" -name "*.puml" -o -name "*.plantuml" | while read -r puml_file; do
        echo "Processing: ${puml_file}"
        # Generate SVG (best for web/pandoc)
        java -jar "${PLANTUML_JAR}" -tsvg "${puml_file}" -o "${ASSETS_DIR}"
        # Generate PNG (fallback)
        java -jar "${PLANTUML_JAR}" -tpng "${puml_file}" -o "${ASSETS_DIR}"
        # Generate PDF (for LaTeX/PDF output)
        java -jar "${PLANTUML_JAR}" -tpdf "${puml_file}" -o "${ASSETS_DIR}"
    done
    
    echo -e "${GREEN}PlantUML diagrams generated successfully${NC}"
}

# Create embedded PlantUML in markdown
create_embedded_md() {
    local puml_file="$1"
    local output_md="$2"
    
    echo -e "${BLUE}Creating Markdown with embedded PlantUML...${NC}"
    
    cat > "${output_md}" << 'EOF'
# PolyBuild System Architecture
## System Overview
The PolyBuild fault-tolerant build system implements a distributed architecture with multiple network topologies for maximum resilience and performance.

```plantuml
@startuml PolyBuild_Architecture
!theme vibrant
title PolyBuild Fault-Tolerant Build System Architecture
skinparam backgroundColor #f8f9fa
skinparam defaultFontName "Arial"
skinparam defaultFontSize 10
' Define colors
skinparam rectangle {
    BackgroundColor<<control>> #e1f5fe
    BorderColor<<control>> #0277bd
    BackgroundColor<<build>> #e8f5e8
    BorderColor<<build>> #2e7d32
    BackgroundColor<<fault>> #fff3e0
    BorderColor<<fault>> #f57c00
    BackgroundColor<<optimize>> #f3e5f5
    BorderColor<<optimize>> #7b1fa2
    BackgroundColor<<monitor>> #fff8e1
    BorderColor<<monitor>> #f9a825
    BackgroundColor<<external>> #fafafa
    BorderColor<<external>> #616161
    BackgroundColor<<artifact>> #e0f2f1
    BorderColor<<artifact>> #00695c
}
package "PolyBuild System" {
    package "Control Layer" <<control>> {
        rectangle "Build Orchestrator" as orchestrator {
            + Fault Detection
            + Load Balancing
            + Resource Management
        }
        rectangle "Topology Manager" as topomgr {
            + P2P, Bus, Ring, Star, Mesh
            + Auto-failover
            + Health Monitoring
        }
        rectangle "Cache Manager" as cachemgr {
            + Distributed Cache
            + Artifact Storage
            + Version Control
        }
    }
    package "Build Topologies" <<build>> {
        package "P2P Network" {
            rectangle "Make Node\n(Primary)" as p2p_make
            rectangle "CMake Node\n(Secondary)" as p2p_cmake
            rectangle "Meson Node\n(Tertiary)" as p2p_meson
            p2p_make ..> p2p_cmake : peer
            p2p_cmake ..> p2p_meson : peer
            p2p_meson ..> p2p_make : peer
        }
        package "Bus Topology" {
            rectangle "Bus Coordinator" as bus_coord
            rectangle "Make Worker" as bus_make
            rectangle "CMake Worker" as bus_cmake
            rectangle "Meson Worker" as bus_meson
            rectangle "Script Worker" as bus_script
            bus_coord --> bus_make
            bus_coord --> bus_cmake
            bus_coord --> bus_meson
            bus_coord --> bus_script
        }
        package "Mesh Network" {
            rectangle "Node 1\n(Make+CMake)" as mesh_n1
            rectangle "Node 2\n(Meson+Scripts)" as mesh_n2
            rectangle "Node 3\n(Testing+Packaging)" as mesh_n3
            rectangle "Node 4\n(Cross-compilation)" as mesh_n4
            mesh_n1 ..> mesh_n2
            mesh_n1 ..> mesh_n3
            mesh_n1 ..> mesh_n4
            mesh_n2 ..> mesh_n3
            mesh_n2 ..> mesh_n4
            mesh_n3 ..> mesh_n4
        }
    }
}
@enduml
```

## Key Features

- **Fault Tolerance**: Automatic failover between build systems
- **Topology-Aware Coordination**: Optimized for different network structures
- **Zero-Overhead Architecture**: Minimizes build latency
- **Distributed Caching**: Efficient artifact storage and retrieval
- **NASA-STD-8739.8 Compliance**: Safety-critical certified

## Integration with PolyCall

The PolyBuild system seamlessly integrates with PolyCall through a modular architecture that supports:

1. Dynamic configuration with hot-reload capabilities
2. Edge-first microservice patterns
3. Cross-platform compilation with FreeBSD compatibility

For more details, see the implementation plan and technical specifications.
EOF
    
    echo -e "${GREEN}Markdown with embedded PlantUML created successfully at ${output_md}${NC}"
}

# Create default PlantUML file if none exists
create_default_puml() {
    local puml_file="${PROJECT_ROOT}/docs/diagrams/polybuild-architecture.puml"
    mkdir -p "$(dirname "${puml_file}")"
    
    if [[ ! -f "${puml_file}" ]]; then
        echo -e "${YELLOW}Creating default PlantUML file...${NC}"
        cat > "${puml_file}" << 'EOF'
@startuml PolyBuild_Architecture
!theme vibrant
title PolyBuild Fault-Tolerant Build System Architecture
skinparam backgroundColor #f8f9fa
skinparam defaultFontName "Arial"
skinparam defaultFontSize 10
' Define colors
skinparam rectangle {
    BackgroundColor<<control>> #e1f5fe
    BorderColor<<control>> #0277bd
    BackgroundColor<<build>> #e8f5e8
    BorderColor<<build>> #2e7d32
    BackgroundColor<<fault>> #fff3e0
    BorderColor<<fault>> #f57c00
    BackgroundColor<<optimize>> #f3e5f5
    BorderColor<<optimize>> #7b1fa2
    BackgroundColor<<monitor>> #fff8e1
    BorderColor<<monitor>> #f9a825
    BackgroundColor<<external>> #fafafa
    BorderColor<<external>> #616161
    BackgroundColor<<artifact>> #e0f2f1
    BorderColor<<artifact>> #00695c
}
package "PolyBuild System" {
    package "Control Layer" <<control>> {
        rectangle "Build Orchestrator" as orchestrator {
            + Fault Detection
            + Load Balancing
            + Resource Management
        }
        rectangle "Topology Manager" as topomgr {
            + P2P, Bus, Ring, Star, Mesh
            + Auto-failover
            + Health Monitoring
        }
        rectangle "Cache Manager" as cachemgr {
            + Distributed Cache
            + Artifact Storage
            + Version Control
        }
    }
    package "Build Topologies" <<build>> {
        package "P2P Network" {
            rectangle "Make Node\n(Primary)" as p2p_make
            rectangle "CMake Node\n(Secondary)" as p2p_cmake
            rectangle "Meson Node\n(Tertiary)" as p2p_meson
            p2p_make ..> p2p_cmake : peer
            p2p_cmake ..> p2p_meson : peer
            p2p_meson ..> p2p_make : peer
        }
        package "Bus Topology" {
            rectangle "Bus Coordinator" as bus_coord
            rectangle "Make Worker" as bus_make
            rectangle "CMake Worker" as bus_cmake
            rectangle "Meson Worker" as bus_meson
            rectangle "Script Worker" as bus_script
            bus_coord --> bus_make
            bus_coord --> bus_cmake
            bus_coord --> bus_meson
            bus_coord --> bus_script
        }
        package "Mesh Network" {
            rectangle "Node 1\n(Make+CMake)" as mesh_n1
            rectangle "Node 2\n(Meson+Scripts)" as mesh_n2
            rectangle "Node 3\n(Testing+Packaging)" as mesh_n3
            rectangle "Node 4\n(Cross-compilation)" as mesh_n4
            mesh_n1 ..> mesh_n2
            mesh_n1 ..> mesh_n3
            mesh_n1 ..> mesh_n4
            mesh_n2 ..> mesh_n3
            mesh_n2 ..> mesh_n4
            mesh_n3 ..> mesh_n4
        }
    }
}
@enduml
EOF
        echo -e "${GREEN}Default PlantUML file created at ${puml_file}${NC}"
    fi
}

# Process markdown files with pandoc
process_markdown() {
    echo -e "${BLUE}Processing Markdown files with Pandoc...${NC}"
    
    # Find all markdown files
    find "${DOCS_DIR}" -name "*.md" | while read -r md_file; do
        local basename=$(basename "${md_file}" .md)
        local output_html="${BUILD_DIR}/${basename}.html"
        
        echo "Processing Markdown: ${md_file} -> ${output_html}"
        
        # Process with pandoc and PlantUML filter
        pandoc "${md_file}" \
            --filter "${FILTER_SCRIPT}" \
            --standalone \
            --metadata title="${basename}" \
            --to html5 \
            --output "${output_html}"
    done
    
    echo -e "${GREEN}Markdown files processed successfully${NC}"
}

# Generate CSS file for HTML output
generate_css() {
    local css_file="${ASSETS_DIR}/style.css"
    mkdir -p "$(dirname "${css_file}")"
    
    if [[ ! -f "${css_file}" ]]; then
        echo -e "${YELLOW}Creating CSS file for HTML output...${NC}"
        cat > "${css_file}" << 'EOF'
body {
    margin: 0 auto;
    max-width: 1200px;
    padding: 1em;
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    line-height: 1.5;
    color: #333;
}

h1, h2, h3, h4, h5, h6 {
    color: #0277bd;
    margin-top: 1.5em;
    margin-bottom: 0.5em;
}

h1 {
    border-bottom: 2px solid #0277bd;
    padding-bottom: 0.2em;
}

h2 {
    border-bottom: 1px solid #0277bd;
    padding-bottom: 0.1em;
}

code {
    background-color: #f5f5f5;
    border-radius: 3px;
    padding: 2px 5px;
    font-family: 'Consolas', 'Monaco', monospace;
}

pre {
    background-color: #f5f5f5;
    border-radius: 5px;
    padding: 1em;
    overflow-x: auto;
}

img {
    max-width: 100%;
    height: auto;
    display: block;
    margin: 2em auto;
}

table {
    border-collapse: collapse;
    width: 100%;
    margin: 2em 0;
}

th, td {
    border: 1px solid #ddd;
    padding: 8px;
    text-align: left;
}

th {
    background-color: #f2f2f2;
}

tr:nth-child(even) {
    background-color: #f9f9f9;
}
EOF
        echo -e "${GREEN}CSS file created at ${css_file}${NC}"
    fi
}

# Main execution flow with fault tolerance
main() {
    display_logo
    
    # Try to create directories, with fallback
    create_directories || {
        echo -e "${RED}Failed to create directories, attempting alternative method...${NC}"
        mkdir -p "${DOCS_DIR}" "${BUILD_DIR}" "${ASSETS_DIR}" "${PROJECT_ROOT}/tools"
    }
    
    # Try to download PlantUML, with fallback
    download_plantuml || {
        echo -e "${YELLOW}Failed to download PlantUML from main source, trying alternative...${NC}"
        local alt_url="https://sourceforge.net/projects/plantuml/files/plantuml.jar/download"
        curl -L "${alt_url}" -o "${PLANTUML_JAR}"
    }
    
    # Create necessary files
    create_pandoc_filter
    create_default_puml
    generate_css
    
    # Generate diagrams from PlantUML files
    generate_diagrams || {
        echo -e "${RED}Failed to generate diagrams from existing files. Creating embedded diagram...${NC}"
        mkdir -p "${DOCS_DIR}/diagrams"
        create_embedded_md "${PROJECT_ROOT}/docs/diagrams/polybuild-architecture.puml" "${DOCS_DIR}/polybuild-architecture.md"
    }
    
    # Process markdown files with pandoc
    process_markdown || {
        echo -e "${RED}Failed to process markdown with pandoc. Attempting direct HTML generation...${NC}"
        # Direct HTML generation as fallback
        find "${DOCS_DIR}" -name "*.md" | while read -r md_file; do
            local basename=$(basename "${md_file}" .md)
            local output_html="${BUILD_DIR}/${basename}.html"
            
            echo "Generating HTML directly: ${md_file} -> ${output_html}"
            
            # Basic HTML generation without pandoc
            cat > "${output_html}" << EOF
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>${basename}</title>
    <link rel="stylesheet" href="../docs/assets/style.css">
</head>
<body>
    <h1>${basename}</h1>
    <p>This is a placeholder generated by the build script when pandoc processing failed.</p>
    <p>Please check the markdown source at: ${md_file}</p>
</body>
</html>
EOF
        done
    }
    
    echo -e "${GREEN}PlantUML processing completed successfully!${NC}"
    echo -e "${YELLOW}Results can be found in: ${BUILD_DIR}${NC}"
}

# Execute main function with error handling
main "$@" || {
    echo -e "${RED}Error: Script execution failed${NC}"
    echo -e "${YELLOW}Please check the logs for more information${NC}"
    exit 1
}
