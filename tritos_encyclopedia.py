#!/usr/bin/env python3
"""TRITOS Encyclopedia — Wikipedia API integration"""
import sys
import json
import urllib.request
import urllib.parse

API_URL = "https://es.wikipedia.org/w/api.php"

def search(query, limit=5):
    params = {
        "action": "query",
        "list": "search",
        "srsearch": query,
        "srlimit": str(limit),
        "format": "json"
    }
    url = API_URL + "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/1.0 (encyclopedia)"})
    with urllib.request.urlopen(req, timeout=10) as resp:
        data = json.loads(resp.read())
    results = data.get("query", {}).get("search", [])
    for r in results:
        print(f"  [{r['pageid']}] {r['title']}")
        snippet = r.get('snippet', '')
        # Strip HTML
        import re
        snippet = re.sub(r'<[^>]+>', '', snippet)
        print(f"    {snippet[:120]}...")
    return results

def get_article(title):
    params = {
        "action": "parse",
        "page": title,
        "prop": "text|sections|categories|links",
        "format": "json"
    }
    url = API_URL + "?" + urllib.parse.urlencode(params)
    with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "TRITOS/1.0"}), timeout=10) as resp:
        data = json.loads(resp.read())

    parse = data.get("parse", {})
    title = parse.get("title", "?")
    text = parse.get("text", {}).get("*", "")
    sections = parse.get("sections", [])
    categories = parse.get("categories", [])

    # Strip HTML
    import re
    text = re.sub(r'<script[^>]*>.*?</script>', '', text, flags=re.DOTALL)
    text = re.sub(r'<style[^>]*>.*?</style>', '', text, flags=re.DOTALL)
    text = re.sub(r'<[^>]+>', '', text)
    text = re.sub(r'&[a-z]+;', ' ', text)
    text = re.sub(r'&#\d+;', '', text)
    text = re.sub(r'\n{3,}', '\n\n', text)
    text = text.strip()

    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")
    print(text[:3000])
    if len(text) > 3000:
        print(f"\n... [{len(text)} caracteres total]")

    if sections:
        print(f"\n--- Secciones ---")
        for s in sections:
            indent = "  " * (int(s.get("toclevel", 1)) - 1)
            print(f"  {indent}{s['line']}")

    if categories:
        print(f"\n--- Categorías ---")
        cats = [c['*'] for c in categories[:10]]
        print(f"  {', '.join(cats)}")

    return text

def get_random():
    params = {
        "action": "query",
        "list": "random",
        "rnlimit": "1",
        "rnnamespace": "0",
        "format": "json"
    }
    url = API_URL + "?" + urllib.parse.urlencode(params)
    with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "TRITOS/1.0"}), timeout=10) as resp:
        data = json.loads(resp.read())
    pages = data.get("query", {}).get("random", [])
    if pages:
        return pages[0]["title"]
    return None

def main():
    if len(sys.argv) < 2:
        print("Usage: tritos_encyclopedia.py <command> [args]")
        print("  search <query>       — Buscar artículos")
        print("  read <title>         — Leer artículo completo")
        print("  random               — Artículo aleatorio")
        print("  topics               — Temas principales")
        return

    cmd = sys.argv[1]

    if cmd == "search" and len(sys.argv) > 2:
        query = " ".join(sys.argv[2:])
        print(f"  Buscando: {query}")
        search(query)
    elif cmd == "read" and len(sys.argv) > 2:
        title = " ".join(sys.argv[2:])
        get_article(title)
    elif cmd == "random":
        title = get_random()
        if title:
            print(f"  Artículo aleatorio: {title}")
            get_article(title)
    elif cmd == "topics":
        topics = [
            "Ciencia", "Matemáticas", "Física", "Química", "Biología",
            "Historia", "Filosofía", "Tecnología", "Computación",
            "Astronomía", "Medicina", "Arte", "Música", "Literatura",
            "Geografía", "Economía", "Psicología", "Sociología"
        ]
        print("  Temas disponibles:")
        for t in topics:
            print(f"    - {t}")
    else:
        print(f"  Comando desconocido: {cmd}")

if __name__ == "__main__":
    main()
