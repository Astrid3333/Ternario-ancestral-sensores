#!/usr/bin/env python3
"""
tritos_math_panel.py — Panel Matemático para Tritos GUI

Integra octave-mcp con la GUI GTK3 de Tritos.
"""

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk, GLib
import json
import os
import sys

# Import MCP client
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tritos_mcp_client import TritosMath

# Ternary colors
TRIT_COLORS = {
    'neg': '#E63946',    # -1 (red)
    'zero': '#457B9D',   # 0 (blue)
    'pos': '#2A9D8F',    # +1 (green)
    'bg': '#1D3557',     # background
    'fg': '#F1FAEE',     # foreground
    'accent': '#E9C46A',  # accent
}

class MathPanel(Gtk.Box):
    """Panel de matemática ternaria"""
    
    def __init__(self):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.set_margin_start(10)
        self.set_margin_end(10)
        self.set_margin_top(10)
        self.set_margin_bottom(10)
        
        self.math = TritosMath()
        
        self._build_ui()
    
    def _build_ui(self):
        # Title
        title = Gtk.Label()
        title.set_markup('<span size="x-large" weight="bold">Matemática Ternaria</span>')
        title.set_halign(Gtk.Align.CENTER)
        self.pack_start(title, False, False, 0)
        
        # Notebook for tabs
        self.notebook = Gtk.Notebook()
        self.pack_start(self.notebook, True, True, 0)
        
        # Tab 1: Ternary Arithmetic
        self._build_arithmetic_tab()
        
        # Tab 2: Conversion
        self._build_conversion_tab()
        
        # Tab 3: Landauer
        self._build_landauer_tab()
        
        # Tab 4: Ethnomath
        self._build_ethnomath_tab()
        
        # Result area
        self.result_label = Gtk.Label()
        self.result_label.set_markup('<span size="medium">Resultado aparecerá aquí</span>')
        self.result_label.set_halign(Gtk.Align.CENTER)
        self.result_label.set_line_wrap(True)
        self.pack_start(self.result_label, False, False, 0)
    
    def _build_arithmetic_tab(self):
        """Tab de aritmética ternaria"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        
        # Input fields
        grid = Gtk.Grid()
        grid.set_column_spacing(10)
        grid.set_row_spacing(6)
        
        label_a = Gtk.Label(label="A:")
        self.entry_a = Gtk.Entry()
        self.entry_a.set_width_chars(10)
        self.entry_a.set_placeholder_text("0")
        
        label_b = Gtk.Label(label="B:")
        self.entry_b = Gtk.Entry()
        self.entry_b.set_width_chars(10)
        self.entry_b.set_placeholder_text("0")
        
        grid.attach(label_a, 0, 0, 1, 1)
        grid.attach(self.entry_a, 1, 0, 1, 1)
        grid.attach(label_b, 0, 1, 1, 1)
        grid.attach(self.entry_b, 1, 1, 1, 1)
        
        box.pack_start(grid, False, False, 0)
        
        # Buttons
        btn_box = Gtk.Box(spacing=6)
        
        btn_add = Gtk.Button(label="+ Sumar")
        btn_add.connect("clicked", self._on_add)
        
        btn_sub = Gtk.Button(label="- Restar")
        btn_sub.connect("clicked", self._on_sub)
        
        btn_mul = Gtk.Button(label="× Multiplicar")
        btn_mul.connect("clicked", self._on_mul)
        
        btn_box.pack_start(btn_add, True, True, 0)
        btn_box.pack_start(btn_sub, True, True, 0)
        btn_box.pack_start(btn_mul, True, True, 0)
        
        box.pack_start(btn_box, False, False, 0)
        
        self.notebook.append_page(box, Gtk.Label(label="Aritmética"))
    
    def _build_conversion_tab(self):
        """Tab de conversión"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        
        label = Gtk.Label(label="Valor decimal:")
        self.entry_conv = Gtk.Entry()
        self.entry_conv.set_width_chars(10)
        self.entry_conv.set_placeholder_text("255")
        
        btn_convert = Gtk.Button(label="Convertir a Ternario Balanceado")
        btn_convert.connect("clicked", self._on_convert)
        
        box.pack_start(label, False, False, 0)
        box.pack_start(self.entry_conv, False, False, 0)
        box.pack_start(btn_convert, False, False, 0)
        
        self.notebook.append_page(box, Gtk.Label(label="Conversión"))
    
    def _build_landauer_tab(self):
        """Tab de Landauer"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        
        label = Gtk.Label(label="Número de símbolos:")
        self.entry_symbols = Gtk.Entry()
        self.entry_symbols.set_width_chars(10)
        self.entry_symbols.set_placeholder_text("1000")
        
        label_base = Gtk.Label(label="Base:")
        self.entry_base = Gtk.Entry()
        self.entry_base.set_width_chars(10)
        self.entry_base.set_placeholder_text("3")
        
        btn_landauer = Gtk.Button(label="Calcular Límite de Landauer")
        btn_landauer.connect("clicked", self._on_landauer)
        
        box.pack_start(label, False, False, 0)
        box.pack_start(self.entry_symbols, False, False, 0)
        box.pack_start(label_base, False, False, 0)
        box.pack_start(self.entry_base, False, False, 0)
        box.pack_start(btn_landauer, False, False, 0)
        
        self.notebook.append_page(box, Gtk.Label(label="Landauer"))
    
    def _build_ethnomath_tab(self):
        """Tab de etnomatemática"""
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        
        label_sys = Gtk.Label(label="Sistema:")
        self.combo_system = Gtk.ComboBoxText()
        for sys in ["maya", "persian", "chinese", "babylonian"]:
            self.combo_system.append_text(sys)
        self.combo_system.set_active(0)
        
        label_num = Gtk.Label(label="Número:")
        self.entry_ethno = Gtk.Entry()
        self.entry_ethno.set_width_chars(10)
        self.entry_ethno.set_placeholder_text("255")
        
        btn_ethno = Gtk.Button(label="Calcular")
        btn_ethno.connect("clicked", self._on_ethnomath)
        
        btn_compare = Gtk.Button(label="Comparar Todos")
        btn_compare.connect("clicked", self._on_compare)
        
        box.pack_start(label_sys, False, False, 0)
        box.pack_start(self.combo_system, False, False, 0)
        box.pack_start(label_num, False, False, 0)
        box.pack_start(self.entry_ethno, False, False, 0)
        
        btn_box = Gtk.Box(spacing=6)
        btn_box.pack_start(btn_ethno, True, True, 0)
        btn_box.pack_start(btn_compare, True, True, 0)
        box.pack_start(btn_box, False, False, 0)
        
        self.notebook.append_page(box, Gtk.Label(label="Etnomatemática"))
    
    def _show_result(self, text: str):
        """Show result in the result area"""
        self.result_label.set_markup(f'<span size="medium">{text}</span>')
    
    def _on_add(self, button):
        try:
            a = int(self.entry_a.get_text() or "0")
            b = int(self.entry_b.get_text() or "0")
            result = self.math.ternary_add(a, b)
            self._show_result(f"Resultado: {json.dumps(result, indent=2)}")
        except Exception as e:
            self._show_result(f"Error: {e}")
    
    def _on_sub(self, button):
        try:
            a = int(self.entry_a.get_text() or "0")
            b = int(self.entry_b.get_text() or "0")
            result = self.math.ternary_sub(a, b)
            self._show_result(f"Resultado: {json.dumps(result, indent=2)}")
        except Exception as e:
            self._show_result(f"Error: {e}")
    
    def _on_mul(self, button):
        try:
            a = int(self.entry_a.get_text() or "0")
            b = int(self.entry_b.get_text() or "0")
            result = self.math.ternary_mul(a, b)
            self._show_result(f"Resultado: {json.dumps(result, indent=2)}")
        except Exception as e:
            self._show_result(f"Error: {e}")
    
    def _on_convert(self, button):
        try:
            value = int(self.entry_conv.get_text() or "0")
            result = self.math.ternary_to_balanced(value)
            self._show_result(f"Resultado: {json.dumps(result, indent=2)}")
        except Exception as e:
            self._show_result(f"Error: {e}")
    
    def _on_landauer(self, button):
        try:
            symbols = int(self.entry_symbols.get_text() or "1000")
            base = int(self.entry_base.get_text() or "3")
            result = self.math.landauer(symbols, base)
            self._show_result(f"Resultado: {json.dumps(result, indent=2)}")
        except Exception as e:
            self._show_result(f"Error: {e}")
    
    def _on_ethnomath(self, button):
        try:
            system = self.combo_system.get_active_text()
            number = int(self.entry_ethno.get_text() or "0")
            result = self.math.ethnomath(system, number)
            self._show_result(f"Resultado: {json.dumps(result, indent=2)}")
        except Exception as e:
            self._show_result(f"Error: {e}")
    
    def _on_compare(self, button):
        try:
            number = int(self.entry_ethno.get_text() or "0")
            result = self.math.ethnomath_compare(number)
            self._show_result(f"Resultado: {json.dumps(result, indent=2)}")
        except Exception as e:
            self._show_result(f"Error: {e}")


def create_math_panel() -> Gtk.Box:
    """Factory function to create the math panel"""
    return MathPanel()


if __name__ == "__main__":
    # Test standalone
    win = Gtk.Window(title="Tritos Math Panel")
    win.set_default_size(400, 500)
    win.connect("destroy", Gtk.main_quit)
    
    panel = MathPanel()
    win.add(panel)
    
    win.show_all()
    Gtk.main()
