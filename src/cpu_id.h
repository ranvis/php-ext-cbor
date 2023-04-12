/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include <stdbool.h>

#if defined(_MSC_VER) && !defined(__F16C__) && defined(__AVX2__) && !defined(_M_ARM)
#define __F16C__
#endif

enum {
	// cf. Intel Architecture Instruction Set Extensions Programming Reference ver.48 1-31
	CPU_ID_1_2_SSE3 = 1 << 0,
	CPU_ID_1_2_F16C = 1 << 29,
};

bool get_cpu_id(int fn_id, int cpu_id[4]);
