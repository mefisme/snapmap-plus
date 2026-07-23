#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

#include "commands.h"
#include "config.h"
#include "user_overrides.h"

#define USER_OVERRIDES_CONFIG_KEY "overrides.user_enabled"

/* -1 = not captured, 0 = disabled, 1 = enabled. */
static volatile LONG g_launch_state = -1;

void sh_user_overrides_capture_launch_state(void)
{
    int enabled = 1;
    (void)sh_config_get_bool(USER_OVERRIDES_CONFIG_KEY, &enabled, NULL);
    InterlockedCompareExchange(&g_launch_state, enabled ? 1 : 0, -1);
}

int sh_user_overrides_enabled_for_launch(void)
{
    LONG state = InterlockedCompareExchange(&g_launch_state, -1, -1);
    return state < 0 ? 1 : state != 0;
}

void h_sh_user_overrides(struct idCmdArgs *args)
{
    int argc = cmd_argc(args);
    int launch_enabled = sh_user_overrides_enabled_for_launch();
    const char *launch_word = launch_enabled ? "enabled" : "disabled";

    if (argc == 1) {
        int configured_enabled = 1;
        unsigned int flags = 0;
        if (!sh_config_get_bool(USER_OVERRIDES_CONFIG_KEY,
                                &configured_enabled, &flags)) {
            sh_printf("sh_user_overrides: user files are %s this launch; "
                      "next-launch value is unavailable (default: enabled).\n",
                      launch_word);
        } else if (flags & SH_CONFIG_STATUS_VOLATILE) {
            sh_printf("sh_user_overrides: user files are %s this launch; "
                      "configured value is %s but is not confirmed saved.\n",
                      launch_word,
                      configured_enabled ? "enabled" : "disabled");
        } else {
            sh_printf("sh_user_overrides: user files are %s this launch; "
                      "next launch is %s.\n",
                      launch_word,
                      configured_enabled ? "enabled" : "disabled");
        }
        return;
    }

    if (argc == 2) {
        const char *value = cmd_argv(args, 1);
        const char *json;
        const char *next_word;
        int result;

        if (value && strcmp(value, "0") == 0) {
            json = "false";
            next_word = "disabled";
        } else if (value && strcmp(value, "1") == 0) {
            json = "true";
            next_word = "enabled";
        } else {
            sh_printf("usage: sh_user_overrides [0|1]\n");
            return;
        }

        result = sh_config_set_json(USER_OVERRIDES_CONFIG_KEY, json);
        if (result == SH_CONFIG_SET_PERSISTED) {
            sh_printf("sh_user_overrides: saved %s; user files will be %s "
                      "next launch. Restart DOOM to apply; this launch "
                      "remains %s.\n",
                      value, next_word, launch_word);
        } else if (result == SH_CONFIG_SET_VOLATILE) {
            sh_printf("sh_user_overrides: %s was not saved to config.json; "
                      "this launch remains %s. Fix the config error and retry "
                      "before restarting DOOM.\n",
                      value, launch_word);
        } else {
            sh_printf("sh_user_overrides: config rejected %s; "
                      "no setting was changed.\n",
                      value);
        }
        return;
    }

    sh_printf("usage: sh_user_overrides [0|1]\n");
}

#ifdef SH_USER_OVERRIDES_TESTING
void sh_user_overrides_test_reset(void)
{
    InterlockedExchange(&g_launch_state, -1);
}
#endif
