/* Compatibility boundaries for the pinned Energia and compiler runtimes. */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

extern char _heap;
extern char _eheap;
extern void *__real__sbrk(unsigned int increment);

/* Newlib's _sbrk_r captures the syscall errno object and copies it to the
 * supplied reentrancy object. The public errno macro names a different object.
 */
#undef errno
extern int errno;

static void *heap_failure(void)
{
    errno = ENOMEM;
    return (void *)(intptr_t)-1;
}

/* Retain Energia's heap ownership while enforcing the modern newlib contract.
 * GNU ld --wrap=_sbrk routes allocator calls here. __real__sbrk remains the
 * actual unmodified SDK routine and owns its heap_end variable.
 */
void *__wrap__sbrk(ptrdiff_t increment)
{
    uintptr_t lower = (uintptr_t)&_heap;
    uintptr_t upper = (uintptr_t)&_eheap;
    uintptr_t current = (uintptr_t)__real__sbrk(0);
    if (current < lower || current > upper)
        return heap_failure();

    if (increment >= 0) {
        if ((uintptr_t)increment > upper - current)
            return heap_failure();
    } else {
        /* Unsigned subtraction also handles PTRDIFF_MIN without negating it
         * in the signed type or allowing the SDK pointer addition to wrap.
         */
        uintptr_t decrease = (uintptr_t)0 - (uintptr_t)increment;
        if (decrease > current - lower)
            return heap_failure();
    }

    void *previous = __real__sbrk((unsigned int)increment);
    if (!previous || (uintptr_t)previous != current)
        return heap_failure();
    return previous;
}

/* Newlib termination hook for the pinned Energia bare-metal startup.
 * Energia main() loops forever and does not implement a process exit service.
 * Modern newlib's destructor-registration path nevertheless references _fini.
 * __libc_fini_array runs fini-array finalizers before this final hook.
 * __aeabi_atexit registrations use __cxa_atexit/__call_exitprocs separately.
 * This supplies that empty platform hook; it does not replace an application,
 * hardware, or library operation with a stub.
 */
void _fini(void) {}
