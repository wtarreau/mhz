#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

/* returns current time in microseconds */
static inline unsigned long long microseconds()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000ULL + tv.tv_usec;
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


/* performs 50 operations in a loop, all dependant on each other, so that the
 * CPU cannot parallelize them, hoping to take 50 cycles per loop, plus the
 * loop counter overhead.
 */
static __attribute__((noinline)) void loop50(unsigned int n)
{
	unsigned int a = 0, b = 0, c = 0, d = 0, e = 0;

	do {
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
	} while (__builtin_expect(--n, 1));
}

/* performs 250 operations in a loop, all dependant on each other, so that the
 * CPU cannot parallelize them, hoping to take 250 cycles per loop, plus the
 * loop counter overhead. Do not increase this loop so that it fits in a small
 * 1 kB L1 cache on 32-bit instruction sets.
 */
static __attribute__((noinline)) void loop250(unsigned int n)
{
	unsigned int a = 0, b = 0, c = 0, d = 0, e = 0;

	do {
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
		run10cycles();
		run100cycles();
		run100cycles();
	} while (__builtin_expect(--n, 1));
}

void run_once(long count)
{
	long long tsc_begin, tsc_end50, tsc_end250;
	long long us_begin, us_end50, us_end250;

	/* now run the loop */
	us_begin   = microseconds();
	tsc_begin  = rdtsc();
	loop50(count);
	tsc_end50 = rdtsc() - tsc_begin;
	us_end50  = microseconds() - us_begin;

	/* now run the loop */
	us_begin   = microseconds();
	tsc_begin  = rdtsc();
	loop250(count);
	tsc_end250 = rdtsc() - tsc_begin;
	us_end250  = microseconds() - us_begin;

	printf("count=%ld us50=%lld us250=%lld diff=%lld cpu_MHz=%.3f",
	       count, us_end50, us_end250, us_end250 - us_end50,
	       count * 200.0 / (us_end250 - us_end50));
#ifdef HAVE_RDTSC
	printf(" tsc50=%lld tsc250=%lld diff=%lld rdtsc_MHz=%.3f",
	       tsc_end50, tsc_end250, (tsc_end250 - tsc_end50) / count,
	       (tsc_end250 - tsc_end50) / (float)(us_end250 - us_end50));
#endif
	putchar('\n');
}

/* spend <delay> ms waiting for the CPU's frequency to raise. Will also stop
 * on backwards time jumps if any.
 */
void pre_heat(long delay)
{
	unsigned long long start = microseconds();

	while (microseconds() - start < (unsigned long long)delay)
		;
}

/* determines how long loop50() must be run to reach more than 20 milliseconds.
 * This will ensure that an integral number of clock ticks will have happened
 * on 100, 250, 1000 Hz systems.
 */
unsigned int calibrate()
{
	unsigned long long duration = 0;
	unsigned long long start;
	unsigned int count = 1000;

	while (duration < 20000) {
		count = count * 5 / 4;
		start = microseconds();
		loop50(count);
		duration = microseconds() - start;
	}
	return count;
}

int main(int argc, char **argv)
{
	unsigned int count;
	long runs = 1;

	if (argc > 1)
		runs = atol(argv[1]);

	if (argc > 2)
		pre_heat(atol(argv[2]));

	count = calibrate();

	if (argc > 3)
		count = atol(argv[3]);

	while (runs--)
		run_once(count);

	return 0;
}
