#pragma once

/* trace options         */
#define TRACE_RECORD            (0)  /* Execute syscall, write out entry and
                                        exit records. */
#define TRACE_RECORD_ENTRY      (1)  /* When this option is chosen, the syscall
                                        is not actually executed. The syscall
                                        entry record is written out, however. */
#define TRACE_RECORD_EXIT       (2)  /* Execute syscall, but write out only the
                                        exit record. */
