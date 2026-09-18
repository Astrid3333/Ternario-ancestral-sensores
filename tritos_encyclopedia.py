#!/usr/bin/env python3
"""TRITOS Encyclopedia — Multi-source encyclopedia aggregator
Sources: Wikipedia, Encycloreader, Enciclopedia Libre, Enciclopèdia Catalana, Encyclopedia.com
"""
import sys
import json
import urllib.request
import urllib.parse
import re
import html
import time

# ═══════════════════════════════════════════════════════════
#  WIKIPEDIA (ES/EN)
# ═══════════════════════════════════════════════════════════

def wiki_search(query, limit=5, lang="es"):
    api = f"https://{lang}.wikipedia.org/w/api.php"
    params = {
        "action": "query", "list": "search",
        "srsearch": query, "srlimit": str(limit), "format": "json"
    }
    url = api + "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0 (encyclopedia)"})
    with urllib.request.urlopen(req, timeout=10) as resp:
        data = json.loads(resp.read())
    results = data.get("query", {}).get("search", [])
    out = []
    for r in results:
        snippet = re.sub(r'<[^>]+>', '', r.get('snippet', ''))
        out.append({
            "source": f"Wikipedia ({lang.upper()})",
            "id": r['pageid'], "title": r['title'],
            "snippet": snippet, "lang": lang
        })
    return out

def wiki_read(title, lang="es"):
    api = f"https://{lang}.wikipedia.org/w/api.php"
    params = {
        "action": "parse", "page": title,
        "prop": "text|sections|categories|links|revid", "format": "json"
    }
    url = api + "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0"})
    with urllib.request.urlopen(req, timeout=10) as resp:
        data = json.loads(resp.read())
    parse = data.get("parse", {})
    text = parse.get("text", {}).get("*", "")
    sections = parse.get("sections", [])
    categories = parse.get("categories", [])
    revid = parse.get("revid", 0)
    title = parse.get("title", "?")
    text = re.sub(r'<script[^>]*>.*?</script>', '', text, flags=re.DOTALL)
    text = re.sub(r'<style[^>]*>.*?</style>', '', text, flags=re.DOTALL)
    text = re.sub(r'<[^>]+>', '', text)
    text = html.unescape(text)
    text = re.sub(r'\n{3,}', '\n\n', text).strip()
    return {
        "source": f"Wikipedia ({lang.upper()})",
        "title": title, "text": text, "sections": sections,
        "categories": [c['*'] for c in categories[:15]],
        "url": f"https://{lang}.wikipedia.org/wiki/{urllib.parse.quote(title)}",
        "revid": revid
    }

def wiki_random(lang="es"):
    api = f"https://{lang}.wikipedia.org/w/api.php"
    params = {"action": "query", "list": "random", "rnlimit": "1", "rnnamespace": "0", "format": "json"}
    url = api + "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0"})
    with urllib.request.urlopen(req, timeout=10) as resp:
        data = json.loads(resp.read())
    pages = data.get("query", {}).get("random", [])
    return pages[0]["title"] if pages else None

# ═══════════════════════════════════════════════════════════
#  ENCYCLOREADER (Encyclosphere — 33 encyclopedias)
# ═══════════════════════════════════════════════════════════

def encycloreader_search(query, limit=5):
    url = f"https://encycloreader.org/find.php?query={urllib.parse.quote(query)}&limit={limit}"
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0 (encyclopedia)"})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            text = resp.read().decode('utf-8', errors='ignore')
        results = []
        for m in re.finditer(r'<a[^>]*href="(/[^"]+)"[^>]*>([^<]+)</a>', text):
            if len(results) >= limit:
                break
            title = html.unescape(m.group(2)).strip()
            if len(title) > 3:  # Skip short links
                results.append({
                    "source": "Encycloreader",
                    "url": "https://encycloreader.org" + m.group(1),
                    "title": title,
                    "snippet": ""
                })
        return results
    except Exception as e:
        return [{"source": "Encycloreader", "error": str(e)}]

def encycloreader_read(title):
    url = f"https://encycloreader.org/r/wikipedia.php?q={urllib.parse.quote(title)}"
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0"})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            text = resp.read().decode('utf-8', errors='ignore')
        body = re.search(r'<div[^>]*id="mw-content-text"[^>]*>(.*?)</div>\s*</div>', text, re.DOTALL)
        if not body:
            body = re.search(r'<article[^>]*>(.*?)</article>', text, re.DOTALL)
        content = body.group(1) if body else text[:5000]
        content = re.sub(r'<script[^>]*>.*?</script>', '', content, flags=re.DOTALL)
        content = re.sub(r'<style[^>]*>.*?</style>', '', content, flags=re.DOTALL)
        content = re.sub(r'<[^>]+>', ' ', content)
        content = html.unescape(content)
        content = re.sub(r'\s+', ' ', content).strip()
        return {"source": "Encycloreader", "title": title, "text": content[:4000], "url": url}
    except Exception as e:
        return {"source": "Encycloreader", "title": title, "error": str(e)}

# ═══════════════════════════════════════════════════════════
#  ENCICLOPEDIA LIBRE UNIVERSAL EN ESPAÑOL
# ═══════════════════════════════════════════════════════════

def enciclopalibre_search(query, limit=5):
    url = f"https://enciclopedia.us.es/index.php?search={urllib.parse.quote(query)}&title=Especial:Buscar&limit={limit}"
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0 (encyclopedia)"})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            text = resp.read().decode('utf-8', errors='ignore')
        results = []
        for m in re.finditer(r'<li[^>]*class="mw-search-result"[^>]*>.*?<a[^>]*href="(/[^"]+)"[^>]*>(.*?)</a>.*?<div[^>]*class="searchresult"[^>]*>(.*?)</div>', text, re.DOTALL):
            if len(results) >= limit:
                break
            results.append({
                "source": "Enciclopedia Libre",
                "url": "https://enciclopedia.us.es" + m.group(1),
                "title": re.sub(r'<[^>]+>', '', html.unescape(m.group(2))).strip(),
                "snippet": html.unescape(m.group(3))[:200]
            })
        return results
    except Exception as e:
        return [{"source": "Enciclopedia Libre", "error": str(e)}]

def enciclopalibre_read(title):
    url = f"https://enciclopedia.us.es/wiki/{urllib.parse.quote(title)}"
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0"})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            text = resp.read().decode('utf-8', errors='ignore')
        body = re.search(r'<div[^>]*id="mw-content-text"[^>]*>(.*?)</div>\s*</div>', text, re.DOTALL)
        content = body.group(1) if body else text[:5000]
        content = re.sub(r'<script[^>]*>.*?</script>', '', content, flags=re.DOTALL)
        content = re.sub(r'<style[^>]*>.*?</style>', '', content, flags=re.DOTALL)
        content = re.sub(r'<[^>]+>', ' ', content)
        content = html.unescape(content)
        content = re.sub(r'\s+', ' ', content).strip()
        return {"source": "Enciclopedia Libre", "title": title, "text": content[:4000], "url": url}
    except Exception as e:
        return {"source": "Enciclopedia Libre", "title": title, "error": str(e)}

# ═══════════════════════════════════════════════════════════
#  ENCYCLOPÈDIA CATALANA
# ═══════════════════════════════════════════════════════════

def enciclopediacatalana_search(query, limit=5):
    url = f"https://www.enciclopedia.cat/cerca/resultats?q={urllib.parse.quote(query)}&limit={limit}"
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0 (encyclopedia)"})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            text = resp.read().decode('utf-8', errors='ignore')
        results = []
        for m in re.finditer(r'<a[^>]*href="(https?://[^"]+)"[^>]*class="[^"]*titol[^"]*"[^>]*>(.*?)</a>.*?<p[^>]*>(.*?)</p>', text, re.DOTALL):
            if len(results) >= limit:
                break
            results.append({
                "source": "Enciclopèdia Catalana",
                "url": m.group(1),
                "title": re.sub(r'<[^>]+>', '', html.unescape(m.group(2))).strip(),
                "snippet": re.sub(r'<[^>]+>', '', html.unescape(m.group(3)))[:200]
            })
        return results
    except Exception as e:
        return [{"source": "Enciclopèdia Catalana", "error": str(e)}]

def enciclopediacatalana_read(title):
    url = f"https://www.enciclopedia.cat/{urllib.parse.quote(title)}"
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0"})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            text = resp.read().decode('utf-8', errors='ignore')
        body = re.search(r'<article[^>]*>(.*?)</article>', text, re.DOTALL)
        content = body.group(1) if body else text[:5000]
        content = re.sub(r'<script[^>]*>.*?</script>', '', content, flags=re.DOTALL)
        content = re.sub(r'<style[^>]*>.*?</style>', '', content, flags=re.DOTALL)
        content = re.sub(r'<[^>]+>', ' ', content)
        content = html.unescape(content)
        content = re.sub(r'\s+', ' ', content).strip()
        return {"source": "Enciclopèdia Catalana", "title": title, "text": content[:4000], "url": url}
    except Exception as e:
        return {"source": "Enciclopèdia Catalana", "title": title, "error": str(e)}

# ═══════════════════════════════════════════════════════════
#  ENCYCLOPEDIA.COM
# ═══════════════════════════════════════════════════════════

def encyclopediadotcom_search(query, limit=5):
    url = f"https://www.encyclopedia.com/searchresults?q={urllib.parse.quote(query)}"
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0 (encyclopedia)"})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            text = resp.read().decode('utf-8', errors='ignore')
        results = []
        for m in re.finditer(r'<a[^>]*href="(/[^"]+)"[^>]*class="[^"]*title[^"]*"[^>]*>(.*?)</a>', text, re.DOTALL):
            if len(results) >= limit:
                break
            results.append({
                "source": "Encyclopedia.com",
                "url": "https://www.encyclopedia.com" + m.group(1),
                "title": re.sub(r'<[^>]+>', '', html.unescape(m.group(2))).strip(),
                "snippet": ""
            })
        return results
    except Exception as e:
        return [{"source": "Encyclopedia.com", "error": str(e)}]

def encyclopediadotcom_read(title):
    url = f"https://www.encyclopedia.com/{urllib.parse.quote(title)}"
    req = urllib.request.Request(url, headers={"User-Agent": "TRITOS/2.0"})
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            text = resp.read().decode('utf-8', errors='ignore')
        body = re.search(r'<article[^>]*>(.*?)</article>', text, re.DOTALL)
        if not body:
            body = re.search(r'<div[^>]*class="[^"]*article[^"]*"[^>]*>(.*?)</div>', text, re.DOTALL)
        content = body.group(1) if body else text[:5000]
        content = re.sub(r'<script[^>]*>.*?</script>', '', content, flags=re.DOTALL)
        content = re.sub(r'<style[^>]*>.*?</style>', '', content, flags=re.DOTALL)
        content = re.sub(r'<[^>]+>', ' ', content)
        content = html.unescape(content)
        content = re.sub(r'\s+', ' ', content).strip()
        return {"source": "Encyclopedia.com", "title": title, "text": content[:4000], "url": url}
    except Exception as e:
        return {"source": "Encyclopedia.com", "title": title, "error": str(e)}

# ═══════════════════════════════════════════════════════════
#  AGGREGATOR — Busca en TODAS las fuentes
# ═══════════════════════════════════════════════════════════

SOURCES = {
    "wikipedia_es":   ("Wikipedia (ES)",     wiki_search, wiki_read, wiki_random),
    "wikipedia_en":   ("Wikipedia (EN)",     lambda q,l: wiki_search(q,l,"en"), lambda t: wiki_read(t,"en"), lambda: wiki_random("en")),
    "encycloreader":  ("Encycloreader",      encycloreader_search, encycloreader_read, None),
    "libre":          ("Enciclopedia Libre", enciclopalibre_search, enciclopalibre_read, None),
    "catalana":       ("Enciclop. Catalana", enciclopediacatalana_search, enciclopediacatalana_read, None),
    "encyclopedia_com":("Encyclopedia.com",  encyclopediadotcom_search, encyclopediadotcom_read, None),
}

def search_all(query, limit=3):
    all_results = []
    for key, (name, search_fn, _, _) in SOURCES.items():
        try:
            results = search_fn(query, limit)
            all_results.extend(results)
        except Exception as e:
            all_results.append({"source": name, "error": str(e)})
    return all_results

def read_all(title):
    articles = []
    for key, (name, _, read_fn, _) in SOURCES.items():
        try:
            article = read_fn(title)
            if article and "text" in article and len(article["text"]) > 50:
                articles.append(article)
        except Exception:
            pass
    return articles

def print_results(results):
    for i, r in enumerate(results, 1):
        src = r.get("source", "?")
        title = r.get("title", "?")
        snippet = r.get("snippet", "")
        error = r.get("error")
        if error:
            print(f"  {i}. [{src}] ERROR: {error[:80]}")
        else:
            print(f"  {i}. [{src}] {title}")
            if snippet:
                print(f"     {snippet[:120]}...")
            if "url" in r:
                print(f"     {r['url']}")

def print_article(article, full=False):
    src = article.get("source", "?")
    title = article.get("title", "?")
    text = article.get("text", "")
    url = article.get("url", "")
    sections = article.get("sections", [])
    categories = article.get("categories", [])

    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"  Fuente: {src}")
    if url:
        print(f"  URL: {url}")
    print(f"{'='*60}")

    if text:
        print(text[:3000] if not full else text[:6000])
        if len(text) > (3000 if not full else 6000):
            print(f"\n... [{len(text)} caracteres total]")

    if sections:
        print(f"\n--- Secciones ---")
        for s in sections:
            indent = "  " * (int(s.get("toclevel", 1)) - 1)
            print(f"  {indent}{s['line']}")

    if categories:
        print(f"\n--- Categorías ---")
        print(f"  {', '.join(categories[:10])}")

# ═══════════════════════════════════════════════════════════
#  CLI
# ═══════════════════════════════════════════════════════════

def main():
    if len(sys.argv) < 2:
        print("TRITOS Encyclopedia v2.0 — Multi-source")
        print("Usage: tritos_encyclopedia.py <command> [args]")
        print()
        print("Commands:")
        print("  search <query>        — Buscar en TODAS las fuentes")
        print("  search <src> <query>  — Buscar en una fuente")
        print("  read <title>          — Leer de TODAS las fuentes")
        print("  read <src> <title>    — Leer de una fuente")
        print("  random                — Artículo aleatorio (Wikipedia)")
        print("  sources               — Listar fuentes disponibles")
        print("  cite <title>          — Citar artículo (formato académico)")
        print("  topics                — Temas principales")
        return

    cmd = sys.argv[1]

    if cmd == "sources":
        print("  Fuentes disponibles:")
        print("  ─────────────────────────────────────────────")
        for key, (name, _, _, rnd) in SOURCES.items():
            random_info = " [random OK]" if rnd else ""
            print(f"    {key:20s} — {name}{random_info}")

    elif cmd == "search":
        if len(sys.argv) < 3:
            print("  Uso: search <query> o search <fuente> <query>")
            return
        # Check if second arg is a source
        if sys.argv[2] in SOURCES:
            src_key = sys.argv[2]
            query = " ".join(sys.argv[3:])
            name, search_fn, _, _ = SOURCES[src_key]
            print(f"  Buscando en {name}: {query}")
            results = search_fn(query, 5)
            print_results(results)
        else:
            query = " ".join(sys.argv[2:])
            print(f"  Buscando en TODAS las fuentes: {query}")
            results = search_all(query, 3)
            print_results(results)

    elif cmd == "read":
        if len(sys.argv) < 3:
            print("  Uso: read <title> o read <fuente> <title>")
            return
        if sys.argv[2] in SOURCES:
            src_key = sys.argv[2]
            title = " ".join(sys.argv[3:])
            _, _, read_fn, _ = SOURCES[src_key]
            article = read_fn(title)
            if article:
                print_article(article, full=True)
            else:
                print(f"  Artículo no encontrado en {SOURCES[src_key][0]}")
        else:
            title = " ".join(sys.argv[2:])
            print(f"\n  Leyendo de TODAS las fuentes: {title}")
            articles = read_all(title)
            if articles:
                for a in articles:
                    print_article(a)
            else:
                print(f"  Artículo no encontrado en ninguna fuente")

    elif cmd == "random":
        title = wiki_random()
        if title:
            print(f"  Artículo aleatorio: {title}")
            article = wiki_read(title)
            print_article(article)

    elif cmd == "cite":
        title = " ".join(sys.argv[2:]) if len(sys.argv) > 2 else None
        if not title:
            print("  Uso: cite <title>")
            return
        articles = read_all(title)
        if articles:
            print(f"\n{'='*60}")
            print(f"  CITAS BIBLIOGRÁFICAS: {title}")
            print(f"{'='*60}")
            for a in articles:
                src = a.get("source", "?")
                url = a.get("url", "")
                print(f"\n  [{src}]")
                print(f'    "{a.get("title", title)}"')
                if url:
                    print(f"    Disponible en: {url}")
                print(f"    Consultado: {time.strftime('%Y-%m-%d')}")
        else:
            print(f"  No se encontró: {title}")

    elif cmd == "topics":
        topics = [
            ("Ciencia", ["Física", "Química", "Biología", "Astronomía", "Matemáticas"]),
            ("Tecnología", ["Computación", "IA", "Internet", "Electrónica", "Robótica"]),
            ("Historia", ["Antigüedad", "Edad Media", "Modernidad", "Contemporánea"]),
            ("Cultura", ["Filosofía", "Arte", "Música", "Literatura", "Idiomas"]),
            ("Sociedad", ["Economía", "Política", "Educación", "Medicina", "Derecho"]),
        ]
        print("  Temas principales:")
        for cat, items in topics:
            print(f"\n  {cat}:")
            for t in items:
                print(f"    - {t}")

    else:
        print(f"  Comando desconocido: {cmd}")
        print("  Use: search, read, random, cite, sources, topics")

if __name__ == "__main__":
    main()
