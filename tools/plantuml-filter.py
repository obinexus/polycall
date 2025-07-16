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
