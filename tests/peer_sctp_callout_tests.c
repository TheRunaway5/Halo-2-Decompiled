/* Exercise the dependency's real callout/observer lock boundary, without ICE,
 * certificates or match simulation. Timer callbacks own only an atomic count. */
#include <sys/types.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <user_atomic.h>
#include <netinet/sctp_sysctl.h>
#include <netinet/sctp_pcb.h>
#include <stdatomic.h>

/* Public usrsctp.h and internal SCTP headers define overlapping protocol
 * types. These two exact pinned public ABI declarations avoid combining them. */
extern void usrsctp_init_nothreads(uint16_t,
    int (*)(void *, void *, size_t, uint8_t, uint8_t), void (*)(const char *, ...));
extern int usrsctp_finish(void);

static sctp_os_timer_t timer;
static _Atomic unsigned callbacks, observations;
static void expired(void *argument)
{
    (void)argument;
    atomic_fetch_add_explicit(&callbacks, 1, memory_order_relaxed);
}
static void *ticks(void *argument)
{
    (void)argument;
    for (unsigned i = 0; i < 200000; ++i) {
        sctp_handle_tick(1);
        if (i % 64 == 0) sched_yield();
    }
    return NULL;
}
static void *observe(void *argument)
{
    (void)argument;
    for (unsigned i = 0; i < 200000; ++i) {
        if (!SCTP_OS_TIMER_PENDING(&timer)) {
            if (SCTP_OS_TIMER_ACTIVE(&timer)) SCTP_OS_TIMER_DEACTIVATE(&timer);
            sctp_os_timer_start(&timer, 1, expired, NULL);
        }
        atomic_fetch_add_explicit(&observations, 1, memory_order_relaxed);
        if (i % 64 == 0) sched_yield();
    }
    return NULL;
}
int main(void)
{
    usrsctp_init_nothreads(0, NULL, NULL);
    sctp_os_timer_init(&timer);
    sctp_os_timer_start(&timer, 1, expired, NULL);
    pthread_t ticker, observer;
    if (pthread_create(&ticker, NULL, ticks, NULL)) return 1;
    if (pthread_create(&observer, NULL, observe, NULL)) { pthread_join(ticker, NULL); return 1; }
    pthread_join(ticker, NULL);
    pthread_join(observer, NULL);
    sctp_os_timer_stop(&timer);
    SCTP_OS_TIMER_DEACTIVATE(&timer);
    if (SCTP_OS_TIMER_ACTIVE(&timer) || SCTP_OS_TIMER_PENDING(&timer) || !callbacks || observations != 200000) return 1;
    if (usrsctp_finish()) return 1;
    printf("PASS actual SCTP callout: concurrent tick/callback and pending/active/deactivate/start, %u callbacks; stopped timer quiescent\n", callbacks);
    return 0;
}
