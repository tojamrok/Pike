/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Generic strap for the code-generator.
 *
 * Henrik Grubbstr�m 20010720
 */

#include "global.h"
#include "program.h"
#include "opcodes.h"
#include "docode.h"
#include "interpret.h"
#include "lex.h"
#include "pike_embed.h"

#include "pikecode.h"

#if PIKE_BYTECODE_METHOD == PIKE_BYTECODE_IA32
#include "code/ia32.c"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_AMD64
#include "code/amd64.c"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_SPARC
#include "code/sparc.c"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_PPC32
#include "code/ppc32.c"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_PPC64
#include "code/ppc64.c"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_ARM32
#include "code/arm32.c"
#elif PIKE_BYTECODE_METHOD == PIKE_BYTECODE_ARM64
#include "code/arm64.c"
#else
#include "code/bytecode.c"
#endif
