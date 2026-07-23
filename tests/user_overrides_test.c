#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "commands.h"
#include "config.h"
#include "user_overrides.h"

static int g_failed;
static unsigned long g_root_serial;
static char g_console[2048];

#define CHECK(expr) do {                                                        \
    if (!(expr)) {                                                              \
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #expr); \
        g_failed++;                                                             \
    }                                                                           \
} while (0)

typedef struct test_cmd_args {
    int argc;
    char _pad[4];
    const char *argv[3];
} test_cmd_args;

void backend_log(const char *line)
{
    (void)line;
}

int cmd_argc(idCmdArgs *args)
{
    return args ? args->argc : 0;
}

const char *cmd_argv(idCmdArgs *args, int index)
{
    return args && index >= 0 && index < args->argc
        ? args->argv[index] : NULL;
}

void sh_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _vsnprintf_s(g_console, sizeof(g_console), _TRUNCATE, format, args);
    va_end(args);
}

static int make_temp_root(wchar_t *out, size_t capacity)
{
    wchar_t base[MAX_PATH];
    DWORD n = GetTempPathW((DWORD)(sizeof(base) / sizeof(base[0])), base);
    unsigned long pid = GetCurrentProcessId();
    unsigned long tick = GetTickCount() + ++g_root_serial;
    if (!n || n >= sizeof(base) / sizeof(base[0])) return 0;
    if (_snwprintf_s(out, capacity, _TRUNCATE,
                     L"%sSnapmapPlusUserOverrides-%lu-%lu",
                     base, pid, tick) < 0)
        return 0;
    return CreateDirectoryW(out, NULL) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
}

static int write_file(const wchar_t *path, const char *bytes, size_t length)
{
    FILE *file = NULL;
    if (_wfopen_s(&file, path, L"wb") != 0 || !file) return 0;
    if (length && fwrite(bytes, 1, length, file) != length) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static void cleanup_temp_root(const wchar_t *root)
{
    WIN32_FIND_DATAW found;
    HANDLE search;
    wchar_t path[MAX_PATH], dir[MAX_PATH], pattern[MAX_PATH];
    _snwprintf_s(dir, MAX_PATH, _TRUNCATE, L"%s\\snapmap-plus", root);
    _snwprintf_s(pattern, MAX_PATH, _TRUNCATE, L"%s\\*", dir);
    search = FindFirstFileW(pattern, &found);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(found.cFileName, L".") != 0 &&
                wcscmp(found.cFileName, L"..") != 0 &&
                !(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                _snwprintf_s(path, MAX_PATH, _TRUNCATE,
                             L"%s\\%s", dir, found.cFileName);
                DeleteFileW(path);
            }
        } while (FindNextFileW(search, &found));
        FindClose(search);
    }
    RemoveDirectoryW(dir);
    RemoveDirectoryW(root);
}

static void run_command(int argc, const char *value, const char *extra)
{
    test_cmd_args args;
    memset(&args, 0, sizeof(args));
    args.argc = argc;
    args.argv[0] = "sh_user_overrides";
    args.argv[1] = value;
    args.argv[2] = extra;
    g_console[0] = '\0';
    h_sh_user_overrides((idCmdArgs *)&args);
}

static void test_launch_snapshot_and_command_contract(void)
{
    wchar_t root[MAX_PATH], marker_path[MAX_PATH];
    int enabled = -1;
    int configured_enabled;
    int launch_enabled;

    CHECK(make_temp_root(root, MAX_PATH));
    sh_config_test_reset();
    sh_config_test_set_local_appdata(root);

    /* Launch captures true; a persisted command 0 does not mutate the launch. */
    CHECK(sh_config_init() == 1);
    sh_user_overrides_test_reset();
    sh_user_overrides_capture_launch_state();
    CHECK(sh_user_overrides_enabled_for_launch() == 1);
    run_command(2, "0", NULL);
    CHECK(strstr(g_console, "saved 0") != NULL);
    CHECK(strstr(g_console, "Restart DOOM") != NULL);
    CHECK(sh_user_overrides_enabled_for_launch() == 1);
    CHECK(sh_config_get_bool("overrides.user_enabled", &enabled, NULL) == 1);
    CHECK(enabled == 0);

    /* Query distinguishes this launch from the next launch. */
    run_command(1, NULL, NULL);
    CHECK(strstr(g_console, "enabled this launch") != NULL);
    CHECK(strstr(g_console, "next launch is disabled") != NULL);

    /* A simulated new process captures the durable false. */
    sh_config_test_reset();
    sh_config_test_set_local_appdata(root);
    CHECK(sh_config_init() == 1);
    sh_user_overrides_test_reset();
    sh_user_overrides_capture_launch_state();
    CHECK(sh_user_overrides_enabled_for_launch() == 0);

    /* Exact 1 works; invalid and extra arguments do not mutate. */
    run_command(2, "1", NULL);
    CHECK(strstr(g_console, "saved 1") != NULL);
    CHECK(sh_config_get_bool("overrides.user_enabled", &enabled, NULL) == 1);
    CHECK(enabled == 1);
    run_command(2, "true", NULL);
    CHECK(strcmp(g_console, "usage: sh_user_overrides [0|1]\n") == 0);
    CHECK(sh_config_get_bool("overrides.user_enabled", &enabled, NULL) == 1);
    CHECK(enabled == 1);
    run_command(3, "0", "extra");
    CHECK(strcmp(g_console, "usage: sh_user_overrides [0|1]\n") == 0);
    CHECK(sh_config_get_bool("overrides.user_enabled", &enabled, NULL) == 1);
    CHECK(enabled == 1);

    /* A marker file has no influence. */
    _snwprintf_s(marker_path, MAX_PATH, _TRUNCATE,
                 L"%s\\snapmap-plus\\overrides.disabled", root);
    CHECK(write_file(marker_path, "", 0));
    CHECK(sh_config_get_bool("overrides.user_enabled",
                             &configured_enabled, NULL) == 1);
    sh_user_overrides_test_reset();
    sh_user_overrides_capture_launch_state();
    CHECK(sh_user_overrides_enabled_for_launch() == configured_enabled);

    /* A failed write is reported and cannot alter this launch. */
    launch_enabled = sh_user_overrides_enabled_for_launch();
    sh_config_test_fail_next(SH_CONFIG_TEST_FAIL_WRITE);
    run_command(2, "0", NULL);
    CHECK(strstr(g_console, "not saved") != NULL);
    CHECK(sh_user_overrides_enabled_for_launch() == launch_enabled);

    /* The failed value is session-only until the same command succeeds. */
    run_command(1, NULL, NULL);
    CHECK(strstr(g_console, "not confirmed saved") != NULL);
    run_command(2, "0", NULL);
    CHECK(strstr(g_console, "saved 0") != NULL);
    CHECK(sh_user_overrides_enabled_for_launch() == launch_enabled);
    run_command(1, NULL, NULL);
    CHECK(strstr(g_console, "next launch is disabled") != NULL);
    CHECK(strstr(g_console, "not confirmed saved") == NULL);

    /* A fresh config initialization reads the successfully retried value. */
    sh_config_test_reset();
    sh_config_test_set_local_appdata(root);
    CHECK(sh_config_init() == 1);
    CHECK(sh_config_get_bool("overrides.user_enabled", &enabled, NULL) == 1);
    CHECK(enabled == 0);

    sh_config_test_reset();
    cleanup_temp_root(root);
}

int main(void)
{
    test_launch_snapshot_and_command_contract();
    if (g_failed) {
        fprintf(stderr, "%d user-overrides test(s) failed\n", g_failed);
        return 1;
    }
    puts("user overrides tests passed");
    return 0;
}
