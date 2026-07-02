/* wiring_cleandirect.h -- sh_target_any "wire-any": bind a wire to any target ENTITY directly, no radial.
 *
 * While sh_target_any is in its reveal state, the editor wire tool's connect creators (cdbb40 / cdb990) are
 * detoured so a target that would otherwise node-mediate (raise the "which input?" radial picker) instead
 * takes the tool's own clean-direct branch -- binding the wire straight to the target entity, no picker, no
 * node mediation. Implemented by transiently flagging the target an input-node for the stock creator's call
 * (see wiring_cleandirect.c); forces no slots, creates/frees no node. A clone improvement over the original,
 * which cannot suppress the override-driven picker. Off unless sh_target_any is revealed.
 */
#ifndef BACKEND_WIRING_CLEANDIRECT_H
#define BACKEND_WIRING_CLEANDIRECT_H

#include <stdint.h>

/* Resolve the two connect creators by signature and install the (flag-gated, off-until-reveal) detours ONCE.
 * If either creator can't be resolved, nothing is installed and the tool stays stock. */
void sh_wiring_cleandirect_install(const uint8_t *module_base);

/* A monotonic counter bumped each time the wire-any hook processes a target pick (a connect edit). The UI
 * polls it (via the iface +0x288 slot) alongside entity_count and rebuilds the entity list when it changes,
 * so the chain's module-name labels re-read + settle after a wire (which nets no entity-count change).
 * Read-only for callers; increments only. */
int sh_wiring_cleandirect_generation(void);

#endif /* BACKEND_WIRING_CLEANDIRECT_H */
