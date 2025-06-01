"""
PyPolyCall Configuration Module
Handles .polycallrc file parsing and configuration management
"""

import os
import configparser
from pathlib import Path

class PolyCallConfig:
    """LibPolyCall configuration handler"""
    
    def __init__(self, config_path=None):
        self.config_path = config_path or self._find_config()
        self.config = configparser.ConfigParser()
        self.load_config()
    
    def _find_config(self):
        """Find .polycallrc file"""
        current_dir = Path.cwd()
        
        # Look in current directory first
        if (current_dir / '.polycallrc').exists():
            return current_dir / '.polycallrc'
        
        # Look in parent directories
        for parent in current_dir.parents:
            config_file = parent / '.polycallrc'
            if config_file.exists():
                return config_file
        
        raise FileNotFoundError("No .polycallrc file found")
    
    def load_config(self):
        """Load configuration from file"""
        if not os.path.exists(self.config_path):
            raise FileNotFoundError(f"Config file not found: {self.config_path}")
        
        # Read raw config (not INI format)
        with open(self.config_path, 'r') as f:
            lines = f.readlines()
        
        self.port_mapping = None
        self.server_type = None
        self.workspace = None
        
        for line in lines:
            line = line.strip()
            if line.startswith('port='):
                self.port_mapping = line.split('=', 1)[1]
            elif line.startswith('server_type='):
                self.server_type = line.split('=', 1)[1]
            elif line.startswith('workspace='):
                self.workspace = line.split('=', 1)[1]
    
    def get_container_port(self):
        """Extract container port from mapping"""
        if self.port_mapping and ':' in self.port_mapping:
            return int(self.port_mapping.split(':')[1])
        return 8084  # Default
    
    def get_host_port(self):
        """Extract host port from mapping"""
        if self.port_mapping and ':' in self.port_mapping:
            return int(self.port_mapping.split(':')[0])
        return 3001  # Default

# Example usage
if __name__ == "__main__":
    config = PolyCallConfig()
    print(f"Container Port: {config.get_container_port()}")
    print(f"Host Port: {config.get_host_port()}")
    print(f"Server Type: {config.server_type}")
