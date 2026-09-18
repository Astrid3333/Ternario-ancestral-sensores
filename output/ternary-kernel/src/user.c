/**
 * user.c — Multi-User System para Tritos OS
 *
 * UIDs, GIDs, permisos, login, autenticación.
 */

#include "../include/ternary.h"

// Max users and groups
#define MAX_USERS   16
#define MAX_GROUPS  8
#define MAX_NAME    16
#define MAX_PASS    32

// User structure
typedef struct {
    uint8_t uid;
    uint8_t gid;
    char name[MAX_NAME];
    char pass_hash[MAX_PASS];
    char home[32];
    char shell[32];
    uint8_t active;
    uint8_t is_admin;
} user_t;

// Group structure
typedef struct {
    uint8_t gid;
    char name[MAX_NAME];
    uint8_t members[16];
    uint8_t member_count;
} group_t;

// File permissions (rwx rwx rwx)
typedef struct {
    uint8_t owner_uid;
    uint8_t owner_gid;
    uint8_t permissions; // rwxrwxrwx (9 bits)
} file_perm_t;

static user_t users[MAX_USERS];
static group_t groups[MAX_GROUPS];
static uint8_t user_count = 0;
static uint8_t group_count = 0;
static int8_t current_user = -1; // -1 = root

// =============================================================================
// Hash simple (para demo — en producción usar SHA-256)
// =============================================================================

static uint32_t simple_hash(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// =============================================================================
// User Management
// =============================================================================

int user_create(const char* name, const char* pass, uint8_t gid) {
    if (user_count >= MAX_USERS) return -1;

    // Check duplicate name
    for (int i = 0; i < user_count; i++) {
        if (strcmp_t(users[i].name, name) == 0) return -1;
    }

    user_t* u = &users[user_count];
    u->uid = user_count + 1;
    u->gid = gid;
    strncpy(u->name, name, MAX_NAME - 1);

    // Hash password
    uint32_t h = simple_hash(pass);
    u->pass_hash[0] = "0123456789abcdef"[(h >> 28) & 0xF];
    u->pass_hash[1] = "0123456789abcdef"[(h >> 24) & 0xF];
    u->pass_hash[2] = "0123456789abcdef"[(h >> 20) & 0xF];
    u->pass_hash[3] = "0123456789abcdef"[(h >> 16) & 0xF];
    u->pass_hash[4] = "0123456789abcdef"[(h >> 12) & 0xF];
    u->pass_hash[5] = "0123456789abcdef"[(h >> 8) & 0xF];
    u->pass_hash[6] = "0123456789abcdef"[(h >> 4) & 0xF];
    u->pass_hash[7] = "0123456789abcdef"[h & 0xF];
    u->pass_hash[8] = 0;

    // Default paths
    u->home[0] = '/'; u->home[1] = 'h'; u->home[2] = 'o'; u->home[3] = 'm';
    u->home[4] = 'e'; u->home[5] = '/'; u->home[6] = 0;
    { int len = 6; const char* n = name; while (*n && len < 30) { u->home[len++] = *n++; } u->home[len] = 0; }
    u->shell[0] = '/'; u->shell[1] = 'b'; u->shell[2] = 'i'; u->shell[3] = 'n';
    u->shell[4] = '/'; u->shell[5] = 's'; u->shell[6] = 'h'; u->shell[7] = 0;
    u->active = 1;
    u->is_admin = 0;

    user_count++;
    return u->uid;
}

int user_delete(const char* name) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp_t(users[i].name, name) == 0) {
            // Can't delete root
            if (users[i].uid == 0) return -1;

            users[i].active = 0;
            return 0;
        }
    }
    return -1;
}

int user_set_admin(const char* name, uint8_t admin) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp_t(users[i].name, name) == 0) {
            users[i].is_admin = admin;
            return 0;
        }
    }
    return -1;
}

// =============================================================================
// Authentication
// =============================================================================

int user_login(const char* name, const char* pass) {
    for (int i = 0; i < user_count; i++) {
        if (users[i].active && strcmp_t(users[i].name, name) == 0) {
            uint32_t h = simple_hash(pass);
            char hash_str[MAX_PASS];
            hash_str[0] = "0123456789abcdef"[(h >> 28) & 0xF];
            hash_str[1] = "0123456789abcdef"[(h >> 24) & 0xF];
            hash_str[2] = "0123456789abcdef"[(h >> 20) & 0xF];
            hash_str[3] = "0123456789abcdef"[(h >> 16) & 0xF];
            hash_str[4] = "0123456789abcdef"[(h >> 12) & 0xF];
            hash_str[5] = "0123456789abcdef"[(h >> 8) & 0xF];
            hash_str[6] = "0123456789abcdef"[(h >> 4) & 0xF];
            hash_str[7] = "0123456789abcdef"[h & 0xF];
            hash_str[8] = 0;

            if (strcmp_t(users[i].pass_hash, hash_str) == 0) {
                current_user = users[i].uid;
                vga_puts("[USER] Logged in as: ");
                vga_puts(name);
                vga_puts("\n");
                return users[i].uid;
            }
        }
    }
    vga_puts("[USER] Invalid credentials\n");
    return -1;
}

void user_logout(void) {
    if (current_user >= 0) {
        vga_puts("[USER] Logged out: ");
        vga_puts(users[current_user].name);
        vga_puts("\n");
    }
    current_user = -1;
}

int user_get_current(void) {
    return current_user;
}

const char* user_get_name(uint8_t uid) {
    if (uid == 0) return "root";
    for (int i = 0; i < user_count; i++) {
        if (users[i].uid == uid) return users[i].name;
    }
    return "unknown";
}

// =============================================================================
// Group Management
// =============================================================================

int group_create(const char* name) {
    if (group_count >= MAX_GROUPS) return -1;

    group_t* g = &groups[group_count];
    g->gid = group_count;
    strncpy(g->name, name, MAX_NAME - 1);
    g->member_count = 0;

    group_count++;
    return g->gid;
}

int group_add_member(const char* group_name, const char* user_name) {
    for (int i = 0; i < group_count; i++) {
        if (strcmp_t(groups[i].name, group_name) == 0) {
            for (int j = 0; j < user_count; j++) {
                if (strcmp_t(users[j].name, user_name) == 0) {
                    if (groups[i].member_count < 16) {
                        groups[i].members[groups[i].member_count++] = users[j].uid;
                        return 0;
                    }
                }
            }
        }
    }
    return -1;
}

int group_has_member(uint8_t gid, uint8_t uid) {
    for (int i = 0; i < group_count; i++) {
        if (groups[i].gid == gid) {
            for (int j = 0; j < groups[i].member_count; j++) {
                if (groups[i].members[j] == uid) return 1;
            }
        }
    }
    return 0;
}

// =============================================================================
// Permissions
// =============================================================================

void perm_set(file_perm_t* perm, uint8_t owner_uid, uint8_t owner_gid, uint8_t perms) {
    perm->owner_uid = owner_uid;
    perm->owner_gid = owner_gid;
    perm->permissions = perms;
}

int perm_check(file_perm_t* perm, uint8_t uid, uint8_t gid, uint8_t wanted) {
    // Root can do anything
    if (uid == 0) return 1;

    // Owner
    if (uid == perm->owner_uid) {
        uint8_t owner_perm = (perm->permissions >> 6) & 0x07;
        return (owner_perm & wanted) == wanted;
    }

    // Group
    if (gid == perm->owner_gid || group_has_member(perm->owner_gid, uid)) {
        uint8_t group_perm = (perm->permissions >> 3) & 0x07;
        return (group_perm & wanted) == wanted;
    }

    // Others
    uint8_t other_perm = perm->permissions & 0x07;
    return (other_perm & wanted) == wanted;
}

void perm_to_string(file_perm_t* perm, char* str) {
    str[0] = (perm->permissions & 0x04) ? 'r' : '-';
    str[1] = (perm->permissions & 0x02) ? 'w' : '-';
    str[2] = (perm->permissions & 0x01) ? 'x' : '-';
    str[3] = (perm->permissions >> 3 & 0x04) ? 'r' : '-';
    str[4] = (perm->permissions >> 3 & 0x02) ? 'w' : '-';
    str[5] = (perm->permissions >> 3 & 0x01) ? 'x' : '-';
    str[6] = (perm->permissions >> 6 & 0x04) ? 'r' : '-';
    str[7] = (perm->permissions >> 6 & 0x02) ? 'w' : '-';
    str[8] = (perm->permissions >> 6 & 0x01) ? 'x' : '-';
    str[9] = 0;
}

// =============================================================================
// Init
// =============================================================================

void user_init(void) {
    user_count = 0;
    group_count = 0;
    current_user = -1;

    // Create default groups
    group_create("root");
    group_create("users");
    group_create("admin");

    // Create root user
    user_create("root", "root", 0);
    users[0].is_admin = 1;

    // Create default user
    user_create("tritos", "tritos", 1);

    vga_puts("[USER] Multi-user initialized\n");
    vga_puts("[USER] Users: root, tritos\n");
}

// =============================================================================
// Status
// =============================================================================

void user_status(void) {
    vga_puts("\n  [Multi-User System]\n\n");

    vga_puts("  Current user: ");
    if (current_user >= 0) {
        vga_puts(user_get_name(current_user));
    } else {
        vga_puts("(root)");
    }
    vga_puts("\n");

    vga_puts("  Users: ");
    { char nb[2]; num_to_str(user_count, nb); vga_puts(nb); }
    vga_puts("\n");

    for (int i = 0; i < user_count; i++) {
        if (users[i].active) {
            vga_puts("    [");
            vga_putc(users[i].is_admin ? 'A' : ' ');
            vga_puts("] UID=");
            { char nb[2]; num_to_str(users[i].uid, nb); vga_puts(nb); }
            vga_puts(" GID=");
            { char nb[2]; num_to_str(users[i].gid, nb); vga_puts(nb); }
            vga_puts(" ");
            vga_puts(users[i].name);
            vga_puts("\n");
        }
    }

    vga_puts("  Groups: ");
    { char nb[2]; num_to_str(group_count, nb); vga_puts(nb); }
    vga_puts("\n");

    for (int i = 0; i < group_count; i++) {
        vga_puts("    GID=");
        { char nb[2]; num_to_str(groups[i].gid, nb); vga_puts(nb); }
        vga_puts(" ");
        vga_puts(groups[i].name);
        vga_puts(" (");
        { char nb[2]; num_to_str(groups[i].member_count, nb); vga_puts(nb); }
        vga_puts(" members)\n");
    }
    vga_puts("\n");
}
