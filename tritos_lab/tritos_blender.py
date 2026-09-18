#!/usr/bin/env python3
"""
tritos_blender.py — Integración Blender para Tritos OS
Renderizado 3D de datos ternarios, visualización científica, animaciones.
"""

import subprocess
import os
import json
import tempfile

class TritosBlender:
    """Cliente Blender headless para Tritos Lab."""

    def __init__(self):
        self.blender = self._find_blender()
        self.available = self.blender is not None

    def _find_blender(self):
        """Busca Blender en el sistema."""
        paths = [
            "/usr/bin/blender",
            "/usr/local/bin/blender",
            "/snap/bin/blender",
            os.path.expanduser("~/.local/bin/blender"),
        ]
        for p in paths:
            if os.path.exists(p):
                return p
        # Try PATH
        try:
            result = subprocess.run(["which", "blender"], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
        except:
            pass
        return None

    def render_sphere_grid(self, values, output="/tmp/tritos_spheres.png"):
        """Renderiza una grilla de esferas con valores ternarios."""
        if not self.available:
            return {"error": "Blender not found"}

        # Generate Blender Python script
        script = f"""
import bpy
import math

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# Values to visualize
values = {json.dumps(values)}

# Create sphere grid
cols = int(math.ceil(math.sqrt(len(values))))
for i, val in enumerate(values):
    row = i // cols
    col = i % cols

    # Create sphere
    bpy.ops.mesh.primitive_uv_sphere_add(
        radius=0.3 + abs(val) * 0.2,
        location=(col * 1.2, row * 1.2, 0)
    )
    obj = bpy.context.active_object

    # Color based on value: negative=blue, zero=white, positive=red
    mat = bpy.data.materials.new(name=f"mat_{{i}}")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    if val < 0:
        bsdf.inputs['Base Color'].default_value = (0.2, 0.4, 1.0, 1)
    elif val == 0:
        bsdf.inputs['Base Color'].default_value = (0.9, 0.9, 0.9, 1)
    else:
        bsdf.inputs['Base Color'].default_value = (1.0, 0.3, 0.2, 1)
    obj.data.materials.append(mat)

# Camera and light
bpy.ops.object.camera_add(location=(cols * 0.6, -5, cols * 0.4))
cam = bpy.context.active_object
cam.rotation_euler = (math.radians(70), 0, 0)
bpy.context.scene.camera = cam

bpy.ops.object.light_add(type='SUN', location=(5, -5, 10))

# Render
bpy.context.scene.render.resolution_x = 1024
bpy.context.scene.render.resolution_y = 768
bpy.context.scene.render.filepath = "{output}"
bpy.ops.render.render(write_still=True)
"""
        return self._run_script(script, output)

    def render_ternary_tree(self, trits, output="/tmp/tritos_tree.png"):
        """Renderiza un árbol ternario (estructura de decisión)."""
        if not self.available:
            return {"error": "Blender not found"}

        script = f"""
import bpy
import math

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

trits = {json.dumps(trits)}

def create_node(x, y, z, text, color=(0.9, 0.9, 0.9)):
    bpy.ops.mesh.primitive_cube_add(size=0.6, location=(x, y, z))
    obj = bpy.context.active_object
    mat = bpy.data.materials.new(name=f"node_{{x}}_{{y}}")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    obj.data.materials.append(mat)
    return obj

def create_edge(start, end):
    mesh = bpy.data.meshes.new("edge")
    obj = bpy.data.objects.new("edge", mesh)
    bpy.context.collection.objects.link(obj)
    mesh.from_pydata([start, end], [[0, 1]], [])

# Build tree from trits
level = 0
nodes = [(0, 0, 0)]
create_node(0, 0, 0, "root", (0.9, 0.8, 0.2))

for i, t in enumerate(trits[:6]):
    new_nodes = []
    for x, y, z in nodes:
        for dx, offset in [(-1, -1), (0, 0), (1, 1)]:
            nx = x + dx * (2 ** (5 - i))
            ny = y - 1.5
            nz = z + offset * 0.3
            color = (0.2, 0.4, 1.0) if t < 0 else ((0.9, 0.9, 0.9) if t == 0 else (1.0, 0.3, 0.2))
            create_node(nx, ny, nz, str(t), color)
            new_nodes.append((nx, ny, nz))
    nodes = new_nodes

# Camera
bpy.ops.object.camera_add(location=(0, -12, 8))
cam = bpy.context.active_object
cam.rotation_euler = (math.radians(60), 0, 0)
bpy.context.scene.camera = cam

bpy.ops.object.light_add(type='SUN', location=(5, -5, 10))

bpy.context.scene.render.resolution_x = 1024
bpy.context.scene.render.resolution_y = 768
bpy.context.scene.render.filepath = "{output}"
bpy.ops.render.render(write_still=True)
"""
        return self._run_script(script, output)

    def render_sensor_data(self, sensors, output="/tmp/tritos_sensors.png"):
        """Renderiza datos de sensores como barras 3D."""
        if not self.available:
            return {"error": "Blender not found"}

        script = f"""
import bpy
import math

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

sensors = {json.dumps(sensors)}

for i, (name, value) in enumerate(sensors.items()):
    # Bar
    height = abs(value) * 0.5
    bpy.ops.mesh.primitive_cube_add(
        size=1,
        location=(i * 2, 0, height / 2),
        scale=(0.8, 0.8, height if height > 0.1 else 0.1)
    )
    obj = bpy.context.active_object
    mat = bpy.data.materials.new(name=f"bar_{{i}}")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    # Color: green=good, yellow=warning, red=alert
    if value < 30:
        bsdf.inputs['Base Color'].default_value = (0.2, 0.8, 0.3, 1)
    elif value < 70:
        bsdf.inputs['Base Color'].default_value = (1.0, 0.8, 0.2, 1)
    else:
        bsdf.inputs['Base Color'].default_value = (1.0, 0.2, 0.2, 1)
    obj.data.materials.append(mat)

bpy.ops.object.camera_add(location=(len(sensors), -8, 6))
cam = bpy.context.active_object
cam.rotation_euler = (math.radians(65), 0, 0)
bpy.context.scene.camera = cam

bpy.ops.object.light_add(type='SUN', location=(5, -5, 10))

bpy.context.scene.render.resolution_x = 1024
bpy.context.scene.render.resolution_y = 768
bpy.context.scene.render.filepath = "{output}"
bpy.ops.render.render(write_still=True)
"""
        return self._run_script(script, output)

    def create_animation(self, frames=60, output="/tmp/tritos_anim.mp4"):
        """Crea una animación de esferas ternarias."""
        if not self.available:
            return {"error": "Blender not found"}

        script = f"""
import bpy
import math

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# Create animated spheres
for i in range(10):
    bpy.ops.mesh.primitive_uv_sphere_add(
        radius=0.5,
        location=(i * 1.5 - 7, 0, 0)
    )
    obj = bpy.context.active_object
    obj.name = f"sphere_{{i}}"

    # Keyframe animation
    obj.location.z = 0
    obj.keyframe_insert(data_path="location", index=2, frame=1)
    obj.location.z = math.sin(i * 0.5) * 2
    obj.keyframe_insert(data_path="location", index=2, frame={frames})

    # Color
    mat = bpy.data.materials.new(name=f"anim_mat_{{i}}")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    hue = i / 10.0
    r = math.sin(hue * 6.28) * 0.5 + 0.5
    g = math.sin(hue * 6.28 + 2.09) * 0.5 + 0.5
    b = math.sin(hue * 6.28 + 4.18) * 0.5 + 0.5
    bsdf.inputs['Base Color'].default_value = (r, g, b, 1)
    obj.data.materials.append(mat)

# Camera
bpy.ops.object.camera_add(location=(0, -12, 4))
cam = bpy.context.active_object
bpy.context.scene.camera = cam

bpy.ops.object.light_add(type='SUN', location=(5, -5, 10))

# Render settings
bpy.context.scene.render.resolution_x = 800
bpy.context.scene.render.resolution_y = 600
bpy.context.scene.render.fps = 30
bpy.context.scene.frame_end = {frames}
bpy.context.scene.render.filepath = "{output}"
bpy.context.scene.render.image_settings.file_format = 'FFMPEG'
bpy.context.scene.render.ffmpeg.format = 'MPEG4'
bpy.context.scene.render.ffmpeg.codec = 'H264'

bpy.ops.render.render(animation=True)
"""
        return self._run_script(script, output)

    def _run_script(self, script, output=None):
        """Ejecuta un script de Blender."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False) as f:
            f.write(script)
            script_path = f.name

        try:
            result = subprocess.run(
                [self.blender, "--background", "--python", script_path],
                capture_output=True, text=True, timeout=120
            )
            if result.returncode == 0 and output and os.path.exists(output):
                return {"success": True, "output": output}
            else:
                return {"error": result.stderr[-500:] if result.stderr else "Unknown error"}
        except subprocess.TimeoutExpired:
            return {"error": "Blender timeout (>120s)"}
        except Exception as e:
            return {"error": str(e)}
        finally:
            os.unlink(script_path)


# =============================================================================
# CLI
# =============================================================================

if __name__ == "__main__":
    import sys

    blender = TritosBlender()
    print(f"Blender available: {blender.available}")
    print(f"Blender path: {blender.blender}")

    if not blender.available:
        print("Install: sudo apt install blender")
        sys.exit(1)

    # Test: render sphere grid
    print("\nRendering test spheres...")
    values = [-2, -1, 0, 1, 2, -1, 0, 1, 0]
    result = blender.render_sphere_grid(values)
    print(f"Result: {result}")
