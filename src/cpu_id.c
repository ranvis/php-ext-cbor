/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cpu_id.h"

#if defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#define GET_CPU_ID(fn, out) __cpuid(fn, out[0], out[1], out[2], out[3]);
#else
#include <intrin.h>
#define GET_CPU_ID(fn, out) __cpuid(out, fn);
#endif

bool get_cpu_id(int fn_id, int cpu_id[4])
{
	if (fn_id > 1) {
		static int max_fn_id;
		if (!max_fn_id) {
			GET_CPU_ID(0, cpu_id);
			max_fn_id = cpu_id[0];
		}
		if (fn_id > max_fn_id) {
			return false;
		}
	}
	GET_CPU_ID(fn_id, cpu_id);
	return true;
}
