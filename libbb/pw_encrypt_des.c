/*
 * FreeSec: libcrypt for NetBSD
 *
 * Copyright (c) 1994 David Burren
 * All rights reserved.
 *
 * Adapted for FreeBSD-2.0 by Geoffrey M. Rehmet
 *	this file should now *only* export crypt(), in order to make
 *	binaries of libcrypt exportable from the USA
 *
 * Adapted for FreeBSD-4.0 by Mark R V Murray
 *	this file should now *only* export crypt_des(), in order to make
 *	a module that can be optionally included in libcrypt.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This is an original implementation of the DES and the crypt(3) interfaces
 * by David Burren <davidb@werj.com.au>.
 *
 * An excellent reference on the underlying algorithm (and related
 * algorithms) is:
 *
 *	B. Schneier, Applied Cryptography: protocols, algorithms,
 *	and source code in C, John Wiley & Sons, 1994.
 *
 * Note that in that book's description of DES the lookups for the initial,
 * pbox, and final permutations are inverted (this has been brought to the
 * attention of the author).  A list of errata for this book has been
 * posted to the sci.crypt newsgroup by the author and is available for FTP.
 *
 * ARCHITECTURE ASSUMPTIONS:
 *	It is assumed that the 8-byte arrays passed by reference can be
 *	addressed as arrays of uint32_t's (ie. the CPU is not picky about
 *	alignment).
 */

/* A pile of data */
static const uint8_t IP[64] = {
	58, 50, 42, 34, 26, 18, 10,  2, 60, 52, 44, 36, 28, 20, 12,  4,
	62, 54, 46, 38, 30, 22, 14,  6, 64, 56, 48, 40, 32, 24, 16,  8,
	57, 49, 41, 33, 25, 17,  9,  1, 59, 51, 43, 35, 27, 19, 11,  3,
	61, 53, 45, 37, 29, 21, 13,  5, 63, 55, 47, 39, 31, 23, 15,  7
};

static const uint8_t key_perm[56] = {
	57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
	10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
	63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
	14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4
};

static const uint8_t key_shifts[16] = {
	1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};

static const uint8_t comp_perm[48] = {
	14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
	23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
	41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
	44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};

/*
 * No E box is used, as it's replaced by some ANDs, shifts, and ORs.
 */

static const uint8_t sbox[8][64] = {
	{
		14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
		 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
		 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
		15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13
	},
	{
		15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
		 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
		 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
		13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9
	},
	{
		10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
		13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
		13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
		 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12
	},
	{
		 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
		13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
		10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
		 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14
	},
	{
		 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
		14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
		 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
		11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3
	},
	{
		12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
		10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
		 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
		 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13
	},
	{
		 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
		13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
		 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
		 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12
	},
	{
		13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
		 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
		 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
		 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
	}
};

static const uint8_t pbox[32] = {
	16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
	 2,  8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25
};

static const uint32_t bits32[32] =
{
	0x80000000, 0x40000000, 0x20000000, 0x10000000,
	0x08000000, 0x04000000, 0x02000000, 0x01000000,
	0x00800000, 0x00400000, 0x00200000, 0x00100000,
	0x00080000, 0x00040000, 0x00020000, 0x00010000,
	0x00008000, 0x00004000, 0x00002000, 0x00001000,
	0x00000800, 0x00000400, 0x00000200, 0x00000100,
	0x00000080, 0x00000040, 0x00000020, 0x00000010,
	0x00000008, 0x00000004, 0x00000002, 0x00000001
};

static const uint8_t bits8[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };


static int 
ascii_to_bin(char ch)
{
	if (ch > 'z')
		return 0;
	if (ch >= 'a')
		return (ch - 'a' + 38);
	if (ch > 'Z')
		return 0;
	if (ch >= 'A')
		return (ch - 'A' + 12);
	if (ch > '9')
		return 0;
	if (ch >= '.')
		return (ch - '.');
	return 0;
}


/* Static stuff that stays resident and doesn't change after 
 * being initialized, and therefore doesn't need to be made 
 * reentrant. */
struct const_des_ctx {
	uint8_t	init_perm[64], final_perm[64]; /* referenced 2 times each */
	uint8_t	m_sbox[4][4096]; /* 5 times */
};
#define C (*cctx)
#define init_perm  (C.init_perm )
#define final_perm (C.final_perm)
#define m_sbox     (C.m_sbox    )

static struct const_des_ctx*
const_des_init(void)
{
	int i, j, b;
	uint8_t	u_sbox[8][64];
	struct const_des_ctx *cctx;

	cctx = xmalloc(sizeof(*cctx));

	/*
	 * Invert the S-boxes, reordering the input bits.
	 */
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 64; j++) {
			b = (j & 0x20) | ((j & 1) << 4) | ((j >> 1) & 0xf);
			u_sbox[i][j] = sbox[i][b];
		}
	}

	/*
	 * Convert the inverted S-boxes into 4 arrays of 8 bits.
	 * Each will handle 12 bits of the S-box input.
	 */
	for (b = 0; b < 4; b++)
		for (i = 0; i < 64; i++)
			for (j = 0; j < 64; j++)
				m_sbox[b][(i << 6) | j] =
					(uint8_t)((u_sbox[(b << 1)][i] << 4) |
						u_sbox[(b << 1) + 1][j]);

	/*
	 * Set up the initial & final permutations into a useful form.
	 */
	for (i = 0; i < 64; i++) {
		final_perm[i] = IP[i] - 1;
		init_perm[final_perm[i]] = (uint8_t)i;
	}

	return cctx;
}


struct des_ctx {
	const struct const_des_ctx *const_ctx;
	uint32_t saltbits; /* referenced 5 times */
	uint32_t old_salt; /* 3 times */
	uint32_t old_rawkey0, old_rawkey1; /* 3 times each */
	uint8_t	un_pbox[32]; /* 2 times */
	uint8_t	inv_comp_perm[56]; /* 3 times */
	uint8_t	inv_key_perm[64]; /* 3 times */
	uint32_t en_keysl[16], en_keysr[16]; /* 2 times each */
	uint32_t de_keysl[16], de_keysr[16]; /* 2 times each */
	uint32_t ip_maskl[8][256], ip_maskr[8][256]; /* 9 times each */
	uint32_t fp_maskl[8][256], fp_maskr[8][256]; /* 9 times each */
	uint32_t key_perm_maskl[8][128], key_perm_maskr[8][128]; /* 9 times */
	uint32_t comp_maskl[8][128], comp_maskr[8][128]; /* 9 times each */
	uint32_t psbox[4][256]; /* 5 times */
};
#define D (*ctx)
#define const_ctx       (D.const_ctx      )
#define saltbits        (D.saltbits       )
#define old_salt        (D.old_salt       )
#define old_rawkey0     (D.old_rawkey0    )
#define old_rawkey1     (D.old_rawkey1    )
#define un_pbox         (D.un_pbox        )
#define inv_comp_perm   (D.inv_comp_perm  )
#define inv_key_perm    (D.inv_key_perm   )
#define en_keysl        (D.en_keysl       )
#define en_keysr        (D.en_keysr       )
#define de_keysl        (D.de_keysl       )
#define de_keysr        (D.de_keysr       )
#define ip_maskl        (D.ip_maskl       )
#define ip_maskr        (D.ip_maskr       )
#define fp_maskl        (D.fp_maskl       )
#define fp_maskr        (D.fp_maskr       )
#define key_perm_maskl  (D.key_perm_maskl )
#define key_perm_maskr  (D.key_perm_maskr )
#define comp_maskl      (D.comp_maskl     )
#define comp_maskr      (D.comp_maskr     )
#define psbox           (D.psbox          )

static struct des_ctx*
des_init(struct des_ctx *ctx, const struct const_des_ctx *cctx)
{
	int i, j, b, k, inbit, obit;
	uint32_t *p, *il, *ir, *fl, *fr;
	const uint32_t *bits28, *bits24;

	if (!ctx)
		ctx = xmalloc(sizeof(*ctx));
	const_ctx = cctx;

	old_rawkey0 = old_rawkey1 = 0L;
	saltbits = 0L;
	old_salt = 0L;
	bits28 = bits32 + 4;
	bits24 = bits28 + 4;

	/*
	 * Initialise the inverted key permutation.
	 */
	for (i = 0; i < 64; i++) {
		inv_key_perm[i] = 255;
	}

	/*
	 * Invert the key permutation and initialise the inverted key
	 * compression permutation.
	 */
	for (i = 0; i < 56; i++) {
		inv_key_perm[key_perm[i] - 1] = (uint8_t)i;
		inv_comp_perm[i] = 255;
	}

	/*
	 * Invert the key compression permutation.
	 */
	for (i = 0; i < 48; i++) {
		inv_comp_perm[comp_perm[i] - 1] = (uint8_t)i;
	}

	/*
	 * Set up the OR-mask arrays for the initial and final permutations,
	 * and for the key initial and compression permutations.
	 */
	for (k = 0; k < 8; k++) {
		for (i = 0; i < 256; i++) {
			il = &ip_maskl[k][i];
			ir = &ip_maskr[k][i];
			fl = &fp_maskl[k][i];
			fr = &fp_maskr[k][i];
			*il = 0;
			*ir = 0;
			*fl = 0;
			*fr = 0;
			for (j = 0; j < 8; j++) {
				inbit = 8 * k + j;
				if (i & bits8[j]) {
					obit = init_perm[inbit];
					if (obit < 32)
						*il |= bits32[obit];
					else
						*ir |= bits32[obit - 32];
					obit = final_perm[inbit];
					if (obit < 32)
						*fl |= bits32[obit];
					else
						*fr |= bits32[obit - 32];
				}
			}
		}
		for (i = 0; i < 128; i++) {
			il = &key_perm_maskl[k][i];
			ir = &key_perm_maskr[k][i];
			*il = 0;
			*ir = 0;
			for (j = 0; j < 7; j++) {
				inbit = 8 * k + j;
				if (i & bits8[j + 1]) {
					obit = inv_key_perm[inbit];
					if (obit == 255)
						continue;
					if (obit < 28)
						*il |= bits28[obit];
					else
						*ir |= bits28[obit - 28];
				}
			}
			il = &comp_maskl[k][i];
			ir = &comp_maskr[k][i];
			*il = 0;
			*ir = 0;
			for (j = 0; j < 7; j++) {
				inbit = 7 * k + j;
				if (i & bits8[j + 1]) {
					obit = inv_comp_perm[inbit];
					if (obit == 255)
						continue;
					if (obit < 24)
						*il |= bits24[obit];
					else
						*ir |= bits24[obit - 24];
				}
			}
		}
	}

	/*
	 * Invert the P-box permutation, and convert into OR-masks for
	 * handling the output of the S-box arrays setup above.
	 */
	for (i = 0; i < 32; i++)
		un_pbox[pbox[i] - 1] = (uint8_t)i;

	for (b = 0; b < 4; b++) {
		for (i = 0; i < 256; i++) {
			p = &psbox[b][i];
			*p = 0;
			for (j = 0; j < 8; j++) {
				if (i & bits8[j])
					*p |= bits32[un_pbox[8 * b + j]];
			}
		}
	}

	return ctx;
}


static void
setup_salt(struct des_ctx *ctx, uint32_t salt)
{
//	const struct const_des_ctx *cctx = const_ctx;
	uint32_t obit, saltbit;
	int i;

	if (salt == old_salt)
		return;
	old_salt = salt;

	saltbits = 0L;
	saltbit = 1;
	obit = 0x800000;
	for (i = 0; i < 24; i++) {
		if (salt & saltbit)
			saltbits |= obit;
		saltbit <<= 1;
		obit >>= 1;
	}
}

static void
des_setkey(struct des_ctx *ctx, const char *key)
{
//	const struct const_des_ctx *cctx = const_ctx;
	uint32_t k0, k1, rawkey0, rawkey1;
	int shifts, round;

	rawkey0 = ntohl(*(const uint32_t *) key);
	rawkey1 = ntohl(*(const uint32_t *) (key + 4));

	if ((rawkey0 | rawkey1)
	 && rawkey0 == old_rawkey0
	 && rawkey1 == old_rawkey1
	) {
		/*
		 * Already setup for this key.
		 * This optimisation fails on a zero key (which is weak and
		 * has bad parity anyway) in order to simplify the starting
		 * conditions.
		 */
		return;
	}
	old_rawkey0 = rawkey0;
	old_rawkey1 = rawkey1;

	/*
	 *	Do key permutation and split into two 28-bit subkeys.
	 */
	k0 = key_perm_maskl[0][rawkey0 >> 25]
	   | key_perm_maskl[1][(rawkey0 >> 17) & 0x7f]
	   | key_perm_maskl[2][(rawkey0 >> 9) & 0x7f]
	   | key_perm_maskl[3][(rawkey0 >> 1) & 0x7f]
	   | key_perm_maskl[4][rawkey1 >> 25]
	   | key_perm_maskl[5][(rawkey1 >> 17) & 0x7f]
	   | key_perm_maskl[6][(rawkey1 >> 9) & 0x7f]
	   | key_perm_maskl[7][(rawkey1 >> 1) & 0x7f];
	k1 = key_perm_maskr[0][rawkey0 >> 25]
	   | key_perm_maskr[1][(rawkey0 >> 17) & 0x7f]
	   | key_perm_maskr[2][(rawkey0 >> 9) & 0x7f]
	   | key_perm_maskr[3][(rawkey0 >> 1) & 0x7f]
	   | key_perm_maskr[4][rawkey1 >> 25]
	   | key_perm_maskr[5][(rawkey1 >> 17) & 0x7f]
	   | key_perm_maskr[6][(rawkey1 >> 9) & 0x7f]
	   | key_perm_maskr[7][(rawkey1 >> 1) & 0x7f];
	/*
	 *	Rotate subkeys and do compression permutation.
	 */
	shifts = 0;
	for (round = 0; round < 16; round++) {
		uint32_t t0, t1;

		shifts += key_shifts[round];

		t0 = (k0 << shifts) | (k0 >> (28 - shifts));
		t1 = (k1 << shifts) | (k1 >> (28 - shifts));

		de_keysl[15 - round] =
		en_keysl[round] = comp_maskl[0][(t0 >> 21) & 0x7f]
				| comp_maskl[1][(t0 >> 14) & 0x7f]
				| comp_maskl[2][(t0 >> 7) & 0x7f]
				| comp_maskl[3][t0 & 0x7f]
				| comp_maskl[4][(t1 >> 21) & 0x7f]
				| comp_maskl[5][(t1 >> 14) & 0x7f]
				| comp_maskl[6][(t1 >> 7) & 0x7f]
				| comp_maskl[7][t1 & 0x7f];

		de_keysr[15 - round] =
		en_keysr[round] = comp_maskr[0][(t0 >> 21) & 0x7f]
				| comp_maskr[1][(t0 >> 14) & 0x7f]
				| comp_maskr[2][(t0 >> 7) & 0x7f]
				| comp_maskr[3][t0 & 0x7f]
				| comp_maskr[4][(t1 >> 21) & 0x7f]
				| comp_maskr[5][(t1 >> 14) & 0x7f]
				| comp_maskr[6][(t1 >> 7) & 0x7f]
				| comp_maskr[7][t1 & 0x7f];
	}
}


static int
do_des(struct des_ctx *ctx, uint32_t l_in, uint32_t r_in, uint32_t *l_out, uint32_t *r_out, int count)
{
	const struct const_des_ctx *cctx = const_ctx;
	/*
	 * l_in, r_in, l_out, and r_out are in pseudo-"big-endian" format.
	 */
	uint32_t l, r, *kl, *kr, *kl1, *kr1;
	uint32_t f = f; /* silence gcc */
	uint32_t r48l, r48r;
	int round;

	/*
	 * Encrypting
	 */
	kl1 = en_keysl;
	kr1 = en_keysr;

	/*
	 *	Do initial permutation (IP).
	 */
	l = ip_maskl[0][l_in >> 24]
	  | ip_maskl[1][(l_in >> 16) & 0xff]
	  | ip_maskl[2][(l_in >> 8) & 0xff]
	  | ip_maskl[3][l_in & 0xff]
	  | ip_maskl[4][r_in >> 24]
	  | ip_maskl[5][(r_in >> 16) & 0xff]
	  | ip_maskl[6][(r_in >> 8) & 0xff]
	  | ip_maskl[7][r_in & 0xff];
	r = ip_maskr[0][l_in >> 24]
	  | ip_maskr[1][(l_in >> 16) & 0xff]
	  | ip_maskr[2][(l_in >> 8) & 0xff]
	  | ip_maskr[3][l_in & 0xff]
	  | ip_maskr[4][r_in >> 24]
	  | ip_maskr[5][(r_in >> 16) & 0xff]
	  | ip_maskr[6][(r_in >> 8) & 0xff]
	  | ip_maskr[7][r_in & 0xff];

	while (count--) {
		/*
		 * Do each round.
		 */
		kl = kl1;
		kr = kr1;
		round = 16;
		while (round--) {
			/*
			 * Expand R to 48 bits (simulate the E-box).
			 */
			r48l	= ((r & 0x00000001) << 23)
				| ((r & 0xf8000000) >> 9)
				| ((r & 0x1f800000) >> 11)
				| ((r & 0x01f80000) >> 13)
				| ((r & 0x001f8000) >> 15);

			r48r	= ((r & 0x0001f800) << 7)
				| ((r & 0x00001f80) << 5)
				| ((r & 0x000001f8) << 3)
				| ((r & 0x0000001f) << 1)
				| ((r & 0x80000000) >> 31);
			/*
			 * Do salting for crypt() and friends, and
			 * XOR with the permuted key.
			 */
			f = (r48l ^ r48r) & saltbits;
			r48l ^= f ^ *kl++;
			r48r ^= f ^ *kr++;
			/*
			 * Do sbox lookups (which shrink it back to 32 bits)
			 * and do the pbox permutation at the same time.
			 */
			f = psbox[0][m_sbox[0][r48l >> 12]]
			  | psbox[1][m_sbox[1][r48l & 0xfff]]
			  | psbox[2][m_sbox[2][r48r >> 12]]
			  | psbox[3][m_sbox[3][r48r & 0xfff]];
			/*
			 * Now that we've permuted things, complete f().
			 */
			f ^= l;
			l = r;
			r = f;
		}
		r = l;
		l = f;
	}
	/*
	 * Do final permutation (inverse of IP).
	 */
	*l_out	= fp_maskl[0][l >> 24]
		| fp_maskl[1][(l >> 16) & 0xff]
		| fp_maskl[2][(l >> 8) & 0xff]
		| fp_maskl[3][l & 0xff]
		| fp_maskl[4][r >> 24]
		| fp_maskl[5][(r >> 16) & 0xff]
		| fp_maskl[6][(r >> 8) & 0xff]
		| fp_maskl[7][r & 0xff];
	*r_out	= fp_maskr[0][l >> 24]
		| fp_maskr[1][(l >> 16) & 0xff]
		| fp_maskr[2][(l >> 8) & 0xff]
		| fp_maskr[3][l & 0xff]
		| fp_maskr[4][r >> 24]
		| fp_maskr[5][(r >> 16) & 0xff]
		| fp_maskr[6][(r >> 8) & 0xff]
		| fp_maskr[7][r & 0xff];
	return 0;
}

#define DES_OUT_BUFSIZE 21

static char *
des_crypt(struct des_ctx *ctx, char output[21], const unsigned char *key, const unsigned char *setting)
{
	uint32_t salt, l, r0, r1, keybuf[2];
	uint8_t *p, *q;

	/*
	 * Copy the key, shifting each character up by one bit
	 * and padding with zeros.
	 */
	q = (uint8_t *)keybuf;
	while (q - (uint8_t *)keybuf - 8) {
		*q++ = *key << 1;
		if (*(q - 1))
			key++;
	}
	des_setkey(ctx, (char *)keybuf);

	/*
	 * setting - 2 bytes of salt
	 * key - up to 8 characters
	 */
	salt = (ascii_to_bin(setting[1]) << 6)
	     |  ascii_to_bin(setting[0]);

	output[0] = setting[0];
	/*
	 * If the encrypted password that the salt was extracted from
	 * is only 1 character long, the salt will be corrupted.  We
	 * need to ensure that the output string doesn't have an extra
	 * NUL in it!
	 */
	output[1] = setting[1] ? setting[1] : output[0];

	p = (uint8_t *)output + 2;

	setup_salt(ctx, salt);
	/*
	 * Do it.
	 */
	do_des(ctx, 0L, 0L, &r0, &r1, 25 /* count */);

	/*
	 * Now encode the result...
	 */
	l = (r0 >> 8);
	*p++ = ascii64[(l >> 18) & 0x3f];
	*p++ = ascii64[(l >> 12) & 0x3f];
	*p++ = ascii64[(l >> 6) & 0x3f];
	*p++ = ascii64[l & 0x3f];

	l = (r0 << 16) | ((r1 >> 16) & 0xffff);
	*p++ = ascii64[(l >> 18) & 0x3f];
	*p++ = ascii64[(l >> 12) & 0x3f];
	*p++ = ascii64[(l >> 6) & 0x3f];
	*p++ = ascii64[l & 0x3f];

	l = r1 << 2;
	*p++ = ascii64[(l >> 12) & 0x3f];
	*p++ = ascii64[(l >> 6) & 0x3f];
	*p++ = ascii64[l & 0x3f];
	*p = 0;

	return output;
}

#undef C
#undef init_perm
#undef final_perm
#undef m_sbox
#undef D
#undef const_ctx
#undef saltbits
#undef old_salt
#undef old_rawkey0
#undef old_rawkey1
#undef un_pbox
#undef inv_comp_perm
#undef inv_key_perm
#undef en_keysl
#undef en_keysr
#undef de_keysl
#undef de_keysr
#undef ip_maskl
#undef ip_maskr
#undef fp_maskl
#undef fp_maskr
#undef key_perm_maskl
#undef key_perm_maskr
#undef comp_maskl
#undef comp_maskr
#undef psbox
