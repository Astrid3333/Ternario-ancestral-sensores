/**
 * filesystem.c — Quipu filesystem over real directory
 *
 * The Quipu FS is a directory (~/.tak/quipu/) where each "knot" is a file.
 * File metadata is stored in a manifest (JSON-like plain text).
 * This gives us real persistence with minimal overhead.
 */

#include "ternary.h"

// =============================================================================
// PATHS
// =============================================================================

static char tak_home[512];
static char quipu_root[512];

void fs_set_home(const char* home) {
    strncpy(tak_home, home, 511);
    snprintf(quipu_root, 512, "%s/quipu", tak_home);
}

const char* fs_get_root(void) {
    return quipu_root;
}

// =============================================================================
// INIT — create directories if missing
// =============================================================================

void fs_init(void) {
    mkdir(tak_home, 0755);
    mkdir(quipu_root, 0755);
}

// =============================================================================
// CREATE — write a file into Quipu
// =============================================================================

int fs_create(const char* name, const char* data, uint32_t size) {
    char path[768];
    snprintf(path, sizeof(path), "%s/%s", quipu_root, name);

    FILE* f = fopen(path, "w");
    if (!f) return -1;
    fwrite(data, 1, size, f);
    fclose(f);
    return 0;
}

// =============================================================================
// READ — read a file from Quipu
// =============================================================================

int fs_read(const char* name, char* buf, uint32_t bufsize) {
    char path[768];
    snprintf(path, sizeof(path), "%s/%s", quipu_root, name);

    FILE* f = fopen(path, "r");
    if (!f) return -1;

    size_t n = fread(buf, 1, bufsize - 1, f);
    buf[n] = 0;
    fclose(f);
    return (int)n;
}

// =============================================================================
// DELETE
// =============================================================================

int fs_delete(const char* name) {
    char path[768];
    snprintf(path, sizeof(path), "%s/%s", quipu_root, name);
    return unlink(path);
}

// =============================================================================
// EXISTS
// =============================================================================

int fs_exists(const char* name) {
    char path[768];
    snprintf(path, sizeof(path), "%s/%s", quipu_root, name);
    struct stat st;
    return stat(path, &st) == 0;
}

// =============================================================================
// LIST — return all files in Quipu
// =============================================================================

int fs_list(tak_file_t* files, int max_files) {
    DIR* d = opendir(quipu_root);
    if (!d) return 0;

    int count = 0;
    struct dirent* ent;
    while ((ent = readdir(d)) != NULL && count < max_files) {
        if (ent->d_name[0] == '.') continue;

        strncpy(files[count].name, ent->d_name, 31);
        files[count].name[31] = 0;

        char path[768];
        snprintf(path, sizeof(path), "%s/%s", quipu_root, ent->d_name);
        struct stat st;
        if (stat(path, &st) == 0) {
            files[count].size = (uint32_t)st.st_size;
            files[count].type = S_ISDIR(st.st_mode) ? 1 : 0;
        } else {
            files[count].size = 0;
            files[count].type = 0;
        }
        snprintf(files[count].path, 256, "%s/%s", quipu_root, ent->d_name);
        count++;
    }
    closedir(d);
    return count;
}

// =============================================================================
// SIZE — get file size
// =============================================================================

uint32_t fs_size(const char* name) {
    char path[768];
    snprintf(path, sizeof(path), "%s/%s", quipu_root, name);
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (uint32_t)st.st_size;
}
