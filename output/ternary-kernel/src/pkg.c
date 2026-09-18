/**
 * pkg.c — Gestor de paquetes para kernel ternario ancestral
 *
 * Instalar, actualizar, eliminar software desde repositorio
 */

#include "../include/ternary.h"

// Package structure
#define PKG_MAX_NAME    32
#define PKG_MAX_VER     16
#define PKG_MAX_DEPS    4
#define PKG_MAX_REPO    8
#define PKG_MAX_PKGS    64

typedef struct {
    char     name[PKG_MAX_NAME];
    char     version[PKG_MAX_VER];
    char     description[64];
    uint32_t size;       // bytes
    uint8_t  installed;
    uint8_t  deps[PKG_MAX_DEPS];
    uint8_t  n_deps;
} package_t;

typedef struct {
    char     name[32];
    char     url[128];
    uint8_t  active;
} repo_t;

static package_t packages[PKG_MAX_PKGS];
static repo_t repos[PKG_MAX_REPO];
static uint8_t n_packages = 0;
static uint8_t n_repos = 0;

// Initialize package manager
void pkg_init(void) {
    vga_puts("[PKG] Package manager initialized\n");
    
    // Add default repository
    strcpy_t(repos[0].name, "tritos-main");
    strcpy_t(repos[0].url, "https://packages.tritos.org/main");
    repos[0].active = 1;
    n_repos = 1;
    
    // Pre-install some base packages
    strcpy_t(packages[0].name, "kernel");
    strcpy_t(packages[0].version, "3.1.0");
    strcpy_t(packages[0].description, "Ternary Ancestral Kernel");
    packages[0].size = 41000;
    packages[0].installed = 1;
    packages[0].n_deps = 0;
    
    strcpy_t(packages[1].name, "shell");
    strcpy_t(packages[1].version, "2.0.0");
    strcpy_t(packages[1].description, "Ternary Shell");
    packages[1].size = 12000;
    packages[1].installed = 1;
    packages[1].n_deps = 0;
    
    strcpy_t(packages[2].name, "netutils");
    strcpy_t(packages[2].version, "1.0.0");
    strcpy_t(packages[2].description, "Network utilities");
    packages[2].size = 8000;
    packages[2].installed = 1;
    packages[2].n_deps = 0;
    
    n_packages = 3;
}

// Search packages
void pkg_search(const char* query) {
    vga_puts("\n  Search results for '");
    vga_puts(query);
    vga_puts("':\n\n");
    
    uint8_t found = 0;
    for (uint8_t i = 0; i < n_packages; i++) {
        // Simple string match
        uint8_t match = 1;
        const char* q = query;
        const char* n = packages[i].name;
        while (*q && *n) {
            if (*q != *n) { match = 0; break; }
            q++; n++;
        }
        
        if (match) {
            vga_puts("  ");
            vga_puts(packages[i].name);
            vga_puts(" (");
            vga_puts(packages[i].version);
            vga_puts(") - ");
            vga_puts(packages[i].description);
            if (packages[i].installed) {
                vga_set_color(0x0A, 0);
                vga_puts(" [installed]");
                vga_set_color(0x07, 0);
            }
            vga_puts("\n");
            found++;
        }
    }
    
    if (!found) {
        vga_puts("  No packages found\n");
    }
}

// List installed packages
void pkg_list(void) {
    vga_puts("\n  Installed packages:\n\n");
    
    uint8_t count = 0;
    for (uint8_t i = 0; i < n_packages; i++) {
        if (packages[i].installed) {
            vga_puts("  ");
            vga_puts(packages[i].name);
            vga_puts(" ");
            vga_puts(packages[i].version);
            vga_puts(" - ");
            vga_puts(packages[i].description);
            vga_puts("\n");
            count++;
        }
    }
    
    vga_puts("\n  Total: ");
    { char nb[4]; num_to_str(count, nb); vga_puts(nb); }
    vga_puts(" packages\n");
}

// Install package (simulated)
int8_t pkg_install(const char* name) {
    vga_puts("[PKG] Installing ");
    vga_puts(name);
    vga_puts("...\n");
    
    // Check if already installed
    for (uint8_t i = 0; i < n_packages; i++) {
        if (strcmp_t(packages[i].name, name) == 0) {
            if (packages[i].installed) {
                vga_puts("[PKG] Already installed\n");
                return 0;
            }
        }
    }
    
    // Check dependencies
    for (uint8_t i = 0; i < n_packages; i++) {
        if (strcmp_t(packages[i].name, name) == 0) {
            for (uint8_t d = 0; d < packages[i].n_deps; d++) {
                uint8_t dep_idx = packages[i].deps[d];
                if (dep_idx < n_packages && !packages[dep_idx].installed) {
                    vga_puts("[PKG] Installing dependency: ");
                    vga_puts(packages[dep_idx].name);
                    vga_puts("\n");
                    packages[dep_idx].installed = 1;
                }
            }
        }
    }
    
    // Simulate download
    vga_puts("[PKG] Downloading from ");
    vga_puts(repos[0].name);
    vga_puts("...\n");
    
    // Simulate installation
    vga_puts("[PKG] Extracting...\n");
    vga_puts("[PKG] Configuring...\n");
    
    // Add package if not found
    uint8_t found = 0;
    for (uint8_t i = 0; i < n_packages; i++) {
        if (strcmp_t(packages[i].name, name) == 0) {
            packages[i].installed = 1;
            found = 1;
            break;
        }
    }
    
    if (!found && n_packages < PKG_MAX_PKGS) {
        strcpy_t(packages[n_packages].name, name);
        strcpy_t(packages[n_packages].version, "1.0.0");
        strcpy_t(packages[n_packages].description, "User package");
        packages[n_packages].size = 10000;
        packages[n_packages].installed = 1;
        packages[n_packages].n_deps = 0;
        n_packages++;
    }
    
    vga_set_color(0x0A, 0);
    vga_puts("[PKG] ");
    vga_puts(name);
    vga_puts(" installed successfully\n");
    vga_set_color(0x07, 0);
    
    return 0;
}

// Remove package
int8_t pkg_remove(const char* name) {
    vga_puts("[PKG] Removing ");
    vga_puts(name);
    vga_puts("...\n");
    
    // Don't remove kernel or shell
    if (strcmp_t(name, "kernel") == 0 || strcmp_t(name, "shell") == 0) {
        vga_puts("[PKG] Cannot remove core packages\n");
        return -1;
    }
    
    for (uint8_t i = 0; i < n_packages; i++) {
        if (strcmp_t(packages[i].name, name) == 0) {
            if (!packages[i].installed) {
                vga_puts("[PKG] Not installed\n");
                return -1;
            }
            packages[i].installed = 0;
            vga_puts("[PKG] Removed\n");
            return 0;
        }
    }
    
    vga_puts("[PKG] Package not found\n");
    return -1;
}

// Update packages
void pkg_update(void) {
    vga_puts("[PKG] Updating package lists...\n");
    
    for (uint8_t r = 0; r < n_repos; r++) {
        if (repos[r].active) {
            vga_puts("[PKG] Fetching from ");
            vga_puts(repos[r].name);
            vga_puts("...\n");
        }
    }
    
    vga_puts("[PKG] Done\n");
}

// Upgrade packages
void pkg_upgrade(void) {
    vga_puts("[PKG] Checking for upgrades...\n");
    
    uint8_t upgrades = 0;
    for (uint8_t i = 0; i < n_packages; i++) {
        if (packages[i].installed) {
            // Simulate version check
            vga_puts("[PKG] ");
            vga_puts(packages[i].name);
            vga_puts(" ");
            vga_puts(packages[i].version);
            vga_puts(" is up to date\n");
        }
    }
    
    if (upgrades == 0) {
        vga_puts("[PKG] All packages up to date\n");
    }
}

// Show package info
void pkg_info(const char* name) {
    for (uint8_t i = 0; i < n_packages; i++) {
        if (strcmp_t(packages[i].name, name) == 0) {
            vga_puts("\n  Package: ");
            vga_puts(packages[i].name);
            vga_puts("\n  Version: ");
            vga_puts(packages[i].version);
            vga_puts("\n  Description: ");
            vga_puts(packages[i].description);
            vga_puts("\n  Size: ");
            { char nb[8]; num_to_str(packages[i].size, nb); vga_puts(nb); }
            vga_puts(" bytes");
            vga_puts("\n  Status: ");
            if (packages[i].installed) {
                vga_set_color(0x0A, 0);
                vga_puts("installed");
                vga_set_color(0x07, 0);
            } else {
                vga_set_color(0x0C, 0);
                vga_puts("not installed");
                vga_set_color(0x07, 0);
            }
            vga_puts("\n");
            return;
        }
    }
    
    vga_puts("[PKG] Package not found\n");
}

// Add repository
void pkg_add_repo(const char* name, const char* url) {
    if (n_repos >= PKG_MAX_REPO) {
        vga_puts("[PKG] Too many repositories\n");
        return;
    }
    
    strcpy_t(repos[n_repos].name, name);
    strcpy_t(repos[n_repos].url, url);
    repos[n_repos].active = 1;
    n_repos++;
    
    vga_puts("[PKG] Repository added: ");
    vga_puts(name);
    vga_puts("\n");
}

// List repositories
void pkg_list_repos(void) {
    vga_puts("\n  Repositories:\n\n");
    
    for (uint8_t i = 0; i < n_repos; i++) {
        vga_puts("  ");
        vga_puts(repos[i].active ? "[x]" : "[ ]");
        vga_puts(" ");
        vga_puts(repos[i].name);
        vga_puts(" - ");
        vga_puts(repos[i].url);
        vga_puts("\n");
    }
}
