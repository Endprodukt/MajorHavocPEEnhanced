/* pokeyio.h - what the program reads from a POKEY: the pots and the
 * random number.
 *
 * (From Tempest Enhanced.) Major Havoc wires its 13/14S switches to POKEY
 * 0's pot inputs as Tempest did its own: a closed switch is a pot
 * that reads "done" at once, an open one a pot that would count to 228.
 * The interrupt starts a scan (POTGO) and reads ALLPOT a few cycles later,
 * so ALLPOT shows the switches as they are. The random number is the 17
 * bit polynomial counter, stepping once per clock while SKCTL is out of
 * reset.
 *
 * Behaviour after MAME's pokey.cpp (BSD-3-Clause, Brad Oliver, Eric
 * Smith, Juergen Buchmueller, Couriersud). The sound side is
 * sound/pokey.h; both see the same register writes.
 */
#ifndef MH_POKEYIO_H
#define MH_POKEYIO_H

#include <stdint.h>

typedef struct mh_pokeyio {
    uint8_t  skctl;
    uint8_t  audctl;
    uint8_t  allpot;        /* a bit per pot: its scan is done */
    uint8_t  potx[8];       /* the value each pot settles at (0..228) */
    uint64_t potgo_at;      /* CPU cycle of the last POTGO */
    uint64_t poly_base;     /* CPU cycle the polynomial counters left reset */
    uint8_t  inputs;        /* the switches on the pot inputs, a bit per pot: 1 open (counts to 228) */
} mh_pokeyio;

void    mh_pokeyio_init(mh_pokeyio *p);
void    mh_pokeyio_write(mh_pokeyio *p, unsigned reg, uint8_t v, uint64_t cycle);
uint8_t mh_pokeyio_read(mh_pokeyio *p, unsigned reg, uint64_t cycle);

#endif
