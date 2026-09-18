#!/usr/bin/env python3
"""
tritos_mcp_client.py — Cliente MCP para integrar octave-mcp con Tritos

Uso:
    from tritos_mcp_client import OctaveMCP
    
    mcp = OctaveMCP()
    result = mcp.call("ternary_arithmetic_tool", {"mode": "add", "a": 1, "b": -1})
    print(result)
"""

import subprocess
import json
import os
import sys
from typing import Any, Dict, Optional

class OctaveMCP:
    """Cliente para octave-mcp server"""
    
    def __init__(self, server_path: str = None):
        if server_path is None:
            # Try common locations
            home = os.path.expanduser("~")
            candidates = [
                os.path.join(home, "octave-mcp", "server.py"),
                os.path.join(home, ".local", "share", "octave-mcp", "server.py"),
                "/usr/local/bin/octave-mcp",
                os.path.join(home, "octave-mcp", "main.py"),
            ]
            for c in candidates:
                if os.path.exists(c):
                    self.server_path = c
                    break
            else:
                self.server_path = os.path.join(home, "octave-mcp", "server.py")
        else:
            self.server_path = server_path
        
        self.request_id = 0
    
    def call(self, tool: str, params: Dict[str, Any] = None) -> Dict[str, Any]:
        """Call an MCP tool and return the result"""
        self.request_id += 1
        
        request = {
            "jsonrpc": "2.0",
            "id": self.request_id,
            "method": "tools/call",
            "params": {
                "name": tool,
                "arguments": params or {}
            }
        }
        
        try:
            result = subprocess.run(
                ["python3", self.server_path],
                input=json.dumps(request) + "\n",
                capture_output=True,
                text=True,
                timeout=30
            )
            
            if result.returncode != 0:
                return {"error": result.stderr or "Unknown error"}
            
            # Parse response (take first line)
            for line in result.stdout.strip().split("\n"):
                if line.strip():
                    try:
                        return json.loads(line)
                    except json.JSONDecodeError:
                        continue
            
            return {"error": "No valid response"}
            
        except subprocess.TimeoutExpired:
            return {"error": "Timeout (30s)"}
        except FileNotFoundError:
            return {"error": f"Server not found: {self.server_path}"}
        except Exception as e:
            return {"error": str(e)}
    
    def list_tools(self) -> list:
        """List available tools"""
        request = {
            "jsonrpc": "2.0",
            "id": 0,
            "method": "tools/list",
            "params": {}
        }
        
        try:
            result = subprocess.run(
                ["python3", self.server_path],
                input=json.dumps(request) + "\n",
                capture_output=True,
                text=True,
                timeout=10
            )
            
            for line in result.stdout.strip().split("\n"):
                if line.strip():
                    try:
                        resp = json.loads(line)
                        return resp.get("result", {}).get("tools", [])
                    except json.JSONDecodeError:
                        continue
            return []
        except Exception:
            return []


# =============================================================================
# TERNARY-SPECIFIC FUNCTIONS
# =============================================================================

class TritosMath:
    """Wrapper para tools ternarias de octave-mcp"""
    
    def __init__(self, server_path: str = None):
        self.mcp = OctaveMCP(server_path)
    
    def ternary_add(self, a: int, b: int) -> Dict:
        """Suma ternaria"""
        return self.mcp.call("ternary_arithmetic_tool", {
            "mode": "add", "a": a, "b": b
        })
    
    def ternary_sub(self, a: int, b: int) -> Dict:
        """Resta ternaria"""
        return self.mcp.call("ternary_arithmetic_tool", {
            "mode": "subtract", "a": a, "b": b
        })
    
    def ternary_mul(self, a: int, b: int) -> Dict:
        """Multiplicación ternaria"""
        return self.mcp.call("ternary_arithmetic_tool", {
            "mode": "multiply", "a": a, "b": b
        })
    
    def ternary_to_balanced(self, value: int, width: int = 8) -> Dict:
        """Convertir a ternario balanceado"""
        return self.mcp.call("ternary_representation_tool", {
            "mode": "to_balanced", "value": value, "width": width
        })
    
    def ternary_from_balanced(self, trits: list) -> Dict:
        """Convertir desde ternario balanceado"""
        return self.mcp.call("ternary_representation_tool", {
            "mode": "from_balanced", "trits": trits
        })
    
    def landauer(self, symbols: int, base: int = 3) -> Dict:
        """Límite de Landauer"""
        return self.mcp.call("landauer_ternary_tool", {
            "symbols": symbols, "base": base
        })
    
    def ethnomath(self, system: str, number: int) -> Dict:
        """Etnomatemática"""
        return self.mcp.call("ethnomath_tool", {
            "system": system, "number": number
        })
    
    def ethnomath_compare(self, number: int) -> Dict:
        """Comparar sistemas"""
        return self.mcp.call("ethnomath_comparative_tool", {
            "number": number
        })
    
    def ancestral_calc(self, system: str, operation: str, a: int, b: int) -> Dict:
        """Cálculo ancestral"""
        return self.mcp.call("ancestral_octave_tool", {
            "system": system, "operation": operation, "a": a, "b": b
        })
    
    def ancient_calculator(self, calculator: str, operation: str, a: int, b: int) -> Dict:
        """Calculadora ancestral"""
        return self.mcp.call("ancient_calculators_tool", {
            "calculator": calculator, "operation": operation, "a": a, "b": b
        })


# =============================================================================
# CLI INTERFACE
# =============================================================================

def main():
    if len(sys.argv) < 2:
        print("Usage: tritos_mcp_client.py <tool> [params...]")
        print("")
        print("Tools:")
        print("  ternary_add a b        Suma ternaria")
        print("  ternary_sub a b        Resta ternaria")
        print("  ternary_mul a b        Multiplicación ternaria")
        print("  to_balanced value      Convertir a balanceado")
        print("  landauer symbols       Límite de Landauer")
        print("  ethnomath system num   Etnomatemática")
        print("  compare number         Comparar sistemas")
        print("  list                   Listar tools")
        sys.exit(1)
    
    tool = sys.argv[1]
    math = TritosMath()
    
    if tool == "list":
        tools = math.mcp.list_tools()
        for t in tools:
            print(f"  {t['name']}: {t.get('description', '')[:60]}")
        return
    
    if tool == "ternary_add" and len(sys.argv) >= 4:
        result = math.ternary_add(int(sys.argv[2]), int(sys.argv[3]))
    elif tool == "ternary_sub" and len(sys.argv) >= 4:
        result = math.ternary_sub(int(sys.argv[2]), int(sys.argv[3]))
    elif tool == "ternary_mul" and len(sys.argv) >= 4:
        result = math.ternary_mul(int(sys.argv[2]), int(sys.argv[3]))
    elif tool == "to_balanced" and len(sys.argv) >= 3:
        result = math.ternary_to_balanced(int(sys.argv[2]))
    elif tool == "landauer" and len(sys.argv) >= 3:
        result = math.landauer(int(sys.argv[2]))
    elif tool == "ethnomath" and len(sys.argv) >= 4:
        result = math.ethnomath(sys.argv[2], int(sys.argv[3]))
    elif tool == "compare" and len(sys.argv) >= 3:
        result = math.ethnomath_compare(int(sys.argv[2]))
    else:
        print(f"Unknown or incomplete command: {tool}")
        sys.exit(1)
    
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
