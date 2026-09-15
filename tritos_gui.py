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
    text-align: left;
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
    text-align: left;
    font-size: 12px;
    color: #c9d1d9;
}
.file-item:hover { background-color: #21262d; }
.path-label { color: #8b949e; font-size: 11px; font-family: monospace; }
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
    ("section", "APLICACIONES"),
    ("item", "🔬 Ciencia Ternaria", "science"),
    ("item", "📡 Sensores", "sensors"),
    ("item", "🖨️ Impresión 3D", "render"),
    ("item", "🔐 Seguridad", "crypto"),
    ("section", "HERRAMIENTAS"),
    ("item", "📁 Archivos", "files"),
    ("item", "⚙️ Sistema", "system"),
    ("item", "🎵 Audio", "audio"),
    ("item", "🧮 Matemática", "math"),
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

        taskbar_alloc = self.taskbar.get_allocation()
        self.taskbar.get_window().get_origin(0, 0)
        root_window = self.get_root_window()
        tx, ty = self.taskbar.translate_coordinates(root_window, 0, 0)
        menu.move(tx, ty - menu.get_allocated_height())

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
        outer.show_all()
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
            "shell": self._show_shell,
        }
        fn = dispatch.get(section, self._show_desktop)
        fn()

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
            ("🔬", "Ciencia\nTernaria", "science"),
            ("📡", "Sensores", "sensors"),
            ("🖨️", "Impresión\n3D", "render"),
            ("🔐", "Seguridad", "crypto"),
            ("📁", "Archivos", "files"),
            ("⚙️", "Sistema", "system"),
            ("🎵", "Audio", "audio"),
            ("🧮", "Matemática", "math"),
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
