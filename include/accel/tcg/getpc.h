/*
 * Get host pc for helper unwinding.
 *
 * Copyright (c) 2003 Fabrice Bellard
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef ACCEL_TCG_GETPC_H
#define ACCEL_TCG_GETPC_H

#ifndef CONFIG_TCG
#error Can only include this header with TCG
#endif

/* GETPC is the true target of the return instruction that we'll execute.  */
#if defined(CONFIG_TCG_INTERPRETER) && defined(CONFIG_TCG_NATIVE)
/*
 * Dual mode: the same helper function is called either by the native
 * backend (from generated code, where the return address is a valid
 * GETPC) or by the TCI interpreter (via ffi_call, where the return
 * address points into the interpreter loop and is not a valid GETPC).
 * tcg_use_interp is chosen at startup; GETPC is only used on cold
 * (exception/fault) paths, so the runtime branch is negligible.
 */
extern __thread uintptr_t tci_tb_ptr;
extern bool tcg_use_interp;
# define GETPC() \
    (tcg_use_interp ? tci_tb_ptr \
                    : ((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0))))
#elif defined(CONFIG_TCG_INTERPRETER)
extern __thread uintptr_t tci_tb_ptr;
# define GETPC() tci_tb_ptr
#else
# define GETPC() \
    ((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)))
#endif

#endif /* ACCEL_TCG_GETPC_H */
