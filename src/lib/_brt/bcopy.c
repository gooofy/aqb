// copied from libnix

#include <stdlib.h>

#define NNN 16

/* This is a _fast_ block move routine! */
void bcopy(const void *s1, void *s2, size_t n) {
	size_t m;
	register char const * from asm("a1") = (char const *)s1;
    register char * to asm("a0") = (char *) s2;
	if (!n)
		return;

	if (s2 < s1) {
		if (n > NNN) {
#if !defined(__mc68020) && !defined(__mc68030) && !defined(__mc68040) && !defined(__mc68060) && !defined(__mc68080)
			if (((((char)(long)from) ^ ((char)(long)to))) % 2 == 0) {
#endif
			if (((long) to) & 1) {
				asm volatile("move.b (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				n--;
			}
			if (((long) to) & 2) {
				asm volatile("move.w (%1)+,(%0)+" : "+rm" (to), "+rm" (from): "r" (to), "r" (from));
				n -= 2;
			}
			for (m = n / (8 * sizeof(long)); m; --m) {
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
			}
			n &= 8 * sizeof(long) - 1;
			for (m = n / sizeof(long); m; --m)
				asm volatile("move.l (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
			n &= sizeof(long) - 1;
			if (!n)
				return;
		}
#if !defined(__mc68020) && !defined(__mc68030) && !defined(__mc68040) && !defined(__mc68060) && !defined(__mc68080)
		}
#endif
		while (n--){
			asm volatile("move.b (%1)+,(%0)+" : "=r" (to), "=r" (from): "r" (to), "r" (from));
        }
	} else {
		from += n;
		to += n;
#if !defined(__mc68020) && !defined(__mc68030) && !defined(__mc68040) && !defined(__mc68060) && !defined(__mc68080)
		if (((((char)(long)from) ^ ((char)(long)to))) % 2 == 0) {
#endif
		if (n > NNN) {
			if (((long) to) & 1) {
				asm volatile("move.b -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				n--;
			}
			if (((long) to) & 2) {
				asm volatile("move.w -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				n -= 2;
			}
			for (m = n / (8 * sizeof(long)); m; --m) {
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
			}
			n &= 8 * sizeof(long) - 1;
			for (m = n / sizeof(long); m; --m)
				asm volatile("move.l -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
			n &= sizeof(long) - 1;
			if (!n)
				return;
		}
#if !defined(__mc68020) && !defined(__mc68030) && !defined(__mc68040) && !defined(__mc68060) && !defined(__mc68080)
		}
#endif
		while (n--){
            asm volatile("move.b -(%1),-(%0)" : "=r" (to), "=r" (from): "r" (to), "r" (from));
        }
	}
}
