#include "global.h"
#include "littlefs.h"

#define MOUNT_PATH "/home"

void _LittleFSInit(void)
{   
    esp_vfs_littlefs_conf_t conf = {
        .base_path = MOUNT_PATH,             // mount point
        .partition_label = "storage",        // nama partisi sesuai tabel partisi
        .format_if_mount_failed = true       // format jika gagal mount
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        LOG_E("Failed to mount or format LittleFS (%s)", esp_err_to_name(ret));
        return;
    }
}


// ===================== WRITE FILE =====================
int L_FS_WRITE(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    const char *content  = luaL_optstring(L, 2, "");

    FILE *f = fopen(filename, "a");  // append, auto create
    if (!f) {
        lua_pushnil(L);
        lua_pushfstring(L, "failed to open file: %s", filename);
        return 2;
    }

    size_t written = fwrite(content, 1, strlen(content), f);
    fclose(f);

    lua_pushboolean(L, written > 0);
    return 1;
}

// ===================== READ FILE =====================
int L_FS_READ(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    FILE *f = fopen(filename, "r");
    if (!f) {
        lua_pushnil(L);
        lua_pushstring(L, "file not found");
        return 2;
    }

    lua_newtable(L); // result table

    char buf[CHAR_BUFFER_SIZE];
    uint8_t index = 1;
    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        if (len && buf[len-1] == '\n') buf[len-1] = '\0'; // trim newline

        lua_pushinteger(L, index);
        lua_pushstring(L, buf);
        lua_settable(L, -3);

        index++;
    }
    fclose(f);

    return 1; // return table
}

// ===================== EDIT FILE =====================
int L_FS_EDIT(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    uint8_t index        = luaL_checkinteger(L, 2);   // 0-based index
    const char *content  = luaL_checkstring(L, 3);    // "" = delete

    if (index == 0 || index > CHAR_BUFFER_SIZE) {
        lua_pushnil(L);
        lua_pushstring(L, "index out of range");
        return 2;
    }

    FILE *f = fopen(filename, "r");
    if (!f) {
        lua_pushnil(L);
        lua_pushstring(L, "failed to open file for read");
        return 2;
    }

    char lines[CHAR_BUFFER_SIZE][CHAR_BUFFER_SIZE];
    uint8_t count = 0;
    while (fgets(lines[count], sizeof(lines[count]), f) && count < CHAR_BUFFER_SIZE) {
        size_t len = strlen(lines[count]);
        if (len && lines[count][len-1] == '\n') lines[count][len-1] = '\0';
        count++;
    }
    fclose(f);

    if (strlen(content) == 0) {
        // delete line
        if (index >= count) {
            lua_pushnil(L);
            lua_pushstring(L, "index out of range for delete");
            return 2;
        }
        for (uint8_t i = index; i < count-1; i++) {
            strcpy(lines[i], lines[i+1]);
        }
        count--;
    } else {
        // replace / add
        if (index >= count) {
            for (uint8_t i = count; i < index; i++) {
                lines[i][0] = '\0';
            }
            count = index+1;
        }
        strncpy(lines[index], content, sizeof(lines[index])-1);
        lines[index][sizeof(lines[index])-1] = '\0';
    }

    f = fopen(filename, "w");
    if (!f) {
        lua_pushnil(L);
        lua_pushstring(L, "failed to open file for write");
        return 2;
    }

    for (uint8_t i = 0; i < count; i++) {
        fprintf(f, "%s\n", lines[i]);
    }
    fclose(f);

    lua_pushboolean(L, 1);
    return 1;
}

// ===================== REMOVE FILE =====================
int L_FS_REMOVE(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);

    if (remove(filename) == 0) {
        lua_pushboolean(L, 1);
        return 1;
    }
    lua_pushnil(L);
    lua_pushstring(L, "failed to remove file");
    return 2;
}

// ===================== RENAME FILE =====================
int L_FS_RENAME(lua_State *L) {
    const char *oldname = luaL_checkstring(L, 1);
    const char *newname = luaL_checkstring(L, 2);

    if (rename(oldname, newname) == 0) {
        lua_pushboolean(L, 1);
        return 1;
    }
    lua_pushnil(L);
    lua_pushstring(L, "failed to rename file");
    return 2;
}

// ===================== SIZE FILE =====================
int L_FS_SIZE(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);

    struct stat st;
    if (stat(filename, &st) != 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "failed to stat file: %s", filename);
        return 2;
    }

    lua_pushinteger(L, st.st_size);
    return 1;
}

// ===================== LIST FILE =====================
int L_FS_LS(lua_State *L) {
    const char* path = "/home"; // default path
    if (lua_gettop(L) >= 1) {
        path = luaL_checkstring(L, 1);
    }

    DIR* dir = opendir(path);
    if (!dir) {
        lua_pushnil(L);
        lua_pushfstring(L, "Directory not found: %s", path);
        return 2; // return nil + error message
    }

    struct dirent* entry;
    lua_newtable(L); // table untuk hasil
    uint8_t i = 1;

    while ((entry = readdir(dir)) != NULL) {
        lua_pushstring(L, entry->d_name);
        lua_rawseti(L, -2, i++);
    }

    closedir(dir);
    return 1; // return table
}

const luaL_Reg FS_LIB[] = {
  {"write", L_FS_WRITE},
  {"read", L_FS_READ},
  {"edit", L_FS_EDIT},
  {"remove", L_FS_REMOVE},
  {"rename", L_FS_RENAME},
  {"size", L_FS_SIZE},
  {"ls", L_FS_LS},
  {NULL, NULL}
};

int LUA_OPEN_FS ( lua_State *L ) { _LittleFSInit(); luaL_newlib(L, FS_LIB); return 1; }