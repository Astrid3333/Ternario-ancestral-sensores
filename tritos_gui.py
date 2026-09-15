#!/usr/bin/env python3
"""
TRITOS — Interfaz Gráfica (GTK3)
Kernel Ternario Ancestral — Escritorio Científico
"""

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango
import subprocess
import os
import json
import time

# =============================================================================
# COLORES DEL TEMA ANCESTRAL
# =============================================================================
COLORS = {
    'bg_dark': '#1a1a2e',
    'bg_panel': '#16213e',
    'bg_card': '#0f3460',
    'accent': '#e94560',
    'gold': '#ffd700',
    'green': '#00ff88',
    'cyan': '#00d4ff',
    'white': '#ffffff',
    'gray': '#8892a0',
    'text': '#e8e8e8',
}

CSS = """
window {
    background-color: #1a1a2e;
}

.main-title {
    font-size: 28px;
    font-weight: bold;
    color: #ffd700;
}

.main-subtitle {
    font-size: 14px;
    color: #8892a0;
}

.category-btn {
    background-color: #0f3460;
    border: 2px solid #1a1a5e;
    border-radius: 12px;
    padding: 20px;
    min-width: 180px;
    min-height: 160px;
}

.category-btn:hover {
    background-color: #1a4a7e;
    border-color: #e94560;
}

.category-icon {
    font-size: 48px;
}

.category-name {
    font-size: 16px;
    font-weight: bold;
    color: #ffffff;
}

.category-desc {
    font-size: 11px;
    color: #8892a0;
}

.status-bar {
    background-color: #16213e;
    border-top: 1px solid #2a2a5e;
    padding: 8px;
    color: #8892a0;
    font-size: 12px;
}

.module-row {
    padding: 8px;
    border-bottom: 1px solid #2a2a5e;
}

.module-name {
    font-size: 14px;
    color: #ffffff;
}

.module-desc {
    font-size: 11px;
    color: #8892a0;
}

.run-btn {
    background-color: #e94560;
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 6px 16px;
    font-weight: bold;
}

.run-btn:hover {
    background-color: #ff6b81;
}

.back-btn {
    background-color: transparent;
    color: #8892a0;
    border: 1px solid #2a2a5e;
    border-radius: 6px;
    padding: 6px 12px;
}

.output-text {
    background-color: #0a0a1a;
    color: #00ff88;
    font-family: monospace;
    font-size: 12px;
    padding: 10px;
    border: 1px solid #2a2a5e;
    border-radius: 6px;
}

.param-label {
    color: #8892a0;
    font-size: 12px;
}

.param-entry {
    background-color: #0a0a1a;
    color: #ffffff;
    border: 1px solid #2a2a5e;
    border-radius: 4px;
    padding: 4px 8px;
}
"""

# =============================================================================
# MÓDULOS CIENCIA
# =============================================================================
SCIENCE_MODULES = [
    {"id": "lga", "name": "LGA", "desc": "Gas de Red Lattice", "icon": "🔬",
     "params": [("Ancho", "16"), ("Alto", "16"), ("Pasos", "20")]},
    {"id": "ising", "name": "Ising", "desc": "Transiciones de Fase", "icon": "🧲",
     "params": [("Ancho", "16"), ("Alto", "16"), ("Pasos", "100"), ("Temp", "2.0")]},
    {"id": "md", "name": "MD", "desc": "Dinámica Molecular", "icon": "⚛️",
     "params": [("Partículas", "8"), ("Pasos", "5")]},
    {"id": "agro", "name": "Agro", "desc": "Sensores Agronómicos", "icon": "🌾",
     "params": [("Nodos", "16"), ("Pasos", "10")]},
    {"id": "seismic", "name": "Sísmico", "desc": "Monitoreo Terrestre", "icon": "🌍",
     "params": [("Estaciones", "8"), ("Pasos", "50")]},
    {"id": "env", "name": "Ambiental", "desc": "Sensores Acoplados", "icon": "🌿",
     "params": [("Nodos", "8"), ("Pasos", "20")]},
    {"id": "perceptron", "name": "Perceptron", "desc": "Red Neuronal Ternaria", "icon": "🧠",
     "params": [("Épocas", "100")]},
    {"id": "hopfield", "name": "Hopfield", "desc": "Memoria Asociativa", "icon": "🔄",
     "params": [("Patrones", "4")]},
    {"id": "logic", "name": "Lógica", "desc": "Tablas Verdaderas", "icon": "📊",
     "params": []},
    {"id": "crypto", "name": "Crypto", "desc": "RSA/DH Ternario", "icon": "🔐",
     "params": [("Bits", "64")]},
    {"id": "compression", "name": "Compresión", "desc": "Comparación de Algoritmos", "icon": "📦",
     "params": [("Tamaño", "1024")]},
    {"id": "blender", "name": "Blender", "desc": "Scripts Python 3D", "icon": "🎨",
     "params": [("Tipo", "sensor")]},
    {"id": "audio", "name": "Audio", "desc": "Compresión WAV Ternaria", "icon": "🎵",
     "params": [("Frecuencia", "440"), ("Duración", "2")]},
    {"id": "compress-opt", "name": "OptComp", "desc": "Compresión Optimizada", "icon": "⚡",
     "params": [("Tamaño", "2048")]},
]

RENDER_MODULES = [
    {"id": "lga", "name": "LGA → Heightmap", "desc": "Densidad como terreno 3D", "icon": "🏔️",
     "params": [("Ancho", "16"), ("Alto", "16"), ("Pasos", "20")]},
    {"id": "ising", "name": "Ising → Voxels", "desc": "Spins como cubos", "icon": "🧊",
     "params": [("Ancho", "16"), ("Alto", "16"), ("Pasos", "500")]},
    {"id": "md", "name": "MD → Esferas", "desc": "Partículas como mallas", "icon": "🔮",
     "params": [("Partículas", "8"), ("Pasos", "5")]},
    {"id": "sensor", "name": "Sensor → Heightmap", "desc": "Red de sensores 3D", "icon": "📡",
     "params": [("Nodos", "16"), ("Pasos", "10")]},
]

SENSOR_TYPES = [
    ("🌡️", "Temperatura", "0-50°C"),
    ("💧", "Humedad", "0-100%"),
    ("📊", "Presión", "300-1100 hPa"),
    ("☀️", "Luz", "0-100000 lux"),
    ("🏃", "Movimiento", "0/1"),
    ("🔊", "Acústica", "0-120 dB"),
    ("🌬️", "Calidad Aire", "0-500 AQI"),
]


class TritosGUI(Gtk.Window):
    def __init__(self):
        super().__init__(title="TRITOS — Kernel Ternario Ancestral")
        self.set_default_size(900, 650)
        self.set_position(Gtk.WindowPosition.CENTER)
        self.connect("destroy", Gtk.main_quit)

        # Apply CSS
        css_provider = Gtk.CssProvider()
        css_provider.load_from_data(CSS.encode())
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        self.build_ui()
        self.show_dashboard()

    def build_ui(self):
        """Construir la estructura principal"""
        self.main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.add(self.main_box)

        # Header
        self.header_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.header_box.set_margin_start(20)
        self.header_box.set_margin_end(20)
        self.header_box.set_margin_top(15)
        self.header_box.set_margin_bottom(10)
        self.main_box.pack_start(self.header_box, False, False, 0)

        # Content area (stack)
        self.stack = Gtk.Stack()
        self.stack.set_transition_type(Gtk.StackTransitionType.SLIDE_LEFT_RIGHT)
        self.stack.set_transition_duration(300)
        self.main_box.pack_start(self.stack, True, True, 0)

        # Status bar
        self.status_bar = Gtk.Label()
        self.status_bar.set_markup(
            f'<span color="{COLORS["gray"]}">📅 Maya: Tzolkin 120/260 │ Haab 45/365 │ ⚡ Carga: +1</span>'
        )
        self.status_bar.set_xalign(0)
        self.status_bar.set_margin_start(20)
        self.status_bar.set_margin_end(20)
        self.status_bar.set_margin_top(5)
        self.status_bar.set_margin_bottom(5)
        self.main_box.pack_end(self.status_bar, False, False, 0)

    def clear_header(self):
        for child in self.header_box.get_children():
            self.header_box.remove(child)

    def show_dashboard(self):
        """Mostrar escritorio principal"""
        self.clear_header()

        # Title
        title = Gtk.Label()
        title.set_markup(f'<span size="xx-large" weight="bold" color="{COLORS["gold"]}">🌿 TRITOS</span>')
        self.header_box.pack_start(title, False, False, 0)

        subtitle = Gtk.Label()
        subtitle.set_markup(f'<span color="{COLORS["gray"]}">Kernel Ternario Ancestral v2.0</span>')
        self.header_box.pack_start(subtitle, False, False, 10)

        # Dashboard grid
        dashboard = Gtk.Grid()
        dashboard.set_column_spacing(15)
        dashboard.set_row_spacing(15)
        dashboard.set_halign(Gtk.Align.CENTER)
        dashboard.set_valign(Gtk.Align.CENTER)
        dashboard.set_margin_start(30)
        dashboard.set_margin_end(30)
        dashboard.set_margin_top(20)

        categories = [
            ("🔬", "CIENCIA", "14 módulos", self.show_science),
            ("📡", "SENSORES", "7 tipos", self.show_sensors),
            ("🖨️", "IMPRESIÓN", "4 renders STL", self.show_render),
            ("🔐", "SEGURIDAD", "RSA/DH", self.show_crypto),
            ("📁", "ARCHIVOS", "Quipu FS", self.show_files),
            ("⚙️", "SISTEMA", "Procesos", self.show_system),
            ("🎵", "AUDIO", "WAV Ternario", self.show_audio),
            ("🧮", "MATEMÁTICA", "Base 60", self.show_math),
        ]

        for i, (icon, name, desc, callback) in enumerate(categories):
            row = i // 4
            col = i % 4
            btn = self.create_category_button(icon, name, desc, callback)
            dashboard.attach(btn, col, row, 1, 1)

        # Replace stack content
        for child in self.stack.get_children():
            self.stack.remove(child)
        self.stack.add_named(dashboard, "dashboard")
        self.stack.set_visible_child_name("dashboard")

    def create_category_button(self, icon, name, desc, callback):
        """Crear botón de categoría"""
        btn = Gtk.Button()
        btn.get_style_context().add_class("category-btn")
        btn.connect("clicked", lambda w: callback())

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
        btn.add(box)

        icon_label = Gtk.Label(label=icon)
        icon_label.get_style_context().add_class("category-icon")
        box.pack_start(icon_label, False, False, 0)

        name_label = Gtk.Label(label=name)
        name_label.get_style_context().add_class("category-name")
        box.pack_start(name_label, False, False, 0)

        desc_label = Gtk.Label(label=desc)
        desc_label.get_style_context().add_class("category-desc")
        box.pack_start(desc_label, False, False, 0)

        return btn

    def show_science(self):
        """Mostrar módulos de ciencia"""
        self.clear_header()

        # Back button
        back_btn = Gtk.Button(label="← Volver")
        back_btn.get_style_context().add_class("back-btn")
        back_btn.connect("clicked", lambda w: self.show_dashboard())
        self.header_box.pack_start(back_btn, False, False, 0)

        title = Gtk.Label()
        title.set_markup(f'<span size="large" weight="bold" color="{COLORS["cyan"]}">🔬 CIENCIA TERNARIA</span>')
        self.header_box.pack_start(title, False, False, 10)

        # Module list
        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)

        list_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        list_box.set_margin_start(20)
        list_box.set_margin_end(20)
        list_box.set_margin_top(10)

        for mod in SCIENCE_MODULES:
            row = self.create_module_row(mod, self.run_science_module)
            list_box.pack_start(row, False, False, 0)

        scroll.add(list_box)

        for child in self.stack.get_children():
            self.stack.remove(child)
        self.stack.add_named(scroll, "science")
        self.stack.set_visible_child_name("science")

    def create_module_row(self, mod, callback):
        """Crear fila de módulo"""
        row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        row.get_style_context().add_class("module-row")
        row.set_margin_top(5)
        row.set_margin_bottom(5)

        icon = Gtk.Label(label=mod["icon"])
        icon.set_margin_end(10)
        row.pack_start(icon, False, False, 0)

        info_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        row.pack_start(info_box, True, True, 0)

        name = Gtk.Label(label=mod["name"])
        name.get_style_context().add_class("module-name")
        name.set_xalign(0)
        info_box.pack_start(name, False, False, 0)

        desc = Gtk.Label(label=mod["desc"])
        desc.get_style_context().add_class("module-desc")
        desc.set_xalign(0)
        info_box.pack_start(desc, False, False, 0)

        run_btn = Gtk.Button(label="Ejecutar")
        run_btn.get_style_context().add_class("run-btn")
        run_btn.connect("clicked", lambda w, m=mod: callback(m))
        row.pack_end(run_btn, False, False, 0)

        return row

    def run_science_module(self, mod):
        """Ejecutar módulo de ciencia"""
        self.clear_header()

        back_btn = Gtk.Button(label="← Volver")
        back_btn.get_style_context().add_class("back-btn")
        back_btn.connect("clicked", lambda w: self.show_science())
        self.header_box.pack_start(back_btn, False, False, 0)

        title = Gtk.Label()
        title.set_markup(f'<span size="large" weight="bold" color="{COLORS["cyan"]}">{mod["icon"]} {mod["name"]}</span>')
        self.header_box.pack_start(title, False, False, 10)

        # Main content
        content = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        content.set_margin_start(20)
        content.set_margin_end(20)
        content.set_margin_top(15)

        # Parameters
        if mod["params"]:
            param_frame = Gtk.Frame(label="Parámetros")
            param_frame.get_style_context().add_class("param-frame")
            content.pack_start(param_frame, False, False, 0)

            param_grid = Gtk.Grid()
            param_grid.set_column_spacing(10)
            param_grid.set_row_spacing(8)
            param_grid.set_margin_start(10)
            param_grid.set_margin_end(10)
            param_grid.set_margin_top(10)
            param_grid.set_margin_bottom(10)
            param_frame.add(param_grid)

            self.param_entries = {}
            for i, (label, default) in enumerate(mod["params"]):
                lbl = Gtk.Label(label=f"{label}:")
                lbl.get_style_context().add_class("param-label")
                lbl.set_xalign(1)
                param_grid.attach(lbl, 0, i, 1, 1)

                entry = Gtk.Entry()
                entry.set_text(default)
                entry.set_width_chars(15)
                entry.get_style_context().add_class("param-entry")
                param_grid.attach(entry, 1, i, 1, 1)
                self.param_entries[label] = entry

        # Buttons
        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        content.pack_start(btn_box, False, False, 0)

        run_btn = Gtk.Button(label="▶ Ejecutar")
        run_btn.get_style_context().add_class("run-btn")
        run_btn.connect("clicked", lambda w, m=mod: self.execute_science(m))
        btn_box.pack_start(run_btn, False, False, 0)

        # Output
        output_frame = Gtk.Frame(label="Resultado")
        content.pack_start(output_frame, True, True, 0)

        self.output_text = Gtk.TextView()
        self.output_text.get_style_context().add_class("output-text")
        self.output_text.set_editable(False)
        self.output_text.set_monospace(True)
        self.output_text.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)

        output_scroll = Gtk.ScrolledWindow()
        output_scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        output_scroll.add(self.output_text)
        output_frame.add(output_scroll)

        for child in self.stack.get_children():
            self.stack.remove(child)
        self.stack.add_named(content, "run_science")
        self.stack.set_visible_child_name("run_science")

    def execute_science(self, mod):
        """Ejecutar el módulo de ciencia"""
        script_dir = os.path.dirname(os.path.abspath(__file__))
        science_bin = os.path.join(script_dir, "tritos_science")

        if not os.path.exists(science_bin):
            science_bin = os.path.join(script_dir, "bin", "tritos_science")

        cmd = [science_bin, mod["id"]]

        for label, entry in self.param_entries.items():
            val = entry.get_text().strip()
            if val:
                short = label[:1].lower()
                cmd.extend([f"-{short}", val])

        self.output_text.get_buffer().set_text(f"Ejecutando: {' '.join(cmd)}\n\n")

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            output = result.stdout + result.stderr
        except subprocess.TimeoutExpired:
            output = "Timeout: la simulación tardó demasiado"
        except Exception as e:
            output = f"Error: {str(e)}"

        self.output_text.get_buffer().set_text(output)

    def show_render(self):
        """Mostrar módulos de render"""
        self.clear_header()

        back_btn = Gtk.Button(label="← Volver")
        back_btn.get_style_context().add_class("back-btn")
        back_btn.connect("clicked", lambda w: self.show_dashboard())
        self.header_box.pack_start(back_btn, False, False, 0)

        title = Gtk.Label()
        title.set_markup(f'<span size="large" weight="bold" color="{COLORS["gold"]}">🖨️ IMPRESIÓN 3D</span>')
        self.header_box.pack_start(title, False, False, 10)

        list_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        list_box.set_margin_start(20)
        list_box.set_margin_end(20)
        list_box.set_margin_top(10)

        for mod in RENDER_MODULES:
            row = self.create_module_row(mod, self.run_render_module)
            list_box.pack_start(row, False, False, 0)

        for child in self.stack.get_children():
            self.stack.remove(child)
        self.stack.add_named(list_box, "render")
        self.stack.set_visible_child_name("render")

    def run_render_module(self, mod):
        """Ejecutar render STL"""
        self.clear_header()

        back_btn = Gtk.Button(label="← Volver")
        back_btn.get_style_context().add_class("back-btn")
        back_btn.connect("clicked", lambda w: self.show_render())
        self.header_box.pack_start(back_btn, False, False, 0)

        title = Gtk.Label()
        title.set_markup(f'<span size="large" weight="bold" color="{COLORS["gold"]}">{mod["icon"]} {mod["name"]}</span>')
        self.header_box.pack_start(title, False, False, 10)

        content = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        content.set_margin_start(20)
        content.set_margin_end(20)
        content.set_margin_top(15)

        # Parameters
        param_frame = Gtk.Frame(label="Parámetros")
        content.pack_start(param_frame, False, False, 0)

        param_grid = Gtk.Grid()
        param_grid.set_column_spacing(10)
        param_grid.set_row_spacing(8)
        param_grid.set_margin_start(10)
        param_grid.set_margin_end(10)
        param_grid.set_margin_top(10)
        param_grid.set_margin_bottom(10)
        param_frame.add(param_grid)

        self.render_entries = {}
        for i, (label, default) in enumerate(mod["params"]):
            lbl = Gtk.Label(label=f"{label}:")
            lbl.get_style_context().add_class("param-label")
            lbl.set_xalign(1)
            param_grid.attach(lbl, 0, i, 1, 1)

            entry = Gtk.Entry()
            entry.set_text(default)
            entry.set_width_chars(15)
            entry.get_style_context().add_class("param-entry")
            param_grid.attach(entry, 1, i, 1, 1)
            self.render_entries[label] = entry

        # Output filename
        out_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        content.pack_start(out_row, False, False, 0)

        out_lbl = Gtk.Label(label="Archivo STL:")
        out_lbl.get_style_context().add_class("param-label")
        out_row.pack_start(out_lbl, False, False, 0)

        self.render_output_entry = Gtk.Entry()
        self.render_output_entry.set_text(f"/tmp/tritos_{mod['id']}.stl")
        self.render_output_entry.set_width_chars(30)
        self.render_output_entry.get_style_context().add_class("param-entry")
        out_row.pack_start(self.render_output_entry, False, False, 0)

        # Buttons
        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        content.pack_start(btn_box, False, False, 0)

        run_btn = Gtk.Button(label="▶ Generar STL")
        run_btn.get_style_context().add_class("run-btn")
        run_btn.connect("clicked", lambda w, m=mod: self.execute_render(m))
        btn_box.pack_start(run_btn, False, False, 0)

        # Output
        output_frame = Gtk.Frame(label="Resultado")
        content.pack_start(output_frame, True, True, 0)

        self.output_text = Gtk.TextView()
        self.output_text.get_style_context().add_class("output-text")
        self.output_text.set_editable(False)
        self.output_text.set_monospace(True)

        output_scroll = Gtk.ScrolledWindow()
        output_scroll.add(self.output_text)
        output_frame.add(output_scroll)

        for child in self.stack.get_children():
            self.stack.remove(child)
        self.stack.add_named(content, "run_render")
        self.stack.set_visible_child_name("run_render")

    def execute_render(self, mod):
        """Ejecutar render STL"""
        script_dir = os.path.dirname(os.path.abspath(__file__))
        science_bin = os.path.join(script_dir, "tritos_science")

        if not os.path.exists(science_bin):
            science_bin = os.path.join(script_dir, "bin", "tritos_science")

        output_file = self.render_output_entry.get_text().strip()

        cmd = [science_bin, "render", mod["id"]]

        for label, entry in self.render_entries.items():
            val = entry.get_text().strip()
            if val:
                short = label[:1].lower()
                cmd.extend([f"-{short}", val])

        cmd.extend(["-o", output_file])

        self.output_text.get_buffer().set_text(f"Generando STL...\n{' '.join(cmd)}\n\n")

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            output = result.stdout + result.stderr
            if os.path.exists(output_file):
                size = os.path.getsize(output_file)
                output += f"\n✅ STL generado: {output_file} ({size:,} bytes)"
        except Exception as e:
            output = f"Error: {str(e)}"

        self.output_text.get_buffer().set_text(output)

    def show_sensors(self):
        self.clear_header()
        back_btn = Gtk.Button(label="← Volver")
        back_btn.get_style_context().add_class("back-btn")
        back_btn.connect("clicked", lambda w: self.show_dashboard())
        self.header_box.pack_start(back_btn, False, False, 0)

        title = Gtk.Label()
        title.set_markup(f'<span size="large" weight="bold" color="{COLORS["green"]}">📡 SENSORES VIRTUALES</span>')
        self.header_box.pack_start(title, False, False, 10)

        grid = Gtk.Grid()
        grid.set_column_spacing(15)
        grid.set_row_spacing(15)
        grid.set_halign(Gtk.Align.CENTER)
        grid.set_margin_top(30)

        for i, (icon, name, range_) in enumerate(SENSOR_TYPES):
            row = i // 4
            col = i % 4

            card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=5)
            card.get_style_context().add_class("category-btn")
            card.set_margin_start(5)
            card.set_margin_end(5)

            icon_lbl = Gtk.Label(label=icon)
            icon_lbl.get_style_context().add_class("category-icon")
            card.pack_start(icon_lbl, False, False, 0)

            name_lbl = Gtk.Label(label=name)
            name_lbl.get_style_context().add_class("category-name")
            card.pack_start(name_lbl, False, False, 0)

            range_lbl = Gtk.Label(label=range_)
            range_lbl.get_style_context().add_class("category-desc")
            card.pack_start(range_lbl, False, False, 0)

            grid.attach(card, col, row, 1, 1)

        for child in self.stack.get_children():
            self.stack.remove(child)
        self.stack.add_named(grid, "sensors")
        self.stack.set_visible_child_name("sensors")

    def show_crypto(self):
        self._show_simple("🔐 SEGURIDAD", "RSA/DH Ternario", COLORS["accent"])

    def show_files(self):
        self._show_simple("📁 ARCHIVOS", "Quipu Filesystem", COLORS["cyan"])

    def show_system(self):
        self._show_simple("⚙️ SISTEMA", "Procesos y Memoria", COLORS["green"])

    def show_audio(self):
        self._show_simple("🎵 AUDIO", "Compresión WAV Ternaria", COLORS["gold"])

    def show_math(self):
        self._show_simple("🧮 MATEMÁTICA", "Base 60 / Calendario Maya", COLORS["cyan"])

    def _show_simple(self, title_text, subtitle, color):
        self.clear_header()
        back_btn = Gtk.Button(label="← Volver")
        back_btn.get_style_context().add_class("back-btn")
        back_btn.connect("clicked", lambda w: self.show_dashboard())
        self.header_box.pack_start(back_btn, False, False, 0)

        title = Gtk.Label()
        title.set_markup(f'<span size="large" weight="bold" color="{color}">{title_text}</span>')
        self.header_box.pack_start(title, False, False, 10)

        label = Gtk.Label(label=f"\n\n  {subtitle}\n  Próximamente...")
        label.get_style_context().add_class("module-desc")

        for child in self.stack.get_children():
            self.stack.remove(child)
        self.stack.add_named(label, "simple")
        self.stack.set_visible_child_name("simple")


def main():
    app = TritosGUI()
    app.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
