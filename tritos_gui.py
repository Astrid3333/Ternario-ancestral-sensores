#!/usr/bin/env python3
"""TRITOS - Ternary Ancestral Kernel — GTK3 Desktop Interface"""

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango
import subprocess
import os
import math
import struct
import wave
import datetime
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SCIENCE_BIN = os.path.join(SCRIPT_DIR, "tritos_science")
if not os.path.exists(SCIENCE_BIN):
    SCIENCE_BIN = os.path.join(SCRIPT_DIR, "bin", "tritos_science")

CSS = b"""
* { font-family: sans-serif; }
window { background-color: #1a1a2e; }
.main-title { font-size: 28px; font-weight: bold; color: #ffd700; }
.main-subtitle { font-size: 13px; color: #8892a0; }

.taskbar {
    background-color: #0d1117;
    border-top: 1px solid #30363d;
    padding: 2px 8px;
}
.start-btn {
    background-color: #1a7f37;
    color: #ffffff;
    border: none;
    border-radius: 4px;
    padding: 4px 12px;
    font-weight: bold;
    font-size: 13px;
}
.start-btn:hover { background-color: #238636; }
.quick-launch-btn {
    background-color: transparent;
    color: #c9d1d9;
    border: none;
    border-radius: 4px;
    padding: 4px 8px;
    font-size: 12px;
}
.quick-launch-btn:hover { background-color: #21262d; }
.clock-label {
    color: #8b949e;
    font-size: 12px;
    font-family: monospace;
}
.start-menu {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 8px;
    padding: 4px;
}
.start-menu-item {
    background-color: transparent;
    color: #c9d1d9;
    border: none;
    border-radius: 4px;
    padding: 8px 16px;

    font-size: 13px;
}
.start-menu-item:hover { background-color: #21262d; }
.start-menu-section {
    color: #8b949e;
    font-size: 10px;
    padding: 4px 16px;
}

.desktop-icon {
    background-color: transparent;
    border: none;
    border-radius: 6px;
    padding: 12px;
    min-width: 110px;
}
.desktop-icon:hover { background-color: rgba(255,255,255,0.08); }
.desktop-icon-label { font-size: 11px; color: #c9d1d9; }

.card {
    background-color: #161b22;
    border: 1px solid #30363d;
    border-radius: 8px;
    padding: 12px;
}
.card:hover { border-color: #58a6ff; }
.card-icon { font-size: 32px; }
.card-name { font-size: 13px; font-weight: bold; color: #c9d1d9; }
.card-desc { font-size: 10px; color: #8b949e; }

.back-btn {
    background-color: #21262d;
    color: #c9d1d9;
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 6px 14px;
    font-size: 12px;
}
.back-btn:hover { background-color: #30363d; }
.run-btn {
    background-color: #238636;
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 6px 16px;
    font-weight: bold;
    font-size: 12px;
}
.run-btn:hover { background-color: #2ea043; }
.gen-btn {
    background-color: #1f6feb;
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 6px 14px;
    font-size: 12px;
}
.gen-btn:hover { background-color: #388bfd; }
.del-btn {
    background-color: transparent;
    color: #f85149;
    border: 1px solid #f85149;
    border-radius: 4px;
    padding: 2px 8px;
    font-size: 11px;
}
.del-btn:hover { background-color: #f85149; color: #ffffff; }

.output-text {
    background-color: #0d1117;
    color: #7ee787;
    font-family: monospace;
    font-size: 12px;
    padding: 8px;
    border: 1px solid #30363d;
    border-radius: 6px;
}
.param-label { color: #8b949e; font-size: 12px; }
.param-entry {
    background-color: #0d1117;
    color: #c9d1d9;
    border: 1px solid #30363d;
    border-radius: 4px;
    padding: 4px 8px;
}
.param-entry:focus { border-color: #58a6ff; }
.section-title { font-size: 18px; font-weight: bold; }

.page-title {
    font-size: 16px;
    font-weight: bold;
    color: #58a6ff;
}
.frame-title {
    color: #8b949e;
    font-size: 12px;
}

.file-item {
    background-color: transparent;
    border: none;
    border-radius: 4px;
    padding: 6px 10px;

    font-size: 12px;
    color: #c9d1d9;
}
.file-item:hover { background-color: #21262d; }
.path-label { color: #8b949e; font-size: 11px; font-family: monospace; }

.term-quick-btn {
    background-color: #21262d;
    color: #c9d1d9;
    border: 1px solid #30363d;
    border-radius: 4px;
    padding: 3px 8px;
    font-size: 11px;
}
.term-quick-btn:hover { background-color: #30363d; border-color: #58a6ff; }
.term-calc-btn {
    background-color: #161b22;
    color: #c9d1d9;
    border: 1px solid #30363d;
    border-radius: 4px;
    padding: 6px 12px;
    font-size: 13px;
    font-weight: bold;
    min-width: 40px;
}
.term-calc-btn:hover { background-color: #21262d; border-color: #ffd700; }
"""

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

START_MENU_ITEMS = [
    ("section", "INTELIGENCIA"),
    ("item", "🧠 IA Ternaria", "ai"),
    ("section", "APLICACIONES"),
    ("item", "🔬 Ciencia Ternaria", "science"),
    ("item", "📡 Sensores", "sensors"),
    ("item", "🖨️ Impresión 3D", "render"),
    ("item", "🌐 Navegador", "browser"),
    ("item", "🔌 Arduino", "arduino"),
    ("section", "HERRAMIENTAS"),
    ("item", "🔐 Seguridad", "security"),
    ("item", "🛡️ Firewall", "firewall"),
    ("item", "👶 Control Parental", "parental"),
    ("item", "📁 Archivos", "files"),
    ("item", "⚙️ Sistema", "system"),
    ("item", "🎵 Audio", "audio"),
    ("item", "🧮 Matemática", "math"),
    ("item", "🔢 Terminal Bin/Tern", "terminal"),
    ("item", "📚 Fórmulas", "formulas"),
    ("section", "CIENCIA"),
    ("item", "🧪 Experimentos", "experiments"),
    ("item", "🖥️ Kernel Bare-Metal", "kernel"),
    ("section", "SISTEMA"),
    ("item", "🐚 Shell Tritos", "shell"),
]


class TritosGUI(Gtk.Window):
    def __init__(self):
        super().__init__(title="TRITOS")
        self.set_default_size(1024, 700)
        self.set_position(Gtk.WindowPosition.CENTER)
        self.connect("destroy", Gtk.main_quit)
        self.connect("delete-event", lambda w, e: Gtk.main_quit())

        css_provider = Gtk.CssProvider()
        css_provider.load_from_data(CSS)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(), css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self._build_ui()
        self._show_desktop()
        self._start_clock()

    def _build_ui(self):
        self.vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.add(self.vbox)

        self.page_stack = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.vbox.pack_start(self.page_stack, True, True, 0)

        self._build_taskbar()
        self.vbox.pack_end(self.taskbar, False, False, 0)

        self.start_menu = None

    def _build_taskbar(self):
        self.taskbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        self.taskbar.get_style_context().add_class("taskbar")
        self.taskbar.set_margin_top(2)
        self.taskbar.set_margin_bottom(2)

        start_btn = Gtk.Button(label="🌿 TRITOS")
        start_btn.get_style_context().add_class("start-btn")
        start_btn.connect("clicked", self._toggle_start_menu)
        self.taskbar.pack_start(start_btn, False, False, 0)

        sep = Gtk.Separator(orientation=Gtk.Orientation.VERTICAL)
        self.taskbar.pack_start(sep, False, False, 0)

        quick_items = [
            ("📁 Archivos", "files"), ("⚙️ Sistema", "system"), ("🧮 Matemática", "math"),
        ]
        for label, section in quick_items:
            btn = Gtk.Button(label=label)
            btn.get_style_context().add_class("quick-launch-btn")
            btn.connect("clicked", lambda w, s=section: self._navigate_to(s))
            self.taskbar.pack_start(btn, False, False, 0)

        self.taskbar.pack_start(Gtk.Label(), True, True, 0)

        self.clock_label = Gtk.Label()
        self.clock_label.get_style_context().add_class("clock-label")
        self.taskbar.pack_end(self.clock_label, False, False, 0)

    def _start_clock(self):
        def update_clock():
            now = datetime.datetime.now()
            self.clock_label.set_text(now.strftime("%d/%m/%Y  %H:%M:%S"))
            return True
        update_clock()
        GLib.timeout_add_seconds(1, update_clock)

    def _toggle_start_menu(self, btn=None):
        if self.start_menu and self.start_menu.get_visible():
            self.start_menu.hide()
            return
        menu = Gtk.Window(type=Gtk.WindowType.POPUP)
        menu.set_decorated(False)
        menu.set_resizable(False)
        menu.get_style_context().add_class("start-menu")
        menu.set_size_request(240, -1)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        vbox.set_margin_top(4)
        vbox.set_margin_bottom(4)
        menu.add(vbox)

        for kind, label, *rest in START_MENU_ITEMS:
            if kind == "section":
                lbl = Gtk.Label(label=label)
                lbl.get_style_context().add_class("start-menu-section")
                lbl.set_xalign(0)
                lbl.set_margin_top(6)
                vbox.pack_start(lbl, False, False, 0)
            else:
                section_id = rest[0] if rest else ""
                item_btn = Gtk.Button(label=label)
                item_btn.get_style_context().add_class("start-menu-item")
                item_btn.set_halign(Gtk.Align.FILL)
                item_btn.connect("clicked",
                    lambda w, s=section_id: (self.start_menu.hide(),
                                             self._navigate_to(s)))
                vbox.pack_start(item_btn, False, False, 0)

        menu.show_all()

        try:
            parent = self.taskbar.get_parent_window()
            if parent:
                tx, ty = self.taskbar.translate_coordinates(parent, 0, 0)
                menu.move(tx, ty - menu.get_allocated_height())
            else:
                menu.move(100, 100)
        except Exception:
            menu.move(100, 100)

        self.start_menu = menu

    def _clear_page(self):
        for child in self.page_stack.get_children():
            self.page_stack.remove(child)

    def _make_page(self, title_text, back_target="desktop"):
        self._clear_page()
        outer = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)

        header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        header.set_margin_start(12)
        header.set_margin_end(12)
        header.set_margin_top(10)
        header.set_margin_bottom(8)

        back_btn = Gtk.Button(label="← Volver")
        back_btn.get_style_context().add_class("back-btn")
        back_btn.connect("clicked", lambda w: self._navigate_to(back_target))
        header.pack_start(back_btn, False, False, 0)

        title_label = Gtk.Label()
        title_label.set_markup(
            f'<span size="large" weight="bold" color="#58a6ff">{title_text}</span>')
        header.pack_start(title_label, False, False, 0)

        outer.pack_start(header, False, False, 0)

        sep = Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL)
        outer.pack_start(sep, False, False, 0)

        content = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        content.set_margin_start(12)
        content.set_margin_end(12)
        content.set_margin_top(6)
        outer.pack_start(content, True, True, 0)

        self.page_stack.pack_start(outer, True, True, 0)
        return content

    def _navigate_to(self, section):
        if self.start_menu and self.start_menu.get_visible():
            self.start_menu.hide()

        dispatch = {
            "desktop": self._show_desktop,
            "science": self._show_science,
            "sensors": self._show_sensors,
            "render": self._show_render,
            "crypto": self._show_crypto,
            "files": self._show_files,
            "system": self._show_system,
            "audio": self._show_audio,
            "math": self._show_math,
            "terminal": self._show_terminal,
            "formulas": self._show_formulas,
            "shell": self._show_shell,
            "ai": self._show_ai,
            "browser": self._show_browser,
            "arduino": self._show_arduino,
            "security": self._show_security,
            "firewall": self._show_firewall,
            "parental": self._show_parental,
            "experiments": self._show_experiments,
            "kernel": self._show_kernel,
        }
        fn = dispatch.get(section, self._show_desktop)
        fn()
        # Show all widgets that were added after _make_page
        for child in self.page_stack.get_children():
            child.show_all()

    def _show_desktop(self):
        self._clear_page()

        desktop_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)

        title_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        title_box.set_margin_start(20)
        title_box.set_margin_top(20)
        title_box.set_margin_bottom(10)
        title = Gtk.Label()
        title.set_markup(
            '<span size="x-large" weight="bold" color="#ffd700">🌿 TRITOS</span>')
        title_box.pack_start(title, False, False, 0)
        subtitle = Gtk.Label()
        subtitle.set_markup(
            '<span color="#8b949e" size="small">Kernel Ternario Ancestral v2.0</span>')
        title_box.pack_start(subtitle, False, False, 0)
        desktop_box.pack_start(title_box, False, False, 0)

        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)

        icon_grid = Gtk.Grid()
        icon_grid.set_column_spacing(8)
        icon_grid.set_row_spacing(8)
        icon_grid.set_margin_start(20)
        icon_grid.set_margin_end(20)
        icon_grid.set_halign(Gtk.Align.START)
        icon_grid.set_valign(Gtk.Align.START)

        desktop_icons = [
            ("🧠", "IA\nTernaria", "ai"),
            ("🔬", "Ciencia\nTernaria", "science"),
            ("📡", "Sensores", "sensors"),
            ("🖨️", "Impresión\n3D", "render"),
            ("🌐", "Navegador", "browser"),
            ("🔌", "Arduino", "arduino"),
            ("🔐", "Seguridad", "security"),
            ("🛡️", "Firewall", "firewall"),
            ("👶", "Control\nParental", "parental"),
            ("📁", "Archivos", "files"),
            ("⚙️", "Sistema", "system"),
            ("🎵", "Audio", "audio"),
            ("🧮", "Matemática", "math"),
            ("🔢", "Terminal\nBin/Tern", "terminal"),
            ("📚", "Fórmulas", "formulas"),
            ("🧪", "Experi-\nmentos", "experiments"),
            ("🖥️", "Kernel\nBare-Metal", "kernel"),
            ("🐚", "Shell\nTritos", "shell"),
        ]

        for i, (icon, label, section) in enumerate(desktop_icons):
            col = i % 4
            row = i // 4
            btn = Gtk.Button()
            btn.get_style_context().add_class("desktop-icon")
            btn.set_relief(Gtk.ReliefStyle.NONE)

            vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
            vbox.set_halign(Gtk.Align.CENTER)
            icon_lbl = Gtk.Label(label=icon)
            icon_lbl.set_markup(f'<span size="xx-large">{icon}</span>')
            vbox.pack_start(icon_lbl, False, False, 0)
            name_lbl = Gtk.Label(label=label)
            name_lbl.get_style_context().add_class("desktop-icon-label")
            name_lbl.set_justify(Gtk.Justification.CENTER)
            name_lbl.set_line_wrap(True)
            vbox.pack_start(name_lbl, False, False, 0)

            btn.add(vbox)
            btn.connect("clicked", lambda w, s=section: self._navigate_to(s))
            icon_grid.attach(btn, col, row, 1, 1)

        scroll.add(icon_grid)
        desktop_box.pack_start(scroll, True, True, 0)

        self.page_stack.pack_start(desktop_box, True, True, 0)
        desktop_box.show_all()

    def _show_science(self):
        content = self._make_page("🔬 CIENCIA TERNARIA")

        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)

        grid = Gtk.Grid()
        grid.set_column_spacing(10)
        grid.set_row_spacing(10)
        grid.set_halign(Gtk.Align.CENTER)
        grid.set_valign(Gtk.Align.START)

        for i, mod in enumerate(SCIENCE_MODULES):
            col = i % 4
            row = i // 4
            card = Gtk.Button()
            card.get_style_context().add_class("card")
            card.set_relief(Gtk.ReliefStyle.NONE)

            vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
            vbox.set_halign(Gtk.Align.CENTER)
            icon_lbl = Gtk.Label(label=mod["icon"])
            icon_lbl.get_style_context().add_class("card-icon")
            vbox.pack_start(icon_lbl, False, False, 0)
            name_lbl = Gtk.Label(label=mod["name"])
            name_lbl.get_style_context().add_class("card-name")
            vbox.pack_start(name_lbl, False, False, 0)
            desc_lbl = Gtk.Label(label=mod["desc"])
            desc_lbl.get_style_context().add_class("card-desc")
            vbox.pack_start(desc_lbl, False, False, 0)

            card.add(vbox)
            card.connect("clicked", lambda w, m=mod: self._show_science_module(m))
            grid.attach(card, col, row, 1, 1)

        scroll.add(grid)
        content.pack_start(scroll, True, True, 0)

    def _show_science_module(self, mod):
        content = self._make_page(f'{mod["icon"]} {mod["name"]}', "science")

        if mod["params"]:
            pf = Gtk.Frame(label=" Parámetros ")
            pf.get_style_context().add_class("card")
            pg = Gtk.Grid()
            pg.set_column_spacing(10)
            pg.set_row_spacing(6)
            pg.set_margin_start(8)
            pg.set_margin_end(8)
            pg.set_margin_top(8)
            pg.set_margin_bottom(8)
            pf.add(pg)
            content.pack_start(pf, False, False, 0)

            self._sci_params = {}
            for idx, (label, default) in enumerate(mod["params"]):
                lbl = Gtk.Label(label=f"{label}:")
                lbl.get_style_context().add_class("param-label")
                lbl.set_xalign(1)
                pg.attach(lbl, 0, idx, 1, 1)
                entry = Gtk.Entry()
                entry.set_text(default)
                entry.set_width_chars(12)
                entry.get_style_context().add_class("param-entry")
                pg.attach(entry, 1, idx, 1, 1)
                self._sci_params[label] = entry

        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        btn_box.set_margin_top(4)
        run_btn = Gtk.Button(label="▶ Ejecutar")
        run_btn.get_style_context().add_class("run-btn")
        run_btn.connect("clicked", lambda w, m=mod: self._exec_science(m))
        btn_box.pack_start(run_btn, False, False, 0)
        content.pack_start(btn_box, False, False, 0)

        of = Gtk.Frame(label=" Resultado ")
        of.get_style_context().add_class("card")
        self._sci_output = Gtk.TextView()
        self._sci_output.get_style_context().add_class("output-text")
        self._sci_output.set_editable(False)
        self._sci_output.set_monospace(True)
        self._sci_output.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw.add(self._sci_output)
        of.add(sw)
        content.pack_start(of, True, True, 0)

    def _exec_science(self, mod):
        cmd = [SCIENCE_BIN, mod["id"]]
        for label, entry in self._sci_params.items():
            val = entry.get_text().strip()
            if val:
                cmd.extend([f"-{label[0].lower()}", val])
        self._sci_output.get_buffer().set_text(f"$ {' '.join(cmd)}\n\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            self._sci_output.get_buffer().set_text(r.stdout + r.stderr)
        except subprocess.TimeoutExpired:
            self._sci_output.get_buffer().set_text("Timeout después de 30s")
        except Exception as e:
            self._sci_output.get_buffer().set_text(f"Error: {e}")

    def _show_sensors(self):
        content = self._make_page("📡 SENSORES VIRTUALES")

        grid = Gtk.Grid()
        grid.set_column_spacing(12)
        grid.set_row_spacing(12)
        grid.set_halign(Gtk.Align.CENTER)
        grid.set_valign(Gtk.Align.START)
        grid.set_margin_top(12)

        for i, (icon, name, range_) in enumerate(SENSOR_TYPES):
            col = i % 4
            row = i // 4
            card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
            card.get_style_context().add_class("card")
            card.set_halign(Gtk.Align.CENTER)
            card.set_size_request(140, 100)

            icon_lbl = Gtk.Label(label=icon)
            icon_lbl.set_markup(f'<span size="xx-large">{icon}</span>')
            card.pack_start(icon_lbl, False, False, 0)
            name_lbl = Gtk.Label(label=name)
            name_lbl.get_style_context().add_class("card-name")
            card.pack_start(name_lbl, False, False, 0)
            range_lbl = Gtk.Label(label=range_)
            range_lbl.get_style_context().add_class("card-desc")
            card.pack_start(range_lbl, False, False, 0)

            grid.attach(card, col, row, 1, 1)

        content.pack_start(grid, False, False, 0)

        info_label = Gtk.Label(
            label="Los sensores virtuales se usan desde los módulos de ciencia "
                  "(agro, seismic, env). Cada módulo genera datos sintéticos.")
        info_label.get_style_context().add_class("card-desc")
        info_label.set_xalign(0)
        info_label.set_line_wrap(True)
        info_label.set_margin_top(12)
        content.pack_start(info_label, False, False, 0)

    def _show_render(self):
        content = self._make_page("🖨️ IMPRESIÓN 3D")

        for mod in RENDER_MODULES:
            frame = Gtk.Frame(label=f' {mod["icon"]} {mod["name"]} ')
            frame.get_style_context().add_class("card")

            vb = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
            vb.set_margin_start(8)
            vb.set_margin_end(8)
            vb.set_margin_top(8)
            vb.set_margin_bottom(8)

            desc_lbl = Gtk.Label(label=mod["desc"])
            desc_lbl.get_style_context().add_class("card-desc")
            desc_lbl.set_xalign(0)
            vb.pack_start(desc_lbl, False, False, 0)

            pg = Gtk.Grid()
            pg.set_column_spacing(8)
            pg.set_row_spacing(4)
            entries = {}
            for idx, (label, default) in enumerate(mod["params"]):
                lbl = Gtk.Label(label=f"{label}:")
                lbl.get_style_context().add_class("param-label")
                lbl.set_xalign(1)
                pg.attach(lbl, 0, idx, 1, 1)
                entry = Gtk.Entry()
                entry.set_text(default)
                entry.set_width_chars(10)
                entry.get_style_context().add_class("param-entry")
                pg.attach(entry, 1, idx, 1, 1)
                entries[label] = entry
            vb.pack_start(pg, False, False, 0)

            out_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
            out_lbl = Gtk.Label(label="STL:")
            out_lbl.get_style_context().add_class("param-label")
            out_box.pack_start(out_lbl, False, False, 0)
            out_entry = Gtk.Entry()
            out_entry.set_text(f"/tmp/tritos_{mod['id']}.stl")
            out_entry.set_width_chars(28)
            out_entry.get_style_context().add_class("param-entry")
            out_box.pack_start(out_entry, False, False, 0)
            vb.pack_start(out_box, False, False, 0)

            gen_btn = Gtk.Button(label="▶ Generar STL")
            gen_btn.get_style_context().add_class("run-btn")
            gen_btn.connect("clicked", lambda w, m=mod, e=entries, o=out_entry:
                            self._exec_render(m, e, o))
            vb.pack_start(gen_btn, False, False, 0)

            result_tv = Gtk.TextView()
            result_tv.get_style_context().add_class("output-text")
            result_tv.set_editable(False)
            result_tv.set_monospace(True)
            result_tv.set_size_request(-1, 40)
            result_tv_sw = Gtk.ScrolledWindow()
            result_tv_sw.set_size_request(-1, 60)
            result_tv_sw.add(result_tv)
            vb.pack_start(result_tv_sw, False, False, 0)

            frame.add(vb)
            content.pack_start(frame, False, False, 0)

            entries["_result_tv"] = result_tv
            entries["_out_entry"] = out_entry

    def _exec_render(self, mod, entries, out_entry):
        cmd = [SCIENCE_BIN, "render", mod["id"]]
        for label in entries:
            if label.startswith("_"):
                continue
            val = entries[label].get_text().strip()
            if val:
                cmd.extend([f"-{label[0].lower()}", val])
        cmd.extend(["-o", out_entry.get_text().strip()])
        result_tv = entries["_result_tv"]
        result_tv.get_buffer().set_text(f"$ {' '.join(cmd)}\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            out = out_entry.get_text().strip()
            txt = r.stdout + r.stderr
            if os.path.exists(out):
                sz = os.path.getsize(out)
                txt += f"\nSTL generado: {out} ({sz:,} bytes)"
            result_tv.get_buffer().set_text(txt)
        except Exception as e:
            result_tv.get_buffer().set_text(f"Error: {e}")

    def _show_math(self):
        content = self._make_page("🧮 MATEMÁTICA")

        f1 = Gtk.Frame(label=" Decimal → Ternario ")
        f1.get_style_context().add_class("card")
        g1 = Gtk.Grid()
        g1.set_column_spacing(8)
        g1.set_row_spacing(6)
        g1.set_margin_start(8)
        g1.set_margin_end(8)
        g1.set_margin_top(8)
        g1.set_margin_bottom(8)
        f1.add(g1)

        lbl = Gtk.Label(label="Número:")
        lbl.get_style_context().add_class("param-label")
        lbl.set_xalign(1)
        g1.attach(lbl, 0, 0, 1, 1)
        self._math_dec_entry = Gtk.Entry()
        self._math_dec_entry.set_text("42")
        self._math_dec_entry.set_width_chars(12)
        self._math_dec_entry.get_style_context().add_class("param-entry")
        g1.attach(self._math_dec_entry, 1, 0, 1, 1)
        btn1 = Gtk.Button(label="Convertir")
        btn1.get_style_context().add_class("run-btn")
        btn1.connect("clicked", lambda w: self._calc_ternary())
        g1.attach(btn1, 2, 0, 1, 1)
        self._math_ternary_res = Gtk.Label()
        self._math_ternary_res.set_xalign(0)
        self._math_ternary_res.set_markup(
            '<span color="#7ee787">Resultado: —</span>')
        g1.attach(self._math_ternary_res, 0, 1, 3, 1)
        content.pack_start(f1, False, False, 0)

        f2 = Gtk.Frame(label=" Decimal → Base 60 (Babilonia) ")
        f2.get_style_context().add_class("card")
        g2 = Gtk.Grid()
        g2.set_column_spacing(8)
        g2.set_row_spacing(6)
        g2.set_margin_start(8)
        g2.set_margin_end(8)
        g2.set_margin_top(8)
        g2.set_margin_bottom(8)
        f2.add(g2)

        lbl2 = Gtk.Label(label="Número:")
        lbl2.get_style_context().add_class("param-label")
        lbl2.set_xalign(1)
        g2.attach(lbl2, 0, 0, 1, 1)
        self._math_b60_entry = Gtk.Entry()
        self._math_b60_entry.set_text("120")
        self._math_b60_entry.set_width_chars(12)
        self._math_b60_entry.get_style_context().add_class("param-entry")
        g2.attach(self._math_b60_entry, 1, 0, 1, 1)
        btn2 = Gtk.Button(label="Convertir")
        btn2.get_style_context().add_class("run-btn")
        btn2.connect("clicked", lambda w: self._calc_base60())
        g2.attach(btn2, 2, 0, 1, 1)
        self._math_b60_res = Gtk.Label()
        self._math_b60_res.set_xalign(0)
        self._math_b60_res.set_markup(
            '<span color="#7ee787">Resultado: —</span>')
        g2.attach(self._math_b60_res, 0, 1, 3, 1)
        content.pack_start(f2, False, False, 0)

        f3 = Gtk.Frame(label=" Calendario Maya — Fecha Actual ")
        f3.get_style_context().add_class("card")
        vb3 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        vb3.set_margin_start(8)
        vb3.set_margin_end(8)
        vb3.set_margin_top(8)
        vb3.set_margin_bottom(8)
        f3.add(vb3)

        now = datetime.datetime.now()
        doy = now.timetuple().tm_yday
        tzolkin_n = ((doy - 1) % 13) + 1
        tzolkin_names = [
            "Imix", "Ik", "Akbal", "Kan", "Chicchan", "Cimi", "Manik",
            "Lamat", "Muluk", "Ok", "Chuen", "Eb", "Ben", "Ix",
            "Men", "Cib", "Caban", "Etz'nab", "Cauac", "Ahau"]
        tzolkin_name = tzolkin_names[(doy - 1) % 20]
        haab_day = ((doy - 1) % 365) + 1
        lc_days = (now - datetime.datetime(2012, 12, 21)).days
        baktun = lc_days // 144000
        katun = (lc_days % 144000) // 7200
        tun = (lc_days % 7200) // 360
        uinal = (lc_days % 360) // 20
        kin = lc_days % 20

        maya_text = (
            f"  Fecha: {now.strftime('%d/%m/%Y %H:%M')}\n\n"
            f"  Tzolkin:        {tzolkin_n} {tzolkin_name} (día {((doy-1)%260)+1}/260)\n"
            f"  Haab:           día {haab_day}/365\n"
            f"  Cuenta Larga:   {lc_days} días desde 21/12/2012\n"
            f"  Long Count:     {baktun}.{katun}.{tun}.{uinal}.{kin}")
        info = Gtk.Label(label=maya_text)
        info.set_xalign(0)
        info.get_style_context().add_class("output-text")
        vb3.pack_start(info, False, False, 0)
        content.pack_start(f3, False, False, 0)

    def _calc_ternary(self):
        try:
            n = int(self._math_dec_entry.get_text().strip())
            if n == 0:
                result = "0"
            else:
                neg = n < 0
                n = abs(n)
                digs = []
                while n > 0:
                    digs.append(str(n % 3))
                    n //= 3
                result = "".join(reversed(digs))
                if neg:
                    result = "-" + result
            self._math_ternary_res.set_markup(
                f'<span color="#7ee787">Resultado: {result}</span>')
        except ValueError:
            self._math_ternary_res.set_markup(
                '<span color="#f85149">Error: ingresá un entero</span>')

    def _calc_base60(self):
        try:
            n = int(self._math_b60_entry.get_text().strip())
            if n == 0:
                result = "0:0"
            else:
                neg = n < 0
                n = abs(n)
                high = n // 60
                low = n % 60
                result = f"{high}:{low}"
                if neg:
                    result = "-" + result
            self._math_b60_res.set_markup(
                f'<span color="#7ee787">Resultado: {result}</span>')
        except ValueError:
            self._math_b60_res.set_markup(
                '<span color="#f85149">Error: ingresá un entero</span>')

    def _show_audio(self):
        content = self._make_page("🎵 AUDIO TERNARIO")

        f1 = Gtk.Frame(label=" Generar Tono WAV ")
        f1.get_style_context().add_class("card")
        g1 = Gtk.Grid()
        g1.set_column_spacing(8)
        g1.set_row_spacing(6)
        g1.set_margin_start(8)
        g1.set_margin_end(8)
        g1.set_margin_top(8)
        g1.set_margin_bottom(8)
        f1.add(g1)

        self._audio_entries = {}
        for idx, (lbl_text, default) in enumerate([
                ("Frecuencia (Hz)", "440"),
                ("Duración (s)", "2"),
                ("Salida WAV", "/tmp/tritos_tone.wav")]):
            lbl = Gtk.Label(label=f"{lbl_text}:")
            lbl.get_style_context().add_class("param-label")
            lbl.set_xalign(1)
            g1.attach(lbl, 0, idx, 1, 1)
            entry = Gtk.Entry()
            entry.set_text(default)
            entry.set_width_chars(18)
            entry.get_style_context().add_class("param-entry")
            g1.attach(entry, 1, idx, 1, 1)
            self._audio_entries[lbl_text] = entry

        gen_btn = Gtk.Button(label="▶ Generar Tono")
        gen_btn.get_style_context().add_class("run-btn")
        gen_btn.connect("clicked", lambda w: self._gen_tone())
        g1.attach(gen_btn, 2, 0, 1, 2)
        content.pack_start(f1, False, False, 0)

        f2 = Gtk.Frame(label=" Comprimir WAV ")
        f2.get_style_context().add_class("card")
        g2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        g2.set_margin_start(8)
        g2.set_margin_end(8)
        g2.set_margin_top(8)
        g2.set_margin_bottom(8)
        f2.add(g2)

        lbl = Gtk.Label(label="Archivo WAV:")
        lbl.get_style_context().add_class("param-label")
        g2.pack_start(lbl, False, False, 0)
        self._audio_wav_entry = Gtk.Entry()
        self._audio_wav_entry.set_text("/tmp/tritos_tone.wav")
        self._audio_wav_entry.set_width_chars(24)
        self._audio_wav_entry.get_style_context().add_class("param-entry")
        g2.pack_start(self._audio_wav_entry, False, False, 0)
        comp_btn = Gtk.Button(label="Comprimir")
        comp_btn.get_style_context().add_class("run-btn")
        comp_btn.connect("clicked", lambda w: self._compress_wav())
        g2.pack_start(comp_btn, False, False, 0)
        content.pack_start(f2, False, False, 0)

        of = Gtk.Frame(label=" Resultado ")
        of.get_style_context().add_class("card")
        self._audio_output = Gtk.TextView()
        self._audio_output.get_style_context().add_class("output-text")
        self._audio_output.set_editable(False)
        self._audio_output.set_monospace(True)
        sw = Gtk.ScrolledWindow()
        sw.add(self._audio_output)
        of.add(sw)
        content.pack_start(of, True, True, 0)

    def _gen_tone(self):
        try:
            freq = float(self._audio_entries["Frecuencia (Hz)"].get_text().strip())
            dur = float(self._audio_entries["Duración (s)"].get_text().strip())
            out = self._audio_entries["Salida WAV"].get_text().strip()
            sr = 8000
            n = int(sr * dur)
            self._audio_output.get_buffer().set_text(
                f"Generando tono {freq}Hz, {dur}s...\n")
            with wave.open(out, 'w') as w:
                w.setnchannels(1)
                w.setsampwidth(2)
                w.setframerate(sr)
                for i in range(n):
                    t = i / sr
                    val = int(16000 * math.sin(2 * math.pi * freq * t))
                    w.writeframes(struct.pack('<h', val))
            sz = os.path.getsize(out)
            self._audio_output.get_buffer().set_text(
                f"Tono generado: {out}\n"
                f"  Frecuencia: {freq} Hz\n"
                f"  Duración: {dur} s\n"
                f"  Tamaño: {sz:,} bytes\n"
                f"  Muestreo: {sr} Hz, 16-bit, mono")
        except Exception as e:
            self._audio_output.get_buffer().set_text(f"Error: {e}")

    def _compress_wav(self):
        wav_path = self._audio_wav_entry.get_text().strip()
        if not os.path.exists(wav_path):
            self._audio_output.get_buffer().set_text(
                f"Error: {wav_path} no existe")
            return
        cmd = [SCIENCE_BIN, "audio", "-i", wav_path]
        self._audio_output.get_buffer().set_text(f"$ {' '.join(cmd)}\n\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            self._audio_output.get_buffer().set_text(r.stdout + r.stderr)
        except Exception as e:
            self._audio_output.get_buffer().set_text(f"Error: {e}")

    def _show_files(self):
        content = self._make_page("📁 ARCHIVOS — QuipuFS")

        quipu_root = os.path.expanduser("~/.tritos/quipu")
        os.makedirs(quipu_root, exist_ok=True)
        self._files_cwd = quipu_root

        toolbar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        content.pack_start(toolbar, False, False, 0)

        for label, handler in [
            ("🔄 Actualizar", lambda w: self._files_refresh()),
            ("📄 Nuevo archivo", lambda w: self._files_new_file()),
            ("📁 Nuevo directorio", lambda w: self._files_new_dir())]:
            btn = Gtk.Button(label=label)
            btn.get_style_context().add_class("back-btn")
            btn.connect("clicked", handler)
            toolbar.pack_start(btn, False, False, 0)

        self._files_path_label = Gtk.Label()
        self._files_path_label.get_style_context().add_class("path-label")
        self._files_path_label.set_xalign(0)
        content.pack_start(self._files_path_label, False, False, 0)

        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self._files_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=1)
        scroll.add(self._files_box)
        content.pack_start(scroll, True, True, 0)

        self._files_refresh()

    def _files_refresh(self):
        for ch in self._files_box.get_children():
            self._files_box.remove(ch)

        path = self._files_cwd
        self._files_path_label.set_text(f"Ruta: {path}")

        base = os.path.expanduser("~/.tritos/quipu")
        if path != base:
            row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
            up_btn = Gtk.Button(label="📂 ..")
            up_btn.get_style_context().add_class("file-item")
            up_btn.connect("clicked", lambda w: self._files_go_up())
            row.pack_start(up_btn, True, True, 0)
            self._files_box.pack_start(row, False, False, 0)

        try:
            entries = sorted(os.listdir(path))
        except PermissionError:
            entries = []

        for name in entries:
            full = os.path.join(path, name)
            row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)

            if os.path.isdir(full):
                btn = Gtk.Button(label=f"📂 {name}/")
                btn.get_style_context().add_class("file-item")
                btn.set_halign(Gtk.Align.FILL)
                btn.set_hexpand(True)
                btn.connect("clicked", lambda w, p=full: self._files_enter(p))
                row.pack_start(btn, True, True, 0)
            else:
                sz = os.path.getsize(full)
                btn = Gtk.Button(label=f"📋 {name}  ({sz:,} bytes)")
                btn.get_style_context().add_class("file-item")
                btn.set_halign(Gtk.Align.FILL)
                btn.set_hexpand(True)
                btn.connect("clicked", lambda w, p=full: self._files_view(p))
                row.pack_start(btn, True, True, 0)

            del_btn = Gtk.Button(label="🗑️")
            del_btn.get_style_context().add_class("del-btn")
            del_btn.connect("clicked", lambda w, p=full: self._files_delete(p))
            row.pack_end(del_btn, False, False, 0)

            self._files_box.pack_start(row, False, False, 0)

        if not entries:
            lbl = Gtk.Label(label="  (directorio vacío)")
            lbl.get_style_context().add_class("card-desc")
            self._files_box.pack_start(lbl, False, False, 0)

        self._files_box.show_all()

    def _files_enter(self, path):
        self._files_cwd = path
        self._files_refresh()

    def _files_go_up(self):
        self._files_cwd = os.path.dirname(self._files_cwd)
        self._files_refresh()

    def _files_new_file(self):
        dialog = Gtk.Dialog(
            title="Nuevo archivo", parent=self,
            flags=Gtk.DialogFlags.MODAL,
            buttons=("Crear", Gtk.ResponseType.OK,
                     "Cancelar", Gtk.ResponseType.CANCEL))
        box = dialog.get_content_area()
        box.set_spacing(6)
        box.set_margin_start(10)
        box.set_margin_end(10)
        box.set_margin_top(10)
        box.add(Gtk.Label(label="Nombre:"))
        name_entry = Gtk.Entry()
        name_entry.set_text("nuevo.txt")
        box.add(name_entry)
        box.add(Gtk.Label(label="Contenido:"))
        content_entry = Gtk.Entry()
        content_entry.set_text("hola ternario")
        box.add(content_entry)
        dialog.show_all()
        resp = dialog.run()
        if resp == Gtk.ResponseType.OK:
            name = name_entry.get_text().strip()
            if name:
                with open(os.path.join(self._files_cwd, name), 'w') as f:
                    f.write(content_entry.get_text())
                self._files_refresh()
        dialog.destroy()

    def _files_new_dir(self):
        dialog = Gtk.Dialog(
            title="Nuevo directorio", parent=self,
            flags=Gtk.DialogFlags.MODAL,
            buttons=("Crear", Gtk.ResponseType.OK,
                     "Cancelar", Gtk.ResponseType.CANCEL))
        box = dialog.get_content_area()
        box.set_spacing(6)
        box.set_margin_start(10)
        box.set_margin_end(10)
        box.set_margin_top(10)
        box.add(Gtk.Label(label="Nombre:"))
        name_entry = Gtk.Entry()
        name_entry.set_text("nueva_carpeta")
        box.add(name_entry)
        dialog.show_all()
        resp = dialog.run()
        if resp == Gtk.ResponseType.OK:
            name = name_entry.get_text().strip()
            if name:
                os.makedirs(os.path.join(self._files_cwd, name), exist_ok=True)
                self._files_refresh()
        dialog.destroy()

    def _files_view(self, path):
        try:
            with open(path, 'r') as f:
                text = f.read(8192)
        except Exception:
            text = "(no se puede leer el archivo)"
        dialog = Gtk.Dialog(
            title=os.path.basename(path), parent=self,
            flags=Gtk.DialogFlags.MODAL,
            buttons=("Cerrar", Gtk.ResponseType.CLOSE))
        dialog.set_default_size(500, 350)
        tv = Gtk.TextView()
        tv.set_editable(False)
        tv.set_monospace(True)
        tv.get_buffer().set_text(text)
        tv.get_style_context().add_class("output-text")
        scroll = Gtk.ScrolledWindow()
        scroll.add(tv)
        dialog.get_content_area().pack_start(scroll, True, True, 0)
        dialog.show_all()
        dialog.run()
        dialog.destroy()

    def _files_delete(self, path):
        dialog = Gtk.Dialog(
            title="Eliminar", parent=self,
            flags=Gtk.DialogFlags.MODAL,
            buttons=("Eliminar", Gtk.ResponseType.OK,
                     "Cancelar", Gtk.ResponseType.CANCEL))
        dialog.get_content_area().add(
            Gtk.Label(label=f"¿Eliminar?\n{os.path.basename(path)}"))
        dialog.show_all()
        resp = dialog.run()
        if resp == Gtk.ResponseType.OK:
            try:
                if os.path.isdir(path):
                    os.rmdir(path)
                else:
                    os.remove(path)
                self._files_refresh()
            except Exception as e:
                err = Gtk.MessageDialog(
                    transient_for=dialog, message_type=Gtk.MessageType.ERROR,
                    buttons=Gtk.ButtonsType.OK, text=f"Error: {e}")
                err.run()
                err.destroy()
        dialog.destroy()

    def _show_system(self):
        content = self._make_page("⚙️ SISTEMA")

        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        content.pack_start(btn_box, False, False, 0)

        for label, cmd in [
            ("💻 CPU", "lscpu"),
            ("💾 Memoria", "free -h"),
            ("📀 Disco", "df -h"),
            ("⏱️ Uptime", "uptime")]:
            btn = Gtk.Button(label=label)
            btn.get_style_context().add_class("gen-btn")
            btn.connect("clicked", lambda w, c=cmd: self._sys_run(c))
            btn_box.pack_start(btn, False, False, 0)

        of = Gtk.Frame(label=" Resultado ")
        of.get_style_context().add_class("card")
        self._sys_output = Gtk.TextView()
        self._sys_output.get_style_context().add_class("output-text")
        self._sys_output.set_editable(False)
        self._sys_output.set_monospace(True)
        sw = Gtk.ScrolledWindow()
        sw.add(self._sys_output)
        of.add(sw)
        content.pack_start(of, True, True, 0)

        self._sys_run("uname -a")

    def _sys_run(self, cmd):
        try:
            r = subprocess.run(cmd, shell=True, capture_output=True, text=True,
                               timeout=10)
            self._sys_output.get_buffer().set_text(
                f"$ {cmd}\n\n{r.stdout}{r.stderr}")
        except Exception as e:
            self._sys_output.get_buffer().set_text(f"Error: {e}")

    def _show_crypto(self):
        content = self._make_page("🔐 SEGURIDAD TERNARIA")

        f1 = Gtk.Frame(label=" RSA Key Generation ")
        f1.get_style_context().add_class("card")
        g1 = Gtk.Grid()
        g1.set_column_spacing(8)
        g1.set_row_spacing(6)
        g1.set_margin_start(8)
        g1.set_margin_end(8)
        g1.set_margin_top(8)
        g1.set_margin_bottom(8)
        f1.add(g1)

        lbl = Gtk.Label(label="Bits:")
        lbl.get_style_context().add_class("param-label")
        lbl.set_xalign(1)
        g1.attach(lbl, 0, 0, 1, 1)
        self._crypto_bits = Gtk.Entry()
        self._crypto_bits.set_text("64")
        self._crypto_bits.set_width_chars(8)
        self._crypto_bits.get_style_context().add_class("param-entry")
        g1.attach(self._crypto_bits, 1, 0, 1, 1)
        gen_btn = Gtk.Button(label="Generar Claves")
        gen_btn.get_style_context().add_class("run-btn")
        gen_btn.connect("clicked", lambda w: self._crypto_gen())
        g1.attach(gen_btn, 2, 0, 1, 1)
        content.pack_start(f1, False, False, 0)

        f2 = Gtk.Frame(label=" Encrypt Message ")
        f2.get_style_context().add_class("card")
        g2 = Gtk.Grid()
        g2.set_column_spacing(8)
        g2.set_row_spacing(6)
        g2.set_margin_start(8)
        g2.set_margin_end(8)
        g2.set_margin_top(8)
        g2.set_margin_bottom(8)
        f2.add(g2)

        lbl2 = Gtk.Label(label="Mensaje:")
        lbl2.get_style_context().add_class("param-label")
        lbl2.set_xalign(1)
        g2.attach(lbl2, 0, 0, 1, 1)
        self._crypto_msg = Gtk.Entry()
        self._crypto_msg.set_text("HOLA TERNARIO")
        self._crypto_msg.set_width_chars(28)
        self._crypto_msg.get_style_context().add_class("param-entry")
        g2.attach(self._crypto_msg, 1, 0, 1, 1)
        enc_btn = Gtk.Button(label="Cifrar")
        enc_btn.get_style_context().add_class("run-btn")
        enc_btn.connect("clicked", lambda w: self._crypto_encrypt())
        g2.attach(enc_btn, 2, 0, 1, 1)
        content.pack_start(f2, False, False, 0)

        of = Gtk.Frame(label=" Resultado ")
        of.get_style_context().add_class("card")
        self._crypto_output = Gtk.TextView()
        self._crypto_output.get_style_context().add_class("output-text")
        self._crypto_output.set_editable(False)
        self._crypto_output.set_monospace(True)
        sw = Gtk.ScrolledWindow()
        sw.add(self._crypto_output)
        of.add(sw)
        content.pack_start(of, True, True, 0)

    def _crypto_gen(self):
        bits = self._crypto_bits.get_text().strip()
        cmd = [SCIENCE_BIN, "crypto", "-b", bits]
        self._crypto_output.get_buffer().set_text(
            f"Generando claves RSA ({bits} bits)...\n$ {' '.join(cmd)}\n\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            self._crypto_output.get_buffer().set_text(r.stdout + r.stderr)
        except Exception as e:
            self._crypto_output.get_buffer().set_text(f"Error: {e}")

    def _crypto_encrypt(self):
        msg = self._crypto_msg.get_text().strip()
        try:
            msg_bytes = msg.encode('utf-8')
            msg_int = int.from_bytes(msg_bytes, 'big')
            self._crypto_output.get_buffer().set_text(
                f"Mensaje: \"{msg}\"\n"
                f"Bytes:   {msg_bytes.hex()}\n"
                f"Entero:  {msg_int}\n\n"
                f"Usá 'Generar Claves' primero para cifrar con RSA ternario.")
        except Exception as e:
            self._crypto_output.get_buffer().set_text(f"Error: {e}")

    def _show_terminal(self):
        content = self._make_page("🔢 TERMINAL BINARIA / TERNARIA")

        # Command input
        cmd_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        cmd_box.set_margin_start(12)
        cmd_box.set_margin_end(12)
        cmd_box.set_margin_top(8)
        content.pack_start(cmd_box, False, False, 0)

        lbl = Gtk.Label(label="tritos₃>")
        lbl.get_style_context().add_class("param-label")
        cmd_box.pack_start(lbl, False, False, 0)

        self._term_entry = Gtk.Entry()
        self._term_entry.set_hexpand(True)
        self._term_entry.set_placeholder_text("Escribí un comando: dec 42, 42 + 10, bal 255, truth, maya...")
        self._term_entry.get_style_context().add_class("param-entry")
        self._term_entry.connect("activate", lambda w: self._term_run())
        cmd_box.pack_start(self._term_entry, True, True, 0)

        run_btn = Gtk.Button(label="▶ Ejecutar")
        run_btn.get_style_context().add_class("run-btn")
        run_btn.connect("clicked", lambda w: self._term_run())
        cmd_box.pack_start(run_btn, False, False, 0)

        # Quick actions
        qa_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        qa_box.set_margin_start(12)
        qa_box.set_margin_end(12)
        qa_box.set_margin_top(6)
        content.pack_start(qa_box, False, False, 0)

        qa_label = Gtk.Label(label="Rápidas:")
        qa_label.get_style_context().add_class("param-label")
        qa_box.pack_start(qa_label, False, False, 0)

        for label, cmd in [("Dec→Todo", "dec "), ("Bal", "bal "), ("Base60", "b60 "),
                           ("Truth", "truth"), ("Maya", "maya"), ("Persa", "persa"),
                           ("Azteca", "azteca")]:
            btn = Gtk.Button(label=label)
            btn.get_style_context().add_class("term-quick-btn")
            btn.connect("clicked", lambda w, c=cmd: self._term_insert(c))
            qa_box.pack_start(btn, False, False, 0)

        # Calculator buttons
        calc_frame = Gtk.Frame(label=" Calculadora Ternaria ")
        calc_frame.get_style_context().add_class("card")
        calc_frame.set_margin_start(12)
        calc_frame.set_margin_end(12)
        calc_frame.set_margin_top(6)
        content.pack_start(calc_frame, False, False, 0)

        calc_grid = Gtk.Grid()
        calc_grid.set_column_spacing(4)
        calc_grid.set_row_spacing(4)
        calc_grid.set_margin_start(8)
        calc_grid.set_margin_end(8)
        calc_grid.set_margin_top(8)
        calc_frame.add(calc_grid)

        ops = [("+", "+"), ("-", "-"), ("×", "*"), ("÷", "/"), ("%", "%"), ("^", "^")]
        for i, (sym, op) in enumerate(ops):
            btn = Gtk.Button(label=sym)
            btn.get_style_context().add_class("term-calc-btn")
            btn.connect("clicked", lambda w, o=op: self._term_insert(f" {o} "))
            calc_grid.attach(btn, i, 0, 1, 1)

        logic_ops = [("AND", "and "), ("OR", "or "), ("XOR", "xor "),
                     ("NOT", "not "), ("→", "implies "), ("↔", "iff ")]
        for i, (sym, cmd) in enumerate(logic_ops):
            btn = Gtk.Button(label=sym)
            btn.get_style_context().add_class("term-calc-btn")
            btn.connect("clicked", lambda w, c=cmd: self._term_insert(c))
            calc_grid.attach(btn, i, 1, 1, 1)

        # Output
        out_frame = Gtk.Frame(label=" Salida ")
        out_frame.get_style_context().add_class("card")
        out_frame.set_margin_start(12)
        out_frame.set_margin_end(12)
        out_frame.set_margin_top(6)
        content.pack_start(out_frame, True, True, 0)

        out_sw = Gtk.ScrolledWindow()
        out_sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        out_frame.add(out_sw)

        self._term_output = Gtk.TextView()
        self._term_output.get_style_context().add_class("output-text")
        self._term_output.set_editable(False)
        self._term_output.set_monospace(True)
        self._term_output.set_left_margin(8)
        self._term_output.set_top_margin(8)
        self._term_output.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        out_sw.add(self._term_output)

    def _term_insert(self, text):
        self._term_entry.insert_text(text, len(text), self._term_entry.get_position())

    def _term_run(self):
        cmd = self._term_entry.get_text().strip()
        if not cmd:
            return
        buf = self._term_output.get_buffer()
        end_iter = buf.get_end_iter()
        buf.insert(end_iter, f"\n  tritos₃> {cmd}\n")

        script_dir = os.path.dirname(os.path.abspath(__file__))
        terminal_bin = os.path.join(script_dir, "tritos_terminal")
        if not os.path.exists(terminal_bin):
            terminal_bin = os.path.join(script_dir, "bin", "tritos_terminal")

        try:
            r = subprocess.run(
                [terminal_bin, cmd],
                capture_output=True, text=True, timeout=10,
                cwd=script_dir)
            output = r.stdout + r.stderr
            if output.strip():
                end_iter = buf.get_end_iter()
                buf.insert(end_iter, output)
        except FileNotFoundError:
            end_iter = buf.get_end_iter()
            buf.insert(end_iter,
                "  Error: tritos_terminal no encontrado.\n"
                "  Compilá con: gcc -o tritos_terminal src/ternary_terminal.c -lm\n")
        except subprocess.TimeoutExpired:
            end_iter = buf.get_end_iter()
            buf.insert(end_iter, "  Timeout (>10s)\n")
        except Exception as e:
            end_iter = buf.get_end_iter()
            buf.insert(end_iter, f"  Error: {e}\n")

        self._term_entry.set_text("")
        # Scroll to end
        end_iter = buf.get_end_iter()
        self._term_output.scroll_mark_onscreen(buf.create_mark(None, end_iter, False))

    def _show_security(self):
        content = self._make_page("🔐 SEGURIDAD")

        f1 = Gtk.Frame(label=" Security Scan ")
        f1.get_style_context().add_class("card")
        vb1 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        vb1.set_margin_start(8)
        vb1.set_margin_end(8)
        vb1.set_margin_top(8)
        vb1.set_margin_bottom(8)
        f1.add(vb1)

        lbl = Gtk.Label(label="URL o texto a escanear:")
        lbl.get_style_context().add_class("param-label")
        lbl.set_xalign(0)
        vb1.pack_start(lbl, False, False, 0)

        row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        self._sec_scan_entry = Gtk.Entry()
        self._sec_scan_entry.set_hexpand(True)
        self._sec_scan_entry.set_width_chars(40)
        self._sec_scan_entry.get_style_context().add_class("param-entry")
        row.pack_start(self._sec_scan_entry, True, True, 0)
        scan_btn = Gtk.Button(label="▶ Escanear")
        scan_btn.get_style_context().add_class("run-btn")
        scan_btn.connect("clicked", lambda w: self._security_scan())
        row.pack_start(scan_btn, False, False, 0)
        vb1.pack_start(row, False, False, 0)
        content.pack_start(f1, False, False, 0)

        f2 = Gtk.Frame(label=" Security Status ")
        f2.get_style_context().add_class("card")
        vb2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        vb2.set_margin_start(8)
        vb2.set_margin_end(8)
        vb2.set_margin_top(8)
        vb2.set_margin_bottom(8)
        f2.add(vb2)
        status_btn = Gtk.Button(label="▶ Ver Estado")
        status_btn.get_style_context().add_class("run-btn")
        status_btn.connect("clicked", lambda w: self._security_status())
        vb2.pack_start(status_btn, False, False, 0)
        content.pack_start(f2, False, False, 0)

        f3 = Gtk.Frame(label=" Resultado ")
        f3.get_style_context().add_class("card")
        self._sec_output = Gtk.TextView()
        self._sec_output.get_style_context().add_class("output-text")
        self._sec_output.set_editable(False)
        self._sec_output.set_monospace(True)
        self._sec_output.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw.add(self._sec_output)
        f3.add(sw)
        content.pack_start(f3, True, True, 0)

    def _security_scan(self):
        url = self._sec_scan_entry.get_text().strip()
        if not url:
            self._sec_output.get_buffer().set_text("Ingresá una URL o texto.")
            return
        cmd = [SCIENCE_BIN, "security-scan", url]
        self._sec_output.get_buffer().set_text(f"$ {' '.join(cmd)}\n\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            self._sec_output.get_buffer().set_text(r.stdout + r.stderr)
        except subprocess.TimeoutExpired:
            self._sec_output.get_buffer().set_text("Timeout después de 30s")
        except Exception as e:
            self._sec_output.get_buffer().set_text(f"Error: {e}")

    def _security_status(self):
        cmd = [SCIENCE_BIN, "security-status"]
        self._sec_output.get_buffer().set_text(f"$ {' '.join(cmd)}\n\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
            self._sec_output.get_buffer().set_text(r.stdout + r.stderr)
        except Exception as e:
            self._sec_output.get_buffer().set_text(f"Error: {e}")

    def _show_firewall(self):
        content = self._make_page("🛡️ FIREWALL TERNARIO")

        f1 = Gtk.Frame(label=" Firewall Status ")
        f1.get_style_context().add_class("card")
        vb1 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        vb1.set_margin_start(8)
        vb1.set_margin_end(8)
        vb1.set_margin_top(8)
        vb1.set_margin_bottom(8)
        f1.add(vb1)

        info_lbl = Gtk.Label(label="Firewall ternario: 3 estados — ALLOW / BLOCK / ANALYZE")
        info_lbl.get_style_context().add_class("card-desc")
        info_lbl.set_xalign(0)
        vb1.pack_start(info_lbl, False, False, 0)

        btn_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        rules_btn = Gtk.Button(label="▶ Ver Reglas")
        rules_btn.get_style_context().add_class("run-btn")
        rules_btn.connect("clicked", lambda w: self._firewall_rules())
        btn_row.pack_start(rules_btn, False, False, 0)
        vb1.pack_start(btn_row, False, False, 0)
        content.pack_start(f1, False, False, 0)

        of = Gtk.Frame(label=" Resultado ")
        of.get_style_context().add_class("card")
        self._fw_output = Gtk.TextView()
        self._fw_output.get_style_context().add_class("output-text")
        self._fw_output.set_editable(False)
        self._fw_output.set_monospace(True)
        self._fw_output.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw.add(self._fw_output)
        of.add(sw)
        content.pack_start(of, True, True, 0)

    def _firewall_rules(self):
        cmd = [SCIENCE_BIN, "security-status"]
        self._fw_output.get_buffer().set_text(f"$ {' '.join(cmd)}\n\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
            self._fw_output.get_buffer().set_text(r.stdout + r.stderr)
        except Exception as e:
            self._fw_output.get_buffer().set_text(f"Error: {e}")

    def _show_parental(self):
        content = self._make_page("👶 CONTROL PARENTAL")

        f1 = Gtk.Frame(label=" Verificar URL ")
        f1.get_style_context().add_class("card")
        g1 = Gtk.Grid()
        g1.set_column_spacing(8)
        g1.set_row_spacing(6)
        g1.set_margin_start(8)
        g1.set_margin_end(8)
        g1.set_margin_top(8)
        g1.set_margin_bottom(8)
        f1.add(g1)

        lbl = Gtk.Label(label="URL:")
        lbl.get_style_context().add_class("param-label")
        lbl.set_xalign(1)
        g1.attach(lbl, 0, 0, 1, 1)
        self._par_url = Gtk.Entry()
        self._par_url.set_text("https://")
        self._par_url.set_width_chars(36)
        self._par_url.get_style_context().add_class("param-entry")
        g1.attach(self._par_url, 1, 0, 1, 1)

        lbl2 = Gtk.Label(label="Nivel:")
        lbl2.get_style_context().add_class("param-label")
        lbl2.set_xalign(1)
        g1.attach(lbl2, 0, 1, 1, 1)
        self._par_level = Gtk.ComboBoxText()
        for i in range(4):
            self._par_level.append_text(str(i))
        self._par_level.set_active(1)
        g1.attach(self._par_level, 1, 1, 1, 1)

        check_btn = Gtk.Button(label="▶ Verificar")
        check_btn.get_style_context().add_class("run-btn")
        check_btn.connect("clicked", lambda w: self._parental_check())
        g1.attach(check_btn, 2, 0, 1, 2)
        content.pack_start(f1, False, False, 0)

        of = Gtk.Frame(label=" Resultado ")
        of.get_style_context().add_class("card")
        self._par_output = Gtk.TextView()
        self._par_output.get_style_context().add_class("output-text")
        self._par_output.set_editable(False)
        self._par_output.set_monospace(True)
        self._par_output.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw.add(self._par_output)
        of.add(sw)
        content.pack_start(of, True, True, 0)

    def _parental_check(self):
        url = self._par_url.get_text().strip()
        level = self._par_level.get_active_text()
        if not url:
            self._par_output.get_buffer().set_text("Ingresá una URL.")
            return
        cmd = [SCIENCE_BIN, "security-scan", url, "-l", level]
        self._par_output.get_buffer().set_text(f"$ {' '.join(cmd)}\n\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            self._par_output.get_buffer().set_text(r.stdout + r.stderr)
        except subprocess.TimeoutExpired:
            self._par_output.get_buffer().set_text("Timeout después de 30s")
        except Exception as e:
            self._par_output.get_buffer().set_text(f"Error: {e}")

    def _show_ai(self):
        content = self._make_page("🧠 IA TERNARIA")

        self._ai_tritos_bin = os.path.join(SCRIPT_DIR, "tritos_ai")
        if not os.path.exists(self._ai_tritos_bin):
            self._ai_tritos_bin = os.path.join(SCRIPT_DIR, "bin", "tritos_ai")

        # Chat frame
        chat_frame = Gtk.Frame(label=" Chat con IA Ternaria ")
        chat_frame.get_style_context().add_class("card")
        chat_frame.set_margin_start(12)
        chat_frame.set_margin_end(12)
        chat_frame.set_margin_top(8)
        content.pack_start(chat_frame, True, True, 0)

        chat_vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        chat_vbox.set_margin_start(8)
        chat_vbox.set_margin_end(8)
        chat_vbox.set_margin_top(8)
        chat_frame.add(chat_vbox)

        chat_sw = Gtk.ScrolledWindow()
        chat_sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        chat_sw.set_min_content_height(200)
        chat_vbox.pack_start(chat_sw, True, True, 0)

        self._ai_chat_output = Gtk.TextView()
        self._ai_chat_output.get_style_context().add_class("output-text")
        self._ai_chat_output.set_editable(False)
        self._ai_chat_output.set_monospace(True)
        self._ai_chat_output.set_left_margin(8)
        self._ai_chat_output.set_top_margin(8)
        self._ai_chat_output.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        chat_sw.add(self._ai_chat_output)

        # Welcome message
        buf = self._ai_chat_output.get_buffer()
        buf.set_text(
            "  🧠 TRITOS IA v1.0 — Motor de Razonamiento Ternario\n"
            "  ─────────────────────────────────────────────────\n"
            "  Lógica: ⊕ (+1) = Certeza  |  0 = Incierto  |  ⊖ (-1) = Descartado\n\n"
            "  Comandos:\n"
            "    chat <msg>       Chat libre con IA\n"
            "    paper <título>   Analizar paper de investigación\n"
            "    sensors          Análisis de sensores demo\n"
            "    hypothesis <txt> Evaluar hipótesis\n"
            "    analyze <text>   Clasificar sentimiento\n"
            "    demo             Demo completa\n\n"
        )

        # Input
        input_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        chat_vbox.pack_start(input_box, False, False, 0)

        self._ai_entry = Gtk.Entry()
        self._ai_entry.set_hexpand(True)
        self._ai_entry.set_placeholder_text("Escribí un comando: chat, paper, sensors, hypothesis...")
        self._ai_entry.get_style_context().add_class("param-entry")
        self._ai_entry.connect("activate", lambda w: self._ai_run())
        input_box.pack_start(self._ai_entry, True, True, 0)

        run_btn = Gtk.Button(label="▶ Enviar")
        run_btn.get_style_context().add_class("run-btn")
        run_btn.connect("clicked", lambda w: self._ai_run())
        input_box.pack_start(run_btn, False, False, 0)

        # Quick actions
        qa_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        qa_box.set_margin_top(4)
        chat_vbox.pack_start(qa_box, False, False, 0)

        for label, cmd in [("💬 Chat", "chat hola"), ("📄 Paper", "paper ternary IoT sensor"),
                           ("📡 Sensores", "sensors"), ("🔬 Hipótesis", "hypothesis Los sistemas ternarios son superiores"),
                           ("📊 Demo", "demo")]:
            btn = Gtk.Button(label=label)
            btn.get_style_context().add_class("term-quick-btn")
            btn.connect("clicked", lambda w, c=cmd: self._ai_entry.set_text(c) or self._ai_run())
            qa_box.pack_start(btn, False, False, 0)

    def _ai_run(self):
        cmd = self._ai_entry.get_text().strip()
        if not cmd:
            return
        buf = self._ai_chat_output.get_buffer()
        end_iter = buf.get_end_iter()
        buf.insert(end_iter, f"\n  Tú: {cmd}\n")

        try:
            r = subprocess.run(
                [self._ai_tritos_bin] + cmd.split(),
                capture_output=True, text=True, timeout=10)
            output = r.stdout + r.stderr
            if output.strip():
                end_iter = buf.get_end_iter()
                buf.insert(end_iter, output)
        except FileNotFoundError:
            end_iter = buf.get_end_iter()
            buf.insert(end_iter,
                "  ⚠️ tritos_ai no encontrado. Compilá con:\n"
                "  gcc -o tritos_ai src/ternary_ai.c -lm\n")
        except Exception as e:
            end_iter = buf.get_end_iter()
            buf.insert(end_iter, f"  Error: {e}\n")

        self._ai_entry.set_text("")
        end_iter = buf.get_end_iter()
        self._ai_chat_output.scroll_mark_onscreen(buf.create_mark(None, end_iter, False))

    def _show_browser(self):
        content = self._make_page("🌐 NAVEGADOR WEB TRITOS")

        # Launch WebKit browser
        launch_frame = Gtk.Frame(label=" Navegador Web Completo (WebKit) ")
        launch_frame.get_style_context().add_class("card")
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        vbox.set_margin_start(12)
        vbox.set_margin_end(12)
        vbox.set_margin_top(8)
        launch_frame.add(vbox)

        info = Gtk.Label()
        info.set_xalign(0)
        info.set_markup(
            '<span color="#c9d1d9">Navegador web completo con soporte JavaScript, '
            'pestañas, bookmarks, y motor WebKit. Navegá a cualquier sitio.</span>')
        info.set_line_wrap(True)
        vbox.pack_start(info, False, False, 0)

        url_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        lbl = Gtk.Label(label="URL:")
        lbl.get_style_context().add_class("param-label")
        url_row.pack_start(lbl, False, False, 0)
        self._br_url = Gtk.Entry()
        self._br_url.set_hexpand(True)
        self._br_url.set_width_chars(50)
        self._br_url.set_text("https://es.wikipedia.org")
        self._br_url.get_style_context().add_class("param-entry")
        url_row.pack_start(self._br_url, True, True, 0)
        vbox.pack_start(url_row, False, False, 0)

        btn_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        nav_btn = Gtk.Button(label="🌐 Abrir Navegador Web")
        nav_btn.get_style_context().add_class("run-btn")
        nav_btn.connect("clicked", lambda w: self._launch_webkit_browser())
        btn_row.pack_start(nav_btn, False, False, 0)
        vbox.pack_start(btn_row, False, False, 0)
        content.pack_start(launch_frame, False, False, 0)

        # TUI Browser for terminal-style
        tui_frame = Gtk.Frame(label=" Navegador TUI (Terminal) ")
        tui_frame.get_style_context().add_class("card")
        tui_vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        tui_vbox.set_margin_start(12)
        tui_vbox.set_margin_end(12)
        tui_vbox.set_margin_top(8)
        tui_frame.add(tui_vbox)

        info2 = Gtk.Label()
        info2.set_xalign(0)
        info2.set_markup(
            '<span color="#c9d1d9">Navegador estilo lynx para terminal. '
            'Links numerados, navegación con teclado.</span>')
        info2.set_line_wrap(True)
        tui_vbox.pack_start(info2, False, False, 0)

        tui_url_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        lbl2 = Gtk.Label(label="URL:")
        lbl2.get_style_context().add_class("param-label")
        tui_url_row.pack_start(lbl2, False, False, 0)
        self._tui_url = Gtk.Entry()
        self._tui_url.set_hexpand(True)
        self._tui_url.set_width_chars(50)
        self._tui_url.set_text("https://es.wikipedia.org")
        self._tui_url.get_style_context().add_class("param-entry")
        tui_url_row.pack_start(self._tui_url, True, True, 0)
        tui_vbox.pack_start(tui_url_row, False, False, 0)

        tui_btn = Gtk.Button(label="⌨️ Abrir en Terminal")
        tui_btn.get_style_context().add_class("run-btn")
        tui_btn.connect("clicked", lambda w: self._launch_tui_browser())
        tui_vbox.pack_start(tui_btn, False, False, 0)
        content.pack_start(tui_frame, False, False, 0)

        # Ternary analysis
        analysis_frame = Gtk.Frame(label=" Análisis Ternario de Página ")
        analysis_frame.get_style_context().add_class("card")
        ana_vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        ana_vbox.set_margin_start(12)
        ana_vbox.set_margin_end(12)
        ana_vbox.set_margin_top(8)
        analysis_frame.add(ana_vbox)

        ana_url_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        lbl3 = Gtk.Label(label="URL:")
        lbl3.get_style_context().add_class("param-label")
        ana_url_row.pack_start(lbl3, False, False, 0)
        self._ana_url = Gtk.Entry()
        self._ana_url.set_hexpand(True)
        self._ana_url.set_width_chars(50)
        self._ana_url.get_style_context().add_class("param-entry")
        ana_url_row.pack_start(self._ana_url, True, True, 0)
        ana_vbox.pack_start(ana_url_row, False, False, 0)

        ana_btn = Gtk.Button(label="🔍 Analizar con ternary-browser")
        ana_btn.get_style_context().add_class("run-btn")
        ana_btn.connect("clicked", lambda w: self._analyze_url())
        ana_vbox.pack_start(ana_btn, False, False, 0)

        self._ana_output = Gtk.TextView()
        self._ana_output.get_style_context().add_class("output-text")
        self._ana_output.set_editable(False)
        self._ana_output.set_monospace(True)
        self._ana_output.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        sw2 = Gtk.ScrolledWindow()
        sw2.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw2.set_size_request(-1, 200)
        sw2.add(self._ana_output)
        ana_vbox.pack_start(sw2, True, True, 0)
        content.pack_start(analysis_frame, True, True, 0)

    def _launch_webkit_browser(self):
        url = self._br_url.get_text().strip() or "https://es.wikipedia.org"
        script_dir = os.path.dirname(os.path.abspath(__file__))
        browser_script = os.path.join(script_dir, "tritos_browser.py")
        try:
            subprocess.Popen(
                [sys.executable, browser_script, url],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL)
        except Exception as e:
            self._ana_output.get_buffer().set_text(f"Error: {e}")

    def _launch_tui_browser(self):
        url = self._tui_url.get_text().strip() or "https://es.wikipedia.org"
        script_dir = os.path.dirname(os.path.abspath(__file__))
        browser_bin = os.path.join(script_dir, "tritos_browser")
        if not os.path.exists(browser_bin):
            browser_bin = os.path.join(script_dir, "output", "tak-userspace", "tritos_browser")
        if not os.path.exists(browser_bin):
            self._ana_output.get_buffer().set_text(
                "Error: tritos_browser no encontrado. Compilá con:\n"
                "gcc -o tritos_browser src/tui_browser.c -O2")
            return
        try:
            subprocess.Popen(
                [browser_bin, url],
                stdout=sys.stdout, stderr=sys.stderr,
                stdin=sys.stdin)
        except Exception as e:
            self._ana_output.get_buffer().set_text(f"Error: {e}")

    def _analyze_url(self):
        url = self._ana_url.get_text().strip()
        if not url:
            self._ana_output.get_buffer().set_text("Ingresá una URL.")
            return
        script_dir = os.path.dirname(os.path.abspath(__file__))
        tritos_bin = os.path.join(script_dir, "tritos")
        if not os.path.exists(tritos_bin):
            tritos_bin = os.path.join(script_dir, "output", "tak-userspace", "tritos")
        try:
            result = subprocess.run(
                [tritos_bin, "-c", f"ternary-browser --dump --encode --compress {url}"],
                capture_output=True, text=True, timeout=30)
            output = result.stdout + result.stderr
            self._ana_output.get_buffer().set_text(output[:5000])
        except subprocess.TimeoutExpired:
            self._ana_output.get_buffer().set_text("Timeout: la página tardó demasiado.")
        except Exception as e:
            self._ana_output.get_buffer().set_text(f"Error: {e}")

    def _show_arduino(self):
        content = self._make_page("🔌 ARDUINO — Librería TernaryAncestral")

        f1 = Gtk.Frame(label=" Librería TernaryAncestral ")
        f1.get_style_context().add_class("card")
        vb1 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        vb1.set_margin_start(8)
        vb1.set_margin_end(8)
        vb1.set_margin_top(8)
        vb1.set_margin_bottom(8)
        f1.add(vb1)

        desc = Gtk.Label(label="Librería Arduino para comunicación ternaria ancestral.\n"
                               "Cada trit = 3 valores: -1, 0, +1 → transmisión eficiente de datos.")
        desc.get_style_context().add_class("card-desc")
        desc.set_xalign(0)
        desc.set_line_wrap(True)
        vb1.pack_start(desc, False, False, 0)

        modes_grid = Gtk.Grid()
        modes_grid.set_column_spacing(8)
        modes_grid.set_row_spacing(8)
        modes_grid.set_margin_top(8)
        vb1.pack_start(modes_grid, False, False, 0)

        modes = [
            ("📡 Lite", "15 bytes", "Modo ligero para sensores\n"
             "Pines: A0, D2\nVelocidad: 4800 baud"),
            ("📡 Original", "162 bytes", "Modo completo original\n"
             "Pines: A0–A5, D2–D8\nVelocidad: 9600 baud"),
            ("🏰 Babylonian", "198 bytes", "Modo base 60 ancestral\n"
             "Pines: A0–A5, D2–D10\nVelocidad: 115200 baud"),
        ]
        for i, (title, size, desc_text) in enumerate(modes):
            card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
            card.get_style_context().add_class("card")
            card.set_size_request(200, -1)
            t_lbl = Gtk.Label(label=f"{title}  ({size})")
            t_lbl.get_style_context().add_class("card-name")
            card.pack_start(t_lbl, False, False, 0)
            d_lbl = Gtk.Label(label=desc_text)
            d_lbl.get_style_context().add_class("card-desc")
            d_lbl.set_xalign(0)
            d_lbl.set_line_wrap(True)
            card.pack_start(d_lbl, False, False, 0)
            modes_grid.attach(card, i, 0, 1, 1)
        content.pack_start(f1, False, False, 0)

        f2 = Gtk.Frame(label=" Compatibilidad ")
        f2.get_style_context().add_class("card")
        vb2 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        vb2.set_margin_start(8)
        vb2.set_margin_end(8)
        vb2.set_margin_top(8)
        vb2.set_margin_bottom(8)
        f2.add(vb2)
        boards = ["Arduino Uno/Nano/Mega", "ESP32", "ESP8266", "ATtiny85"]
        for b in boards:
            bl = Gtk.Label(label=f"  ✓  {b}")
            bl.get_style_context().add_class("card-desc")
            bl.set_xalign(0)
            vb2.pack_start(bl, False, False, 0)
        content.pack_start(f2, False, False, 0)

        f3 = Gtk.Frame(label=" Ejemplos ")
        f3.get_style_context().add_class("card")
        vb3 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        vb3.set_margin_start(8)
        vb3.set_margin_end(8)
        vb3.set_margin_top(8)
        vb3.set_margin_bottom(8)
        f3.add(vb3)
        examples = [
            ("sensor_transmitter", "Transmite datos de sensor con codificación ternaria"),
            ("data_receiver", "Recibe y decodifica datos ternarios"),
        ]
        for name, desc_text in examples:
            el = Gtk.Label(label=f"  📁 {name}\n     {desc_text}")
            el.get_style_context().add_class("card-desc")
            el.set_xalign(0)
            el.set_line_wrap(True)
            vb3.pack_start(el, False, False, 0)
        content.pack_start(f3, False, False, 0)

    def _show_experiments(self):
        content = self._make_page("🧪 EXPERIMENTOS")

        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)

        grid = Gtk.Grid()
        grid.set_column_spacing(10)
        grid.set_row_spacing(10)
        grid.set_halign(Gtk.Align.CENTER)
        grid.set_valign(Gtk.Align.START)
        grid.set_margin_top(12)

        experiments = [
            ("01", "Sensores Ternarios\nAncestrales",
             "Compresión: 3.2x vs binario\n"
             "Ahorro energía: 67%\n"
             "Latencia: reducida 40%"),
            ("02", "Nodo Ternario\nDatacenter",
             "Compresión: 8x\n"
             "Ahorro energía: 80.2%\n"
             "Throughput: +35%"),
            ("03", "Compresión\nAncestral Extendida",
             "Maya: 4.1x compresión\n"
             "Persa: 3.8x compresión\n"
             "Babilonia: 3.5x compresión"),
            ("04", "Calendario Maya/\nAzteca/Persa",
             "Maya: 13 Baktunes, Long Count\n"
             "Azteca: Xiuhpohualli 365 días\n"
             "Persa: Solar Hijri, Nowruz"),
        ]

        for i, (num, title, results) in enumerate(experiments):
            col = i % 2
            row = i // 2
            card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
            card.get_style_context().add_class("card")
            card.set_size_request(380, -1)
            num_lbl = Gtk.Label(label=f"#{num}")
            num_lbl.set_markup(f'<span size="large" weight="bold" color="#ffd700">#{num}</span>')
            card.pack_start(num_lbl, False, False, 0)
            t_lbl = Gtk.Label(label=title)
            t_lbl.get_style_context().add_class("card-name")
            t_lbl.set_line_wrap(True)
            card.pack_start(t_lbl, False, False, 0)
            r_lbl = Gtk.Label(label=results)
            r_lbl.get_style_context().add_class("card-desc")
            r_lbl.set_xalign(0)
            r_lbl.set_line_wrap(True)
            card.pack_start(r_lbl, False, False, 0)

            exp_dir = os.path.join(SCRIPT_DIR, f"experiments/exp{num}")
            for fname in os.listdir(exp_dir) if os.path.isdir(exp_dir) else []:
                if fname.endswith(".json"):
                    try:
                        import json
                        with open(os.path.join(exp_dir, fname)) as jf:
                            data = json.load(jf)
                        summary = f"  📊 {fname}: {json.dumps(data, indent=2)[:200]}"
                        s_lbl = Gtk.Label(label=summary)
                        s_lbl.get_style_context().add_class("output-text")
                        s_lbl.set_xalign(0)
                        s_lbl.set_line_wrap(True)
                        card.pack_start(s_lbl, False, False, 0)
                    except Exception:
                        pass

            grid.attach(card, col, row, 1, 1)

        scroll.add(grid)
        content.pack_start(scroll, True, True, 0)

    def _show_kernel(self):
        content = self._make_page("🖥️ KERNEL BARE-METAL TRITOS")

        f1 = Gtk.Frame(label=" Kernel x86 ")
        f1.get_style_context().add_class("card")
        vb1 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        vb1.set_margin_start(8)
        vb1.set_margin_end(8)
        vb1.set_margin_top(8)
        vb1.set_margin_bottom(8)
        f1.add(vb1)

        info = Gtk.Label(label="Kernel bare-metal x86 en ensamblador.\n"
                               "Components: boot.asm (bootloader), modo texto VGA, "
                               "driver de teclado, shell interactivo.")
        info.get_style_context().add_class("card-desc")
        info.set_xalign(0)
        info.set_line_wrap(True)
        vb1.pack_start(info, False, False, 0)
        content.pack_start(f1, False, False, 0)

        f2 = Gtk.Frame(label=" Compilar ")
        f2.get_style_context().add_class("card")
        vb2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        vb2.set_margin_start(8)
        vb2.set_margin_end(8)
        vb2.set_margin_top(8)
        vb2.set_margin_bottom(8)
        f2.add(vb2)
        build_btn = Gtk.Button(label="▶ Compilar Kernel")
        build_btn.get_style_context().add_class("run-btn")
        build_btn.connect("clicked", lambda w: self._kernel_build())
        vb2.pack_start(build_btn, False, False, 0)
        content.pack_start(f2, False, False, 0)

        f3 = Gtk.Frame(label=" Ejecutar ")
        f3.get_style_context().add_class("card")
        vb3 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        vb3.set_margin_start(8)
        vb3.set_margin_end(8)
        vb3.set_margin_top(8)
        vb3.set_margin_bottom(8)
        f3.add(vb3)
        qemu_btn = Gtk.Button(label="▶ QEMU")
        qemu_btn.get_style_context().add_class("gen-btn")
        qemu_btn.connect("clicked", lambda w: self._kernel_run())
        vb3.pack_start(qemu_btn, False, False, 0)
        content.pack_start(f3, False, False, 0)

        of = Gtk.Frame(label=" Resultado ")
        of.get_style_context().add_class("card")
        self._kern_output = Gtk.TextView()
        self._kern_output.get_style_context().add_class("output-text")
        self._kern_output.set_editable(False)
        self._kern_output.set_monospace(True)
        self._kern_output.set_wrap_mode(Gtk.WrapMode.WORD_CHAR)
        sw = Gtk.ScrolledWindow()
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        sw.add(self._kern_output)
        of.add(sw)
        content.pack_start(of, True, True, 0)

    def _kernel_build(self):
        script_dir = os.path.dirname(os.path.abspath(__file__))
        build_script = os.path.join(script_dir, "build-kernel.sh")
        if not os.path.exists(build_script):
            build_script = os.path.join(script_dir, "bin", "build-kernel.sh")
        cmd = ["bash", build_script]
        self._kern_output.get_buffer().set_text(f"$ {' '.join(cmd)}\n\n")
        try:
            r = subprocess.run(cmd, capture_output=True, text=True, timeout=60,
                               cwd=script_dir)
            self._kern_output.get_buffer().set_text(r.stdout + r.stderr)
        except FileNotFoundError:
            self._kern_output.get_buffer().set_text(
                "Error: build-kernel.sh no encontrado.\n"
                f"Esperado en: {script_dir}")
        except subprocess.TimeoutExpired:
            self._kern_output.get_buffer().set_text("Timeout después de 60s")
        except Exception as e:
            self._kern_output.get_buffer().set_text(f"Error: {e}")

    def _kernel_run(self):
        script_dir = os.path.dirname(os.path.abspath(__file__))
        disk_img = os.path.join(script_dir, "disk.img")
        if not os.path.exists(disk_img):
            self._kern_output.get_buffer().set_text(
                f"Error: disk.img no encontrado en {script_dir}\n"
                "Compilá el kernel primero.")
            return
        cmd = ["qemu-system-i386", "-drive", f"file={disk.img}"]
        self._kern_output.get_buffer().set_text(f"$ {' '.join(cmd)}\n\n")
        try:
            subprocess.Popen(cmd, cwd=script_dir,
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self._kern_output.get_buffer().set_text(
                "QEMU iniciado.\n"
                f"$ {' '.join(cmd)}")
        except FileNotFoundError:
            self._kern_output.get_buffer().set_text(
                "Error: qemu-system-i386 no encontrado.\n"
                "Instalá QEMU: sudo apt install qemu-system-x86")
        except Exception as e:
            self._kern_output.get_buffer().set_text(f"Error: {e}")

    def _show_formulas(self):
        content = self._make_page("📚 FÓRMULAS / REFERENCIA CIENTÍFICA")

        categories = [
            ("⚖️ Aritmética Ternaria", [
                ("Decimal → Balanceado", "r = n % 3; si r==2: digit=-1, carry=+1"),
                ("Balanceado → Decimal", "value = Σ(d[i] × 3^i, i=0..n)"),
                ("Suma Ternaria", "sum = a[i] + b[i] + carry; si >1: -3, carry=1"),
                ("Float → Coordenada Ternaria", "digit = 1 si v/power ≥ 0.666, 0 si ≥ 0.333, -1 si no"),
            ]),
            ("🧲 Modelo de Ising Ternario", [
                ("Hamiltoniano (E local)", "E = -J × Σ(s_i × s_vecino) - h × s_i"),
                ("Energía Total", "E_total = (1/2) × Σ(E_local_i)"),
                ("Magnetización", "M = (1/N) × Σ(s_i)"),
                ("Criterio de Metropolis", "si ΔE ≤ 0: aceptar; si rand() < exp(-ΔE/T): aceptar"),
                ("Calor Específico", "C = (⟨E²⟩ - ⟨E⟩²) / (k × T²)"),
                ("Susceptibilidad", "χ = (⟨M²⟩ - ⟨M⟩²) / (k × T)"),
            ]),
            ("⚛️ Dinámica Molecular", [
                ("Potencial Lennard-Jones", "U(r) = 4ε × ((σ/r)¹² - (σ/r)⁶)"),
                ("Fuerza Lennard-Jones", "F(r) = 24ε × (2(σ/r)¹² - (σ/r)⁶) / r"),
                ("Integración Velocity Verlet", "x(t+dt) = x(t) + v(t)dt + f(t)dt²/2"),
                ("Energía Cinética", "KE = Σ(½ × m × (vx² + vy² + vz²))"),
                ("Convención de Imagen Mínima", "dx = x_i - x_j - L × round((x_i - x_j)/L)"),
                ("Maxwell-Boltzmann", "v_x = (rand() - 0.5) × √T"),
            ]),
            ("🧠 Perceptrón Ternario", [
                ("Activación Ternaria", "f(sum) = +1 si >0, 0 si =0, -1 si <0"),
                ("Producto Punto", "dot(a,b) = Σ(a[i] × b[i])"),
                ("Forward Pass", "output = activation(Σ(W[i][j] × input[j] + bias[i]))"),
                ("Regla de Aprendizaje", "delta = sign(error) × sign(input); W += delta"),
            ]),
            ("🔄 Red de Hopfield", [
                ("Actualización Hebbiana", "W[i][j] = sign(pattern[i] × pattern[j])"),
                ("Actualización Neurona", "sum = Σ(W[i][j] × state[j]); state = sign(sum + noise)"),
                ("Overlap", "overlap = count(state[i] == pattern[i]) / N"),
            ]),
            ("🔐 Criptografía", [
                ("Exponenciación Modular", "mod_pow(base, exp, mod) — squaring repetido"),
                ("GCD Extendido", "ax + by = gcd(a,b)"),
                ("Inversa Modular", "mod_inverse(a,m) = x tal que a×x ≡ 1 (mod m)"),
                ("RSA: Generación de Claves", "n=p×q; φ=(p-1)(q-1); d=mod_inverse(e,φ)"),
                ("Diffie-Hellman", "pub = g^priv mod p; shared = remote^local mod p"),
                ("PRNG Ternario (LFSR)", "feedback = (s[0]+s[1]+s[2]) % 3"),
            ]),
            ("🌊 Ondas Sísmicas", [
                ("Onda P", "amp = mag × exp(-dist/100) × exp(-|t-t_p|/0.1)"),
                ("Onda S", "amp = mag × 1.5 × exp(-dist/80) × exp(-|t-t_s|/0.2)"),
                ("Onda Superficial", "amp = mag × 2.0 × exp(-dist/60) × exp(-|t-t_surf|/0.5)"),
                ("Tiempos de Llegada", "t_p = dist/v_p (v_p=6 km/s); t_s = dist/v_s (v_s=3.5 km/s)"),
                ("Amplitud Combinada", "A = √(A_p² + A_s² + A_surf²)"),
            ]),
            ("🌍 Sensores Ambientales", [
                ("Derivada Temperatura", "dT/dt = k_rad×(rad-500)/500 + k_wind×wind/10 + noise"),
                ("Derivada Humedad", "dH/dt = k_temp×(T-25)/10 + k_rain×rain/10 + noise"),
                ("Forzado Solar", "radiation = 500 + 400 × sin((hour-6)/24 × 2π)"),
                ("Integración Euler", "T(t+dt) = T(t) + dT/dt × dt"),
            ]),
            ("⚡ Límite de Landauer", [
                ("Energía por Símbolo", "E = k_B × T × ln(B); k_B=1.38e-23 J/K"),
                ("Eficiencia de Codificación", "eff(B) = ln(B)/B; máximo en B=e≈2.718"),
                ("Ventaja Ternaria vs Binaria", "(ln(3)/3 - ln(2)/2) / (ln(2)/2) × 100 = 5.7%"),
                ("Ahorro Energético", "saving = 1 - (E_ternary / E_binary)"),
            ]),
            ("📦 Compresión", [
                ("Residuo mod-33", "value = Σ(block[i] × 3^(3-i)); residue = value % 33"),
                ("Checksum Quipu", "p1 = -(Σblock) % 3; p2 = -(Πblock) % 3"),
                ("Ratio de Compresión", "ratio = original_bits / compressed_bits"),
                ("Delta Encoding + Ternario", "delta[i] = data[i] - data[i-1]; pack 5 trits/int"),
                ("Run-Length Ternario", "output = (state+1) << 14 | (count & 0x3FFF)"),
                ("MSE", "MSE = (1/n) × Σ(original[i] - compressed[i])²"),
            ]),
            ("📅 Calendarios Ancestrales", [
                ("Maya: Long Count → JDN", "JDN = 584283 + (b×144000 + k×7200 + t×360 + w×20 + n)"),
                ("Tonalpohualli (260 días)", "(día, signo) = ((n-1)%13 + 1, (n-1)%20 + 1)"),
                ("Año Bisiesto Persa", "bisiesto si (año % 33) ∈ {1,5,9,13,17,22,26,30}"),
                ("Error de Año Medio", "error_seg = (año_medio - 365.24219) × 86400"),
                ("Calendario Redondo", "LCM(260,365) = 18980; 52×365 = 73×260"),
                ("Era 13 Baktun", "13 × 144000 = 1.872.000 días"),
                ("Ciclo Persa 33 años", "año_medio = 12053/33 = 365.24242 días"),
            ]),
            ("🔢 Compresión Multi-Base", [
                ("Trits por Dígito", "trits = log₃(base); Maya=2.72, Persa=3.15, Babilonia=3.91"),
                ("Bits por Dígito", "bits = log₂(base); Babilonia(60)=5.91 bits"),
                ("Ahorro vs Ternario", "Maya=63%, Persa=68.2%, Babilonia=74.3%"),
            ]),
            ("📊 Seguridad", [
                ("Reputación", "rep = Σ(confidence[i] × score[i]) / count"),
                ("Tasa de Falsos Positivos", "FPR = FP / (FP + TN)"),
                ("Precisión / Recall / F1", "P=TP/(TP+FP); R=TP/(TP+FN); F1=2PR/(P+R)"),
            ]),
            ("🖨️ Render 3D", [
                ("Vector Normal (Cross Product)", "n = AB × AC; n_x = (B_y-A_y)(C_z-A_z) - (B_z-A_z)(C_y-A_y)"),
            ]),
            ("📈 Entropía", [
                ("Entropía de Shannon", "H = -Σ(p_i × log₂(p_i))"),
            ]),
            ("🧬 IA Ternaria", [
                ("Voto Ponderado", "norm = Σ(value×conf) / Σ(conf); >0.3: ⊕, <-0.3: ⊖, sino: 0"),
                ("Confianza Promedio", "avg = Σ(confidence[i]) / count"),
            ]),
        ]

        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        content.pack_start(scroll, True, True, 0)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        vbox.set_margin_start(16)
        vbox.set_margin_end(16)
        vbox.set_margin_top(12)
        scroll.add(vbox)

        # Summary
        summary = Gtk.Label()
        summary.set_markup(
            '<span size="large" weight="bold" color="#ffd700">'
            '67 fórmulas en 20 categorías — Todo el conocimiento científico de TRITOS</span>')
        summary.set_xalign(0)
        summary.set_line_wrap(True)
        vbox.pack_start(summary, False, False, 0)

        for cat_name, formulas in categories:
            frame = Gtk.Frame(label=f" {cat_name} ")
            frame.get_style_context().add_class("card")
            vbox.pack_start(frame, False, False, 0)

            grid = Gtk.Grid()
            grid.set_column_spacing(12)
            grid.set_row_spacing(4)
            grid.set_margin_start(8)
            grid.set_margin_end(8)
            grid.set_margin_top(6)
            grid.set_margin_bottom(6)
            frame.add(grid)

            for i, (name, formula) in enumerate(formulas):
                name_lbl = Gtk.Label(label=name)
                name_lbl.set_xalign(1)
                name_lbl.set_markup(f'<span weight="bold" color="#c9d1d9">{name}</span>')
                grid.attach(name_lbl, 0, i, 1, 1)

                formula_lbl = Gtk.Label(label=formula)
                formula_lbl.set_xalign(0)
                formula_lbl.set_markup(f'<span font_family="monospace" color="#7ee787">{formula.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")}</span>')
                grid.attach(formula_lbl, 1, i, 1, 1)

    def _show_shell(self):
        script_dir = os.path.dirname(os.path.abspath(__file__))
        tritos_bin = os.path.join(script_dir, "tritos")
        if not os.path.exists(tritos_bin):
            tritos_bin = os.path.join(script_dir, "bin", "tritos")

        try:
            subprocess.Popen(
                ["xterm", "-title", "Tritos Shell", "-e", tritos_bin],
                cwd=script_dir)
        except FileNotFoundError:
            content = self._make_page("🐚 SHELL TRITOS")
            err_label = Gtk.Label(
                label="xterm no encontrado. Instalá xterm o abrí una terminal "
                      "manualmente:\n\n"
                      f"  cd {script_dir}\n  ./tritos")
            err_label.get_style_context().add_class("card")
            err_label.set_line_wrap(True)
            err_label.set_xalign(0)
            err_label.set_margin_top(20)
            content.pack_start(err_label, False, False, 0)


def main():
    app = TritosGUI()
    app.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
