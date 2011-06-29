#pragma once

/* print debug message   */
#define RETRO_DEBUG

/* hooking options       */
#define HOOK_CONT (0)  /* Execute syscall fully. */
#define HOOK_SKIP (1)  /* Drop syscall. Do not execute. */

/* deterministic process */
#define RETRO_OPT_DET
