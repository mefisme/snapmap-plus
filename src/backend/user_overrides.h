#ifndef BACKEND_USER_OVERRIDES_H
#define BACKEND_USER_OVERRIDES_H

struct idCmdArgs;

void sh_user_overrides_capture_launch_state(void);
int sh_user_overrides_enabled_for_launch(void);
void h_sh_user_overrides(struct idCmdArgs *args);

#ifdef SH_USER_OVERRIDES_TESTING
void sh_user_overrides_test_reset(void);
#endif

#endif
