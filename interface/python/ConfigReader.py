"""
A zero-dependency, pure-Python ConfigReader for pFaces configuration files (*.cfg).
Parses hierarchical object blocks and key-value pairs formatted as:
    object_name {
        key = "value";
    }
"""

import os
import re


class ConfigReader:
    """Reader for pFaces configuration files (*.cfg)."""

    def __init__(self, filepath, debug=False):
        self.filepath = filepath
        self.debug = debug
        self.values = {}
        self.load(filepath)

    def load(self, filepath):
        if not os.path.exists(filepath):
            raise FileNotFoundError(f"Config file not found: {filepath}")

        with open(filepath, 'r', encoding='utf-8') as f:
            lines = [line for line in f if not line.strip().startswith('#')]
        content = '\n'.join(lines)

        stack = []
        pattern = re.compile(r'(\b\w+\b)\s*\{|(\})|\b(\w+)\b\s*=\s*\"([^\"]*)\"\s*;')
        for match in pattern.finditer(content):
            block, close_brace, key, val = match.groups()
            if block:
                stack.append(block)
            elif close_brace:
                if stack:
                    stack.pop()
            elif key is not None:
                full_key = '.'.join(stack + [key])
                self.values[full_key] = val
                if self.debug:
                    print(f"[ConfigReader] {full_key} = {val}")

    def get_value_string(self, key):
        """Retrieve a string configuration value by dotted key (e.g. 'system.states.dimension')."""
        return self.values.get(key, "")

    def get_all_keys(self):
        """Return all parsed keys."""
        return list(self.values.keys())

    def print_list(self):
        """Print all key-value pairs."""
        for key, val in self.values.items():
            print(f"{key} = \"{val}\"")
