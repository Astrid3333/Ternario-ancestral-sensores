#!/usr/bin/env python3
"""TRITOS Encyclopedia — Britannica (seria, expertos verificados)"""
import sys
import json
import urllib.request
import urllib.parse
import re

BRITANNICA = "https://www.britannica.com"

def search(query, limit=5):
    url = f"{BRITANNICA}/search?query={urllib.parse.quote(query)}"
    req = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36",
        "Accept": "text/html"
    })
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            html = resp.read().decode("utf-8", errors="replace")

        results = []
        # Find article links
        matches = re.findall(r'href="(\/[a-z][\w\/\-]+)"', html)
        seen = set()
        for m in matches:
            if m not in seen and m.count('/') >= 2 and '/search' not in m and '/browse' not in m:
                seen.add(m)
                title = m.split('/')[-1].replace('-', ' ').title()
                results.append({"path": m, "title": title})
                if len(results) >= limit:
                    break

        for i, r in enumerate(results):
            print(f"  [{i+1}] {r['title']}")
            print(f"      britannica.com{r['path']}")
        return results
    except Exception as e:
        print(f"  Error: {e}")
        return []

def read_article(path):
    url = f"{BRITANNICA}{path}"
    req = urllib.request.Request(url, headers={
        "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36",
        "Accept": "text/html"
    })
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            html = resp.read().decode("utf-8", errors="replace")

        title_match = re.search(r'<h1[^>]*>([^<]+)</h1>', html)
        title = title_match.group(1).strip() if title_match else path.split('/')[-1].replace('-', ' ').title()

        # Extract paragraphs
        paragraphs = re.findall(r'<p[^>]*>(.*?)</p>', html, re.DOTALL)
        body = []
        for p in paragraphs:
            clean = re.sub(r'<[^>]+>', '', p).strip()
            if len(clean) > 30:
                body.append(clean)

        text = '\n\n'.join(body)

        # Extract sections
        sections = re.findall(r'<h2[^>]*>([^<]+)</h2>', html)

        print(f"\n{'='*60}")
        print(f"  {title}")
        print(f"  Encyclopædia Britannica — Expertos verificados")
        print(f"{'='*60}")
        print(text[:5000])
        if len(text) > 5000:
            print(f"\n... [{len(text)} caracteres total]")

        if sections:
            print(f"\n--- Secciones ---")
            for s in sections:
                print(f"  • {s.strip()}")
        return text
    except Exception as e:
        print(f"  Error: {e}")
        return ""

def topics():
    cats = [
        "Física", "Matemáticas", "Química", "Biología",
        "Historia mundial", "Filosofía", "Tecnología", "Computación",
        "Astronomía", "Medicina", "Arte", "Música", "Literatura",
        "Geografía", "Economía", "Psicología", "Derecho", "Política"
    ]
    print("  Temas Britannica:")
    for c in cats:
        print(f"    - {c}")

def main():
    if len(sys.argv) < 2:
        print("Usage: tritos_britannica.py <command> [args]")
        print("  search <query>       — Buscar en Britannica")
        print("  read <path>          — Leer artículo")
        print("  topics               — Temas principales")
        return

    cmd = sys.argv[1]
    if cmd == "search" and len(sys.argv) > 2:
        query = " ".join(sys.argv[2:])
        print(f"  Buscando en Britannica: {query}")
        search(query)
    elif cmd == "read" and len(sys.argv) > 2:
        path = sys.argv[2]
        if not path.startswith("/"):
            path = "/" + path
        read_article(path)
    elif cmd == "topics":
        topics()

if __name__ == "__main__":
    main()
