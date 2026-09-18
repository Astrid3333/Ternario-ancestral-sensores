#!/usr/bin/env python3
"""TRITOS Web Browser — GTK WebKit2"""
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('WebKit2', '4.1')
from gi.repository import Gtk, WebKit2, Gdk, GLib

class TritosBrowser(Gtk.Window):
    def __init__(self):
        super().__init__(title="TRITOS Browser")
        self.set_default_size(1024, 700)
        self.set_icon_name("web-browser")

        style = Gtk.CssProvider()
        try:
            style.load_from_data(b"""
                .tritos-header { background: #1a1b26; padding: 2px; }
                .tritos-url { background: #24283b; color: #c0caf5; font-family: monospace;
                              font-size: 13px; padding: 4px 8px; border-radius: 4px;
                              border: 1px solid #414868; }
                .tritos-url:focus { border-color: #7aa2f7; }
                .tritos-btn { background: #414868; color: #c0caf5; padding: 4px 10px;
                              border-radius: 4px; font-size: 12px; }
                .tritos-btn:hover { background: #565f89; }
                .tritos-status { background: #1a1b26; color: #565f89; font-size: 11px;
                                 padding: 2px 8px; }
            """)
        except Exception:
            pass
        style_ctx = self.get_style_context()
        style_ctx.add_provider(style, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(), style,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.add(vbox)

        # Navigation bar
        hbox = Gtk.Box(spacing=6)
        hbox.set_margin_start(6)
        hbox.set_margin_end(6)
        hbox.set_margin_top(4)
        hbox.set_margin_bottom(4)
        hbox.get_style_context().add_class("tritos-header")
        vbox.pack_start(hbox, False, False, 0)

        self.btn_back = Gtk.Button(label="◀")
        self.btn_back.get_style_context().add_class("tritos-btn")
        self.btn_back.connect("clicked", self.on_back)
        hbox.pack_start(self.btn_back, False, False, 0)

        self.btn_forward = Gtk.Button(label="▶")
        self.btn_forward.get_style_context().add_class("tritos-btn")
        self.btn_forward.connect("clicked", self.on_forward)
        hbox.pack_start(self.btn_forward, False, False, 0)

        self.btn_reload = Gtk.Button(label="⟳")
        self.btn_reload.get_style_context().add_class("tritos-btn")
        self.btn_reload.connect("clicked", self.on_reload)
        hbox.pack_start(self.btn_reload, False, False, 0)

        self.btn_home = Gtk.Button(label="⌂")
        self.btn_home.get_style_context().add_class("tritos-btn")
        self.btn_home.connect("clicked", self.on_home)
        hbox.pack_start(self.btn_home, False, False, 0)

        self.url_bar = Gtk.Entry()
        self.url_bar.get_style_context().add_class("tritos-url")
        self.url_bar.set_hexpand(True)
        self.url_bar.connect("activate", self.on_url_enter)
        hbox.pack_start(self.url_bar, True, True, 0)

        self.btn_go = Gtk.Button(label="Ir")
        self.btn_go.get_style_context().add_class("tritos-btn")
        self.btn_go.connect("clicked", self.on_url_enter)
        hbox.pack_start(self.btn_go, False, False, 0)

        # WebView
        self.webview = WebKit2.WebView()
        self.webview.get_settings().set_enable_javascript(True)
        self.webview.connect("load-changed", self.on_load_changed)
        self.webview.connect("decide-policy", self.on_decide_policy)

        scroll = Gtk.ScrolledWindow()
        scroll.add(self.webview)
        vbox.pack_start(scroll, True, True, 0)

        # Status bar
        self.statusbar = Gtk.Label(label="Listo")
        self.statusbar.set_xalign(0)
        self.statusbar.get_style_context().add_class("tritos-status")
        vbox.pack_start(self.statusbar, False, False, 0)

        # Bookmarks bar
        bbox = Gtk.Box(spacing=4)
        bbox.set_margin_start(6)
        bbox.set_margin_end(6)
        bbox.set_margin_bottom(2)
        for name, url in [
            ("Wikipedia", "https://es.wikipedia.org"),
            ("GitHub", "https://github.com"),
            ("arXiv", "https://arxiv.org"),
            ("OpenScience", "https://github.com/Astrid3333"),
        ]:
            btn = Gtk.Button(label=name)
            btn.get_style_context().add_class("tritos-btn")
            btn.connect("clicked", self.on_bookmark, url)
            bbox.pack_start(btn, False, False, 0)
        vbox.pack_start(bbox, False, False, 0)

        # Load initial URL
        initial = "https://es.wikipedia.org/wiki/Ternary_computing"
        if len(sys.argv) > 1:
            initial = sys.argv[1]
        self.webview.load_uri(initial)

        self.show_all()

    def on_back(self, btn):
        if self.webview.can_go_back():
            self.webview.go_back()

    def on_forward(self, btn):
        if self.webview.can_go_forward():
            self.webview.go_forward()

    def on_reload(self, btn):
        self.webview.reload()

    def on_home(self, btn):
        self.webview.load_uri("https://es.wikipedia.org")

    def on_url_enter(self, widget):
        url = self.url_bar.get_text().strip()
        if url and not url.startswith("http"):
            if "." in url:
                url = "https://" + url
            else:
                url = "https://es.wikipedia.org/wiki/" + url.replace(" ", "_")
        if url:
            self.webview.load_uri(url)

    def on_bookmark(self, btn, url):
        self.webview.load_uri(url)

    def on_load_changed(self, webview, load_event):
        if load_event == WebKit2.LoadEvent.STARTED:
            self.statusbar.set_text("Cargando...")
        elif load_event == WebKit2.LoadEvent.REDIRECTED:
            self.statusbar.set_text("Redirigiendo...")
        elif load_event == WebKit2.LoadEvent.COMMITTED:
            uri = webview.get_uri()
            self.url_bar.set_text(uri or "")
            self.statusbar.set_text("Cargando contenido...")
        elif load_event == WebKit2.LoadEvent.FINISHED:
            title = webview.get_title()
            self.statusbar.set_text(f"Listo — {title or ''}")

    def on_decide_policy(self, webview, decision, decision_type):
        if decision_type == WebKit2.PolicyDecisionType.NAVIGATION_ACTION:
            nav_action = decision.get_navigation_action()
            request = nav_action.get_request()
            uri = request.get_uri()
            self.url_bar.set_text(uri or "")
        return False

import sys
win = TritosBrowser()
win.connect("destroy", Gtk.main_quit)
Gtk.main()
