/* mh_map.h - the Major Havoc board's addresses, as Major Havoc - The
 * Promised End (Jess M. Askey) uses it.
 *
 * The hardware is from HBMAME's driver (src/hbmame/drivers/mhavoc_hb.cpp,
 * mhavocpe_adpcm, BSD-3-Clause) and Atari's/the PE source's equates
 * (upstream/Source/a_def.ah, mh_gamma.asm). Two 6502s:
 *
 *   Alpha  2.5 MHz   the game: picture, logic
 *   Gamma  1.25 MHz  sound (four POKEYs, the OKI 6295 speech), the
 *                    switches, the roller, the EEPROM
 *
 * They talk through one latch each way (a write to the other side's port
 * gives the gamma an NMI).
 */
#ifndef MH_MAP_H
#define MH_MAP_H

/* --- Alpha --- */
#define MH_A_RAM0       0x0000  /* 0000-01FF RAM */
#define MH_A_ZRAM_LO    0x0200  /* 0200-07FF the paged RAM (bank 0 or 1) */
#define MH_A_RAM1       0x0800  /* 0800-09FF RAM */
#define MH_A_ZRAM_HI    0x0a00  /* 0A00-0FFF the paged RAM again (the same bank) */
#define MH_A_PORTRD     0x1000  /* read: the gamma's latch */
#define MH_A_INPUTS     0x1200  /* read: IN0 */
#define MH_A_COLRAM     0x1400  /* 1400-141F colour RAM, 32 nibbles */
#define MH_A_OUTPUTS    0x1600  /* D7 invert Y, D6 invert X, D5 player select, D3 gamma run, D2 beta run, D0 roller light */
#define MH_A_VGGO       0x1640
#define MH_A_WATCHDOG   0x1680
#define MH_A_VGRST      0x16c0
#define MH_A_INTACK     0x1700
#define MH_A_ROMPG      0x1740  /* the 8K page at 2000 (three bits) */
#define MH_A_RAMPG      0x1780  /* the paged RAM's bank (bit 0) */
#define MH_A_PORTWR     0x17c0  /* write: to the gamma's latch */
#define MH_A_COMRAM     0x1800  /* 1800-1FFF RAM (the beta's, never fitted: plain RAM here) */
#define MH_A_PAGED      0x2000  /* 2000-3FFF the paged program ROM */
#define MH_A_VGRAM      0x4000  /* 4000-4FFF vector RAM */
#define MH_A_VGROM      0x5000  /* 5000-5FFF vector ROM (6KL, first 4K) */
#define MH_A_VGROM2     0x6000  /* 6000-7FFF the vector ROM page the generator selected (6H, 6JK) */
#define MH_A_PROGRAM    0x8000  /* 8000-FFFF program ROM (1MN, 1L) */

/* IN0 (1200) */
#define MH_IN0_VGHALT   0x01    /* the vector generator is done (high) */
#define MH_IN0_2K4      0x02    /* the 2.4 kHz signal */
#define MH_IN0_GXMTD    0x04    /* the gamma has sent (its latch is full) */
#define MH_IN0_GRCVD    0x08    /* the gamma has read the alpha's byte */
#define MH_IN0_UNUSED   0x10    /* active low */
#define MH_IN0_DIAG     0x20    /* diagnostic step, active low */
#define MH_IN0_BIT6     0x40    /* player select 0: right coin; 1: the cabinet switch (active low) */
#define MH_IN0_BIT7     0x80    /* player select 0: left coin; 1: self test (active low) */

/* --- Gamma --- */
#define MH_G_RAM        0x0000  /* 0000-07FF, mirrored to 1FFF */
#define MH_G_POKEY      0x2000  /* 2000-203F the four POKEYs, mirrored to 27FF */
#define MH_G_INPUT      0x2800  /* read: IN1 */
#define MH_G_INDATA     0x3000  /* read: the alpha's latch */
#define MH_G_LETA       0x3800  /* read: 3800 the roller, 3801 the cocktail's */
#define MH_G_INTACK     0x4000  /* write: the interrupt's acknowledge; read: the switches at 8S */
#define MH_G_COUNTER    0x4800  /* write: coin counters */
#define MH_G_OUTDATA    0x5000  /* write: to the alpha's latch */
#define MH_G_OKI        0x5800  /* write (5800 and 5900): the OKI 6295 */
#define MH_G_OKI2       0x5900
#define MH_G_EEROM      0x6000  /* 6000-61FF the EEPROM (2804, 512 bytes), mirrored to 7FFF */
#define MH_G_ROM        0x8000  /* 8000-BFFF 9S, again at C000 */

/* IN1 (2800) */
#define MH_IN1_AXMTD    0x01    /* the alpha has sent */
#define MH_IN1_ARCVD    0x02    /* the alpha has read the gamma's byte */
#define MH_IN1_TIRDY    0x04    /* the TMS5220 is ready (high: not ready; no TMS here) */
#define MH_IN1_UNUSED   0x08
#define MH_IN1_P2_B2    0x10    /* active low */
#define MH_IN1_P2_B1    0x20
#define MH_IN1_B2       0x40    /* button 2: the shield */
#define MH_IN1_B1       0x80    /* button 1: jump (and start) */

/* POKEY register offsets used here */
#define MH_PK_ALLPOT    0x08    /* read */
#define MH_PK_RANDOM    0x0a    /* read */
#define MH_PK_POTGO     0x0b    /* write */
#define MH_PK_SKCTL     0x0f    /* write */

/* --- clocks (MAME: MHAVOC_CLOCK 10 MHz) --- */
#define MH_MASTER_HZ        10000000.0
#define MH_ALPHA_HZ         2500000.0       /* the time base of the emulation: alpha cycles */
#define MH_GAMMA_DIV        2               /* alpha cycles per gamma cycle (1.25 MHz; the POKEYs too) */
#define MH_TICK_CYCLES      512             /* alpha cycles per 5 kHz tick (625 kHz / 128 = 4882.8 Hz) */
#define MH_VG_HZ            12096000.0      /* MAME runs every AVG on this clock */

#endif
