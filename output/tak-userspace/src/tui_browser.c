#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>

#define MAX_LINKS 256
#define MAX_URL 2048
#define MAX_LINE 1024
#define MAX_HISTORY 64
#define MAX_PAGES 16

typedef struct {
    int id;
    char href[MAX_URL];
    char text[256];
} Link;

typedef struct {
    char url[MAX_URL];
    char title[256];
    char *content;
    int content_len;
    Link links[MAX_LINKS];
    int num_links;
    int scroll_y;
} Page;

static Page pages[MAX_PAGES];
static int num_pages = 0;
static int current_page = -1;
static char history[MAX_HISTORY][MAX_URL];
static int history_pos = -1;
static int history_count = 0;

static int term_cols = 80;
static int term_rows = 24;

static void get_term_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        term_cols = ws.ws_col;
        term_rows = ws.ws_row;
    }
}

static void cursor_move(int row, int col) {
    printf("\033[%d;%dH", row + 1, col + 1);
}

static void cursor_hide(void) { printf("\033[?25l"); }
static void cursor_show(void) { printf("\033[?25h"); }
static void clear_screen(void) { printf("\033[2J"); }
static void clear_line(void) { printf("\033[2K"); }
static void set_bold(void) { printf("\033[1m"); }
static void set_underline(void) { printf("\033[4m"); }
static void set_color(int fg, int bg) { printf("\033[%d;%dm", fg, bg); }
static void reset_style(void) { printf("\033[0m"); }

static int fetch_url(const char *url, const char *outfile) {
    char cmd[MAX_URL + 256];
    snprintf(cmd, sizeof(cmd),
        "curl -sL --max-time 15 -A 'TRITOS/1.0' '%s' > '%s' 2>/dev/null",
        url, outfile);
    return system(cmd);
}

static int extract_links_from_file(const char *filepath, Link *links) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    char line[MAX_LINE * 4];
    int count = 0;

    while (fgets(line, sizeof(line), f) && count < MAX_LINKS) {
        char *p = line;
        while ((p = strstr(p, "<a ")) != NULL) {
            char *href_start = strstr(p, "href=\"");
            if (!href_start) { p += 3; continue; }
            href_start += 6;

            char *href_end = strchr(href_start, '"');
            if (!href_end) { p = href_start; continue; }

            int href_len = href_end - href_start;
            if (href_len >= MAX_URL) { p = href_end; continue; }

            char *text_start = strchr(p, '>');
            if (!text_start) { p = href_end; continue; }
            text_start++;

            char *text_end = strstr(text_start, "</a>");
            if (!text_end) { p = href_end; continue; }

            int text_len = text_end - text_start;
            if (text_len >= 256) text_len = 255;
            if (text_len <= 0) { p = text_end + 3; continue; }

            memcpy(links[count].href, href_start, href_len);
            links[count].href[href_len] = '\0';

            memcpy(links[count].text, text_start, text_len);
            links[count].text[text_len] = '\0';

            // Strip HTML entities and tags from text
            char *t = links[count].text;
            char clean[256];
            int ci = 0;
            int in_tag = 0;
            for (int i = 0; t[i] && ci < 255; i++) {
                if (t[i] == '<') { in_tag = 1; continue; }
                if (t[i] == '>') { in_tag = 0; continue; }
                if (!in_tag) {
                    if (t[i] == '&') {
                        if (strncmp(&t[i], "&amp;", 5) == 0) { clean[ci++] = '&'; i += 4; }
                        else if (strncmp(&t[i], "&lt;", 4) == 0) { clean[ci++] = '<'; i += 3; }
                        else if (strncmp(&t[i], "&gt;", 4) == 0) { clean[ci++] = '>'; i += 3; }
                        else if (strncmp(&t[i], "&quot;", 6) == 0) { clean[ci++] = '"'; i += 5; }
                        else if (strncmp(&t[i], "&#39;", 5) == 0) { clean[ci++] = '\''; i += 4; }
                        else if (strncmp(&t[i], "&nbsp;", 6) == 0) { clean[ci++] = ' '; i += 5; }
                        else { clean[ci++] = t[i]; }
                    } else {
                        clean[ci++] = t[i];
                    }
                }
            }
            clean[ci] = '\0';

            // Trim whitespace
            while (ci > 0 && (clean[ci-1] == ' ' || clean[ci-1] == '\n' || clean[ci-1] == '\r'))
                clean[--ci] = '\0';

            if (ci > 0) {
                strncpy(links[count].text, clean, 255);
                links[count].id = count + 1;
                count++;
            }

            p = text_end + 3;
        }
    }

    fclose(f);
    return count;
}

static char *strip_html(const char *html, int *out_len) {
    int len = strlen(html);
    char *text = malloc(len + 1);
    if (!text) return NULL;

    int j = 0;
    int in_tag = 0;
    int in_script = 0;
    int in_style = 0;

    for (int i = 0; i < len; i++) {
        if (strncasecmp(&html[i], "<script", 7) == 0) { in_script = 1; i += 6; continue; }
        if (strncasecmp(&html[i], "</script>", 9) == 0) { in_script = 0; i += 8; continue; }
        if (strncasecmp(&html[i], "<style", 6) == 0) { in_style = 1; i += 5; continue; }
        if (strncasecmp(&html[i], "</style>", 8) == 0) { in_style = 0; i += 7; continue; }

        if (in_script || in_style) continue;

        if (html[i] == '<') {
            in_tag = 1;
            // Add newline for block elements
            if (strncasecmp(&html[i], "<br", 3) == 0 ||
                strncasecmp(&html[i], "<p>", 3) == 0 ||
                strncasecmp(&html[i], "<div", 4) == 0 ||
                strncasecmp(&html[i], "<li", 3) == 0 ||
                strncasecmp(&html[i], "<h", 2) == 0 ||
                strncasecmp(&html[i], "<tr", 3) == 0) {
                text[j++] = '\n';
            }
            continue;
        }
        if (html[i] == '>') { in_tag = 0; continue; }
        if (in_tag) continue;

        // Decode common entities
        if (html[i] == '&') {
            if (strncmp(&html[i], "&amp;", 5) == 0) { text[j++] = '&'; i += 4; }
            else if (strncmp(&html[i], "&lt;", 4) == 0) { text[j++] = '<'; i += 3; }
            else if (strncmp(&html[i], "&gt;", 4) == 0) { text[j++] = '>'; i += 3; }
            else if (strncmp(&html[i], "&quot;", 6) == 0) { text[j++] = '"'; i += 5; }
            else if (strncmp(&html[i], "&#39;", 5) == 0) { text[j++] = '\''; i += 4; }
            else if (strncmp(&html[i], "&nbsp;", 6) == 0) { text[j++] = ' '; i += 5; }
            else if (strncmp(&html[i], "&ndash;", 7) == 0) { text[j++] = '-'; i += 6; }
            else if (strncmp(&html[i], "&mdash;", 7) == 0) { text[j++] = '-'; i += 6; }
            else if (strncmp(&html[i], "&eacute;", 8) == 0) { text[j++] = 'e'; i += 7; }
            else if (strncmp(&html[i], "&aacute;", 8) == 0) { text[j++] = 'a'; i += 7; }
            else if (strncmp(&html[i], "&oacute;", 8) == 0) { text[j++] = 'o'; i += 7; }
            else if (strncmp(&html[i], "&uacute;", 8) == 0) { text[j++] = 'u'; i += 7; }
            else if (strncmp(&html[i], "&ntilde;", 8) == 0) { text[j++] = 'n'; i += 7; }
            else if (strncmp(&html[i], "&iquest;", 8) == 0) { text[j++] = '?'; i += 7; }
            else if (strncmp(&html[i], "&iexcl;", 7) == 0) { text[j++] = '!'; i += 6; }
            else if (strncmp(&html[i], "&laquo;", 7) == 0) { text[j++] = '<'; i += 6; }
            else if (strncmp(&html[i], "&raquo;", 7) == 0) { text[j++] = '>'; i += 6; }
            else if (strncmp(&html[i], "&rsquo;", 7) == 0) { text[j++] = '\''; i += 6; }
            else if (strncmp(&html[i], "&ldquo;", 7) == 0) { text[j++] = '"'; i += 6; }
            else if (strncmp(&html[i], "&rdquo;", 7) == 0) { text[j++] = '"'; i += 6; }
            else if (html[i+1] == '#') {
                int code = atoi(&html[i+2]);
                if (code > 0 && code < 128) { text[j++] = (char)code; }
                else { text[j++] = '?'; }
                while (html[i] && html[i] != ';') i++;
            }
            else { text[j++] = html[i]; }
        } else if (html[i] == '\n' || html[i] == '\r' || html[i] == '\t') {
            text[j++] = ' ';
        } else {
            text[j++] = html[i];
        }
    }

    text[j] = '\0';

    // Collapse multiple spaces
    int k = 0;
    for (int i = 0; i < j; i++) {
        if (text[i] == ' ' && i > 0 && text[i-1] == ' ') continue;
        text[k++] = text[i];
    }
    text[k] = '\0';

    *out_len = k;
    return text;
}

static void resolve_url(const char *base, const char *rel, char *out, int outsize) {
    if (strncmp(rel, "http://", 7) == 0 || strncmp(rel, "https://", 8) == 0) {
        strncpy(out, rel, outsize - 1);
        out[outsize - 1] = '\0';
        return;
    }

    // Find last / in base
    const char *last_slash = strrchr(base, '/');
    if (!last_slash) last_slash = base + strlen(base);

    if (rel[0] == '/') {
        // Absolute path
        const char *scheme_end = strstr(base, "://");
        if (scheme_end) {
            scheme_end += 3;
            const char *host_start = scheme_end;
            const char *path_start = strchr(scheme_end, '/');
            if (path_start) {
                snprintf(out, outsize, "%.*s%s", (int)(path_start - base), base, rel);
            } else {
                snprintf(out, outsize, "%s%s", base, rel);
            }
        } else {
            snprintf(out, outsize, "%s", rel);
        }
    } else {
        // Relative path
        snprintf(out, outsize, "%.*s/%s", (int)(last_slash - base), base, rel);
    }
}

static void render_page(void) {
    if (current_page < 0 || current_page >= num_pages) return;

    Page *pg = &pages[current_page];
    get_term_size();
    clear_screen();

    // Header bar
    cursor_move(0, 0);
    set_color(37, 44);
    clear_line();
    printf(" \xe2\x96\x84 TRITOS Browser \xe2\x96\x84 ");
    reset_style();

    // URL bar
    cursor_move(1, 0);
    set_color(33, 40);
    clear_line();
    printf(" URL: %s", pg->url);
    reset_style();

    // Title
    cursor_move(2, 0);
    set_bold();
    set_color(37, 40);
    if (pg->title[0]) {
        printf(" %s", pg->title);
    }
    reset_style();

    // Separator
    cursor_move(3, 0);
    set_color(90, 40);
    for (int i = 0; i < term_cols; i++) printf("\xe2\x94\x80");
    reset_style();

    // Content
    if (pg->content) {
        int content_row = 4;
        int visible_rows = term_rows - 7; // header + url + title + sep + status + links
        char *text = pg->content;
        int text_len = pg->content_len;

        // Word wrap and display
        int row = 0;
        int col = 0;
        int line_start = 0;

        for (int i = 0; i < text_len && (row - pg->scroll_y) < visible_rows; i++) {
            if (text[i] == '\n' || col >= term_cols - 2) {
                if (row >= pg->scroll_y) {
                    cursor_move(content_row + row - pg->scroll_y, 0);
                    clear_line();
                    // Print the line
                    int len = i - line_start;
                    if (len > term_cols - 2) len = term_cols - 2;
                    if (len > 0) {
                        set_color(37, 40);
                        printf(" %.*s", len, &text[line_start]);
                        reset_style();
                    }
                }
                row++;
                col = 0;
                line_start = i + 1;
                if (text[i] == '\n') continue;
            } else {
                col++;
            }
        }
        // Print last line
        if (line_start < text_len && row >= pg->scroll_y && (row - pg->scroll_y) < visible_rows) {
            cursor_move(content_row + row - pg->scroll_y, 0);
            clear_line();
            int len = text_len - line_start;
            if (len > term_cols - 2) len = term_cols - 2;
            if (len > 0) {
                set_color(37, 40);
                printf(" %.*s", len, &text[line_start]);
                reset_style();
            }
        }
    }

    // Links section
    if (pg->num_links > 0) {
        int link_row = term_rows - 3;
        cursor_move(link_row, 0);
        set_color(90, 40);
        for (int i = 0; i < term_cols; i++) printf("\xe2\x94\x80");
        reset_style();

        cursor_move(link_row + 1, 0);
        set_color(36, 40);
        set_bold();
        printf(" Links:\n");
        reset_style();

        int max_links_display = 8;
        for (int i = 0; i < pg->num_links && i < max_links_display; i++) {
            cursor_move(link_row + 2 + i, 0);
            set_color(33, 40);
            printf("  [%d]", pg->links[i].id);
            reset_style();
            set_color(37, 40);
            printf(" %s", pg->links[i].text);
            reset_style();
        }
    }

    // Status bar
    cursor_move(term_rows - 1, 0);
    set_color(37, 44);
    clear_line();
    printf(" q:quit  r:reload  b:back  f:forward  [N]:goto link  /:search  s:scroll");
    reset_style();

    cursor_move(term_rows - 1, term_cols - 1);
    reset_style();
}

static void add_history(const char *url) {
    if (history_count < MAX_HISTORY) {
        strncpy(history[history_count], url, MAX_URL - 1);
        history_count++;
        history_pos = history_count - 1;
    }
}

static int navigate_to(const char *url) {
    if (num_pages >= MAX_PAGES) {
        // Remove oldest
        if (pages[0].content) free(pages[0].content);
        for (int i = 1; i < num_pages; i++) pages[i-1] = pages[i];
        num_pages--;
        if (current_page > 0) current_page--;
    }

    Page *pg = &pages[num_pages];
    memset(pg, 0, sizeof(Page));
    strncpy(pg->url, url, MAX_URL - 1);

    // Fetch
    char tmpfile[] = "/tmp/tak_browser_XXXXXX";
    int fd = mkstemp(tmpfile);
    if (fd < 0) return -1;
    close(fd);

    cursor_move(term_rows - 1, 0);
    set_color(33, 40);
    printf(" Fetching: %s ...", url);
    reset_style();
    fflush(stdout);

    if (fetch_url(url, tmpfile) != 0) {
        unlink(tmpfile);
        return -1;
    }

    // Read file
    struct stat st;
    if (stat(tmpfile, &st) != 0 || st.st_size == 0) {
        unlink(tmpfile);
        return -1;
    }

    FILE *f = fopen(tmpfile, "r");
    if (!f) { unlink(tmpfile); return -1; }

    pg->content = malloc(st.st_size + 1);
    if (!pg->content) { fclose(f); unlink(tmpfile); return -1; }

    pg->content_len = fread(pg->content, 1, st.st_size, f);
    pg->content[pg->content_len] = '\0';
    fclose(f);

    // Extract title
    char *title_start = strcasestr(pg->content, "<title>");
    if (title_start) {
        title_start += 7;
        char *title_end = strcasestr(title_start, "</title>");
        if (title_end) {
            int tlen = title_end - title_start;
            if (tlen >= 256) tlen = 255;
            memcpy(pg->title, title_start, tlen);
            pg->title[tlen] = '\0';
        }
    }

    // Extract links
    pg->num_links = extract_links_from_file(tmpfile, pg->links);

    // Strip HTML for display
    int text_len;
    char *text = strip_html(pg->content, &text_len);
    if (text) {
        free(pg->content);
        pg->content = text;
        pg->content_len = text_len;
    }

    pg->scroll_y = 0;
    current_page = num_pages;
    num_pages++;

    add_history(url);
    unlink(tmpfile);

    cursor_move(term_rows - 1, 0);
    set_color(32, 40);
    printf(" %d links found. ", pg->num_links);
    reset_style();

    return 0;
}

static void cmd_tui_browser(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: tritos-browser <url>\n");
        printf("  Navegador web TUI de TRITOS\n");
        printf("  Comandos: q=Salir r=Recargar b=Atrás f=Adelante [N]=Ir a link /=Buscar\n");
        return;
    }

    // Initialize
    memset(pages, 0, sizeof(pages));
    num_pages = 0;
    current_page = -1;
    history_count = 0;
    history_pos = -1;

    // Initial navigation
    if (navigate_to(argv[1]) != 0) {
        printf("Error: No se pudo cargar %s\n", argv[1]);
        return;
    }

    // Main loop
    cursor_hide();
    render_page();

    while (1) {
        int ch = getchar();

        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'r' || ch == 'R') {
            // Reload
            if (current_page >= 0) {
                navigate_to(pages[current_page].url);
                render_page();
            }
        } else if (ch == 'b' || ch == 'B') {
            // Back
            if (history_pos > 0) {
                history_pos--;
                // Find page in history
                for (int i = 0; i < num_pages; i++) {
                    if (strcmp(pages[i].url, history[history_pos]) == 0) {
                        current_page = i;
                        render_page();
                        break;
                    }
                }
            }
        } else if (ch == 'f' || ch == 'F') {
            // Forward
            if (history_pos < history_count - 1) {
                history_pos++;
                for (int i = 0; i < num_pages; i++) {
                    if (strcmp(pages[i].url, history[history_pos]) == 0) {
                        current_page = i;
                        render_page();
                        break;
                    }
                }
            }
        } else if (ch == 's' || ch == 'S') {
            // Scroll down
            if (current_page >= 0) {
                pages[current_page].scroll_y += 5;
                render_page();
            }
        } else if (ch >= '1' && ch <= '9') {
            // Go to link
            int link_id = ch - '0';
            if (current_page >= 0) {
                Page *pg = &pages[current_page];
                for (int i = 0; i < pg->num_links; i++) {
                    if (pg->links[i].id == link_id) {
                        char resolved[MAX_URL];
                        resolve_url(pg->url, pg->links[i].href, resolved, MAX_URL);
                        navigate_to(resolved);
                        render_page();
                        break;
                    }
                }
            }
        } else if (ch == '/') {
            // Search (simple)
            cursor_move(term_rows - 1, 0);
            set_color(37, 44);
            clear_line();
            printf(" Buscar: ");
            reset_style();
            fflush(stdout);

            char query[256] = {0};
            int qi = 0;
            while (1) {
                int c = getchar();
                if (c == '\n' || c == 27) break;
                if (c == 127 && qi > 0) { qi--; printf("\b \b"); continue; }
                if (qi < 255) { query[qi++] = c; printf("%c", c); }
            }
            query[qi] = '\0';

            // Search in page content
            if (current_page >= 0 && pages[current_page].content) {
                char *found = strcasestr(pages[current_page].content, query);
                if (found) {
                    // Calculate line number
                    int line = 1;
                    for (char *p = pages[current_page].content; p < found; p++) {
                        if (*p == '\n') line++;
                    }
                    pages[current_page].scroll_y = (line > 5) ? line - 5 : 0;
                }
            }
            render_page();
        } else if (ch == 'H' || ch == 'h') {
            // Home - go to tritos://home or about:blank
            navigate_to("about:blank");
            render_page();
        }

        // Process escape sequences (arrows)
        if (ch == 27) {
            int ch2 = getchar();
            int ch3 = getchar();
            if (ch2 == '[') {
                if (ch3 == 'A') {
                    // Up arrow
                    if (current_page >= 0 && pages[current_page].scroll_y > 0) {
                        pages[current_page].scroll_y--;
                        render_page();
                    }
                } else if (ch3 == 'B') {
                    // Down arrow
                    if (current_page >= 0) {
                        pages[current_page].scroll_y++;
                        render_page();
                    }
                } else if (ch3 == '5') {
                    // Page Up
                    getchar(); // consume ~
                    if (current_page >= 0) {
                        pages[current_page].scroll_y -= 10;
                        if (pages[current_page].scroll_y < 0)
                            pages[current_page].scroll_y = 0;
                        render_page();
                    }
                } else if (ch3 == '6') {
                    // Page Down
                    getchar(); // consume ~
                    if (current_page >= 0) {
                        pages[current_page].scroll_y += 10;
                        render_page();
                    }
                }
            }
        }
    }

    // Cleanup
    cursor_show();
    clear_screen();
    cursor_move(0, 0);

    for (int i = 0; i < num_pages; i++) {
        if (pages[i].content) free(pages[i].content);
    }
}

int main(int argc, char **argv) {
    cmd_tui_browser(argc, argv);
    return 0;
}
