#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

/* returns current time in microseconds */
static inline unsigned long long microseconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

#if defined(__i386__) || defined(__x86_64__)
#define HAVE_RDTSC 1
#endif

#ifdef HAVE_RDTSC
static inline unsigned long long rdtsc()
{
	unsigned int a, d;
	asm volatile("rdtsc" : "=a" (a), "=d" (d));
	return a + ((unsigned long long)d << 32);
}
#else
#define rdtsc() 0
#endif

/* performs read-after-write operations that the CPU is not supposed to be able
 * to parallelize. The "asm" statements are here to prevent the compiler from
 * reordering this code.
 */
#define dont_move(var) do { asm volatile("" : "=r"(var) : "0" (var)); } while (0)

#define run1cycle_ae()   do { a ^= e; dont_move(a); } while (0)
#define run1cycle_ba()   do { b ^= a; dont_move(b); } while (0)
#define run1cycle_cb()   do { c ^= b; dont_move(c); } while (0)
#define run1cycle_dc()   do { d ^= c; dont_move(d); } while (0)
#define run1cycle_ed()   do { e ^= d; dont_move(e); } while (0)
#define run1cycle_eb()   do { e ^= b; dont_move(e); } while (0)

#define run5cycles()                                    \
	do {                                            \
		run1cycle_ae();				\
		run1cycle_ba();				\
		run1cycle_cb();				\
		run1cycle_dc();				\
		run1cycle_ed();				\
	} while (0)

#define run10cycles()          \
	do {                   \
		run5cycles();  \
		run5cycles();  \
	} while (0)

#define run100cycles()          \
	do {                    \
		run10cycles();  \
		run10cycles();  \
		run10cycles();  \
		run10cycles();  \
		run10cycles();  \
		run10cycles();  \
		run10cycles();  \
		run10cycles();  \
		run10cycles();  \
		run10cycles();  \
	} while (0)


/* performs 1000 operations in a loop, all dependant on each other,
 * so that the CPU cannot parallelize them, hoping to take 1000 cycles.
 */
static void loop(long n)
{
	long a = 1, b = 2, c = 3, d = 3, e = n;
	volatile long sink __attribute__((unused));

	while (1) {
		run100cycles();
		run100cycles();
		run100cycles();
		run100cycles();
		run100cycles();
		run100cycles();
		run100cycles();
		run100cycles();
		run100cycles();        // 900 cycles

		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();         // 990 cycles

		run5cycles();          // 995 cycles

		run1cycle_ae();        // 996 cycles
		run1cycle_ba();        // 997 cycles
		run1cycle_eb();        // 998 cycles

		n--;                   // 999 cycles
		if (!n)                // 1000 cycles (conditional jump)
			break;
	}
	sink = e;
}

void run_once(long count)
{
	long long tsc_begin, tsc_end, tsc_cost;
	long long us_begin, us_end, us_cost;

	/* measure overhead */
	us_begin  = microseconds();
	tsc_begin = rdtsc();
	tsc_cost  = rdtsc() - tsc_begin;
	us_cost   = microseconds() - us_begin;

	/* now run the loop */
	us_begin  = microseconds();
	tsc_begin = rdtsc();
	loop(count);
	tsc_end   = rdtsc() - tsc_begin;
	us_end    = microseconds() - us_begin;

	tsc_end  -= tsc_cost;
	us_end   -= us_cost;

	printf("us=%lld cpu_MHz=%lld", us_end, count * 1000LL / us_end);
#ifdef HAVE_RDTSC
	printf(" TSC_cycles=%lld (%Ld/loop) rdtsc_MHz=%lld",
	       tsc_end, tsc_end / count, tsc_end / us_end);
#endif
	putchar('\n');
}

int main(int argc, char **argv)
{
	long count = 100000;
	long runs  = 1;

	if (argc > 1)
		runs = atol(argv[1]);

	if (argc > 2)
		count = atol(argv[2]);

	while (runs--)
		run_once(count);

	return 0;
}
