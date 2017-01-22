/*
 *  based on:
 *  ssss version 0.5  -  Copyright 2005,2006 B. Poettering
 *
 *  refactored by Christopher Hall 2017-01-18
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <gmp.h>

#include "shamir.h"

typedef struct {
	unsigned int degree;
	mpz_t poly;
} poly_degree_t;


#define mpz_lshift(A, B, l) mpz_mul_2exp(A, B, l)
#define mpz_sizeinbits(A) (mpz_cmp_ui(A, 0) ? mpz_sizeinbase(A, 2) : 0)


// field arithmetic routines

int field_size_valid(int deg) {
	return (deg >= 8) && (deg <= MAXDEGREE) && (deg % 8 == 0);
}

// initialize 'poly' to a bitfield representing the coefficients of an
// irreducible polynomial of degree 'deg'

void field_init(poly_degree_t *pd, int deg)
{
	// coefficients of some irreducible polynomials over GF(2)
	static const unsigned char irred_coeff[] = {
		4,3,1,5,3,1,4,3,1,7,3,2,5,4,3,5,3,2,7,4,2,4,3,1,10,9,3,9,4,2,7,6,2,10,9,
		6,4,3,1,5,4,3,4,3,1,7,2,1,5,3,2,7,4,2,6,3,2,5,3,2,15,3,2,11,3,2,9,8,7,7,
		2,1,5,3,2,9,3,1,7,3,1,9,8,3,9,4,2,8,5,3,15,14,10,10,5,2,9,6,2,9,3,2,9,5,
		2,11,10,1,7,3,2,11,2,1,9,7,4,4,3,1,8,3,1,7,4,1,7,2,1,13,11,6,5,3,2,7,3,2,
		8,7,5,12,3,2,13,10,6,5,3,2,5,3,2,9,5,2,9,7,2,13,4,3,4,3,1,11,6,4,18,9,6,
		19,18,13,11,3,2,15,9,6,4,3,1,16,5,2,15,14,6,8,5,2,15,11,2,11,6,2,7,5,3,8,
		3,1,19,16,9,11,9,6,15,7,6,13,4,3,14,13,3,13,6,3,9,5,2,19,13,6,19,10,3,11,
		6,5,9,2,1,14,3,2,13,3,1,7,5,4,11,9,8,11,6,5,23,16,9,19,14,6,23,10,2,8,3,
		2,5,4,3,9,6,4,4,3,2,13,8,6,13,11,1,13,10,3,11,6,5,19,17,4,15,14,7,13,9,6,
		9,7,3,9,7,1,14,3,2,11,8,2,11,6,4,13,5,2,11,5,1,11,4,1,19,10,3,21,10,6,13,
		3,1,15,7,5,19,18,10,7,5,3,12,7,2,7,5,1,14,9,6,10,3,2,15,13,12,12,11,9,16,
		9,7,12,9,3,9,5,2,17,10,6,24,9,3,17,15,13,5,4,3,19,17,8,15,6,3,19,6,1 };

	assert(field_size_valid(deg));
	mpz_init_set_ui(pd->poly, 0);
	mpz_setbit(pd->poly, deg);
	mpz_setbit(pd->poly, irred_coeff[3 * (deg / 8 - 1) + 0]);
	mpz_setbit(pd->poly, irred_coeff[3 * (deg / 8 - 1) + 1]);
	mpz_setbit(pd->poly, irred_coeff[3 * (deg / 8 - 1) + 2]);
	mpz_setbit(pd->poly, 0);
	pd->degree = deg;
}

void field_deinit(poly_degree_t *pd) {
	mpz_clear(pd->poly);
	pd->degree = 0;
}

// I/O routines for GF(2^deg) field elements

error_t field_import(const unsigned int degree, mpz_t x, const char *s, int hexmode) {
	if (hexmode) {
		if (strlen(s) > degree / 4) {
			return ERROR_INPUT_STRING_TOO_LONG;
		}
		if (strlen(s) < degree / 4) {
			return ERROR_INPUT_STRING_TOO_SHORT;
		}
		if (mpz_set_str(x, s, 16) || (mpz_cmp_ui(x, 0) < 0)) {
			return ERROR_INVALID_SYNTAX;
		}
	} else {
		int i;
		int warn = 0;
		if (strlen(s) > degree / 8) {
			return ERROR_INPUT_STRING_TOO_LONG;
		}
		for(i = strlen(s) - 1; i >= 0; i--) {
			warn = warn || (s[i] < 32) || (s[i] >= 127);
		}
		if (warn) {
			return ERROR_BINARY_DATA;
		}
		mpz_import(x, strlen(s), 1, 1, 0, 0, s);
	}
	return ERROR_OK;
}

// print a field in hex or ASCII with optional prefix and share count
error_t field_print(char *buffer, size_t size, const char *prefix, int format_length, int number, const unsigned int degree, const mpz_t x, bool hexmode) {

	// ensure clear buffer
	memset(buffer, 0, size);

	// prefix
	if (NULL != prefix) {
		size_t n = snprintf(buffer, size, "%s-", prefix);
		if (n > size) {
			return ERROR_BUFFER_TOO_SMALL;
		}
		size -= n;
		buffer += n;
	}

	// count
	if (0 != format_length) {
		size_t n = snprintf(buffer, size, "%0*d-", format_length, number);
		if (n > size) {
			return ERROR_BUFFER_TOO_SMALL;
		}
		size -= n;
		buffer += n;
	}

	// share
	if (hexmode) {
		size_t s = mpz_sizeinbase(x, 16);
		if (size < s + 2) {
			return ERROR_BUFFER_TOO_SMALL;
		}
		for(int i = degree / 4 - s; i; i--) {
			*buffer++ = '0';
			--size;
		}
		//mpz_out_str(stream, 16, x);
		mpz_get_str(buffer, 16, x);
	} else {
		char buf[MAXDEGREE / 8 + 1];
		size_t t;
		int warn = 0;
		memset(buf, degree / 8 + 1, 0);
		mpz_export(buf, &t, 1, 1, 0, 0, x);
		for(size_t i = 0; i < t; i++) {
			int printable = (buf[i] >= 32) && (buf[i] < 127);
			warn = warn || ! printable;
			//fprintf(stream, "%c", printable ? buf[i] : '.');
			*buffer++ = printable ? buf[i] : '.';
			--size;
			if (size <= 1) {
				return ERROR_BUFFER_TOO_SMALL;
			}
		}
		if (warn) {
			return ERROR_BINARY_DATA;
		}
	}
	return ERROR_OK;
}


// basic field arithmetic in GF(2^deg)

void field_add(mpz_t z, const mpz_t x, const mpz_t y) {
	mpz_xor(z, x, y);
}

void field_mult(mpz_t z, const mpz_t x, const mpz_t y, poly_degree_t *pd) {
	mpz_t b;
	unsigned int i;
	assert(z != y);
	mpz_init_set(b, x);
	if (mpz_tstbit(y, 0)) {
		mpz_set(z, b);
	} else {
		mpz_set_ui(z, 0);
	}
	for(i = 1; i < pd->degree; i++) {
		mpz_lshift(b, b, 1);
		if (mpz_tstbit(b, pd->degree)) {
			mpz_xor(b, b, pd->poly);
		}
		if (mpz_tstbit(y, i)) {
			mpz_xor(z, z, b);
		}
	}
	mpz_clear(b);
}

void field_invert(mpz_t z, const mpz_t x, poly_degree_t *pd) {
	mpz_t u, v, g, h;
	int i;
	assert(mpz_cmp_ui(x, 0));
	mpz_init_set(u, x);
	mpz_init_set(v, pd->poly);
	mpz_init_set_ui(g, 0);
	mpz_set_ui(z, 1);
	mpz_init(h);
	while (mpz_cmp_ui(u, 1)) {
		i = mpz_sizeinbits(u) - mpz_sizeinbits(v);
		if (i < 0) {
			mpz_swap(u, v);
			mpz_swap(z, g);
			i = -i;
		}
		mpz_lshift(h, v, i);
		mpz_xor(u, u, h);
		mpz_lshift(h, g, i);
		mpz_xor(z, z, h);
	}
	mpz_clear(u); mpz_clear(v); mpz_clear(g); mpz_clear(h);
}

// routines for the random number generator

error_t cprng_init(int *cprng) {
	if (NULL == cprng) {
		return ERROR_CANNOT_OPEN_RANDOM;
	}
	int fd = open(RANDOM_SOURCE, O_RDONLY);
	if (fd < 0) {
		return ERROR_CANNOT_OPEN_RANDOM;
	}
	*cprng = fd;
	return ERROR_OK;

}

error_t cprng_deinit(int *cprng) {
	if (NULL == cprng) {
		return ERROR_CANNOT_CLOSE_RANDOM;
	}
	if (-1 == *cprng) {
		return ERROR_OK; // already closed
	}
	if (close(*cprng) < 0) {
		return ERROR_CANNOT_CLOSE_RANDOM;
	}
	*cprng = -1;
	return ERROR_OK;
}

error_t cprng_read(int *cprng, const unsigned int degree, mpz_t x) {
	if (NULL == cprng) {
		return ERROR_CANNOT_READ_RANDOM;
	}
	char buf[MAXDEGREE / 8];
	unsigned int count;
	int i;
	for(count = 0; count < degree / 8; count += i) {
		if ((i = read(*cprng, buf + count, degree / 8 - count)) < 0) {
			close(*cprng);
			return ERROR_CANNOT_READ_RANDOM;
		}
	}
	mpz_import(x, degree / 8, 1, 1, 0, 0, buf);
	return ERROR_OK;
}

// a 64 bit pseudo random permutation (based on the XTEA cipher)

void encipher_block(uint32_t *v) {
	uint32_t sum = 0, delta = 0x9E3779B9;
	int i;
	for(i = 0; i < 32; i++) {
		v[0] += (((v[1] << 4) ^ (v[1] >> 5)) + v[1]) ^ sum;
		sum += delta;
		v[1] += (((v[0] << 4) ^ (v[0] >> 5)) + v[0]) ^ sum;
	}
}

void decipher_block(uint32_t *v) {
	uint32_t sum = 0xC6EF3720, delta = 0x9E3779B9;
	int i;
	for(i = 0; i < 32; i++) {
		v[1] -= ((v[0] << 4 ^ v[0] >> 5) + v[0]) ^ sum;
		sum -= delta;
		v[0] -= ((v[1] << 4 ^ v[1] >> 5) + v[1]) ^ sum;
	}
}

void encode_slice(uint8_t *data, int idx, int len,  void (*process_block)(uint32_t*)) {
	uint32_t v[2];
	int i;
	for(i = 0; i < 2; i++) {
		v[i] = data[(idx + 4 * i) % len] << 24 |
			data[(idx + 4 * i + 1) % len] << 16 |
			data[(idx + 4 * i + 2) % len] << 8 | data[(idx + 4 * i + 3) % len];
	}
	process_block(v);
	for(i = 0; i < 2; i++) {
		data[(idx + 4 * i + 0) % len] = v[i] >> 24;
		data[(idx + 4 * i + 1) % len] = (v[i] >> 16) & 0xff;
		data[(idx + 4 * i + 2) % len] = (v[i] >> 8) & 0xff;
		data[(idx + 4 * i + 3) % len] = v[i] & 0xff;
	}
}

enum encdec {ENCODE, DECODE};

void encode_mpz(const unsigned int degree, mpz_t x, enum encdec encdecmode) {
	uint8_t v[(MAXDEGREE + 8) / 16 * 2];
	size_t t;
	int i;
	memset(v, 0, (degree + 8) / 16 * 2);
	mpz_export(v, &t, -1, 2, 1, 0, x);
	if (degree % 16 == 8) {
		v[degree / 8 - 1] = v[degree / 8];
	}
	if (encdecmode == ENCODE) {            // 40 rounds are more than enough!
		for(i = 0; i < 40 * ((int)degree / 8); i += 2) {
			encode_slice(v, i, degree / 8, encipher_block);
		}
	} else {
		for(i = 40 * (degree / 8) - 2; i >= 0; i -= 2) {
			encode_slice(v, i, degree / 8, decipher_block);
		}
	}
	if (degree % 16 == 8) {
		v[degree / 8] = v[degree / 8 - 1];
		v[degree / 8 - 1] = 0;
	}
	mpz_import(x, (degree + 8) / 16, -1, 2, 1, 0, v);
	assert(mpz_sizeinbits(x) <= degree);
}

// evaluate polynomials efficiently

void horner(int n, mpz_t y, const mpz_t x, const mpz_t coeff[], poly_degree_t *pd) {
	int i;
	mpz_set(y, x);
	for(i = n - 1; i; i--) {
		field_add(y, y, coeff[i]);
		field_mult(y, y, x, pd);
	}
	field_add(y, y, coeff[0]);
}

// calculate the secret from a set of shares solving a linear equation system

#define MPZ_SWAP(A, B)                                          \
	do {                                                    \
		mpz_set(h, A); mpz_set(A, B); mpz_set(B, h);    \
	} while(0)

int restore_secret(int n, mpz_t (*A)[n], mpz_t b[], poly_degree_t *pd) {
	mpz_t (*AA)[n] = (mpz_t (*)[n])A;
	int i, j, k, found;
	mpz_t h;
	mpz_init(h);
	for(i = 0; i < n; i++) {
		if (! mpz_cmp_ui(AA[i][i], 0)) {
			for(found = 0, j = i + 1; j < n; j++) {
				if (mpz_cmp_ui(AA[i][j], 0)) {
					found = 1;
					break;
				}
			}
			if (! found) {
				return -1;
			}
			for(k = i; k < n; k++) {
				MPZ_SWAP(AA[k][i], AA[k][j]);
			}
			MPZ_SWAP(b[i], b[j]);
		}
		for(j = i + 1; j < n; j++) {
			if (mpz_cmp_ui(AA[i][j], 0)) {
				for(k = i + 1; k < n; k++) {
					field_mult(h, AA[k][i], AA[i][j], pd);
					field_mult(AA[k][j], AA[k][j], AA[i][i], pd);
					field_add(AA[k][j], AA[k][j], h);
				}
				field_mult(h, b[i], AA[i][j], pd);
				field_mult(b[j], b[j], AA[i][i], pd);
				field_add(b[j], b[j], h);
			}
		}
	}
	field_invert(h, AA[n - 1][n - 1], pd);
	field_mult(b[n - 1], b[n - 1], h, pd);
	mpz_clear(h);
	return 0;
}


// generate shares for a secret
error_t split(const char *secret, process_share_t *process_share, void *data,
	      int security, int threshold, int number, bool diffusion,
	      char *prefix, bool hexmode) {

	mpz_t x, y, coeff[threshold];

	if (0 == security) {
		security = hexmode ? 4 * ((strlen(secret) + 1) & ~1): 8 * strlen(secret);
		if (! field_size_valid(security)) {
			return ERROR_INVALID_SECURITY_LEVEL;
		}
	}

	unsigned int format_length = 0;
	int i = 0;
	for(format_length = 1, i = number; i >= 10; i /= 10, ++format_length) {
	}

	poly_degree_t pd;
	field_init(&pd, security);

	mpz_init(coeff[0]);
	error_t err = field_import(pd.degree, coeff[0], secret, hexmode);
	if (ERROR_OK != err) {
		return err;
	}

	if (diffusion) {
		if (pd.degree >= 64) {
			encode_mpz(pd.degree, coeff[0], ENCODE);
		} else {
			return ERROR_SECURITY_LEVEL_TOO_SMALL_FOR_DIFFUSION;
		}
	}

	int cprng;
	err = cprng_init(&cprng);
	if (ERROR_OK != err) {
		return err;
	}

	for(int i = 1; i < threshold; i++) {
		mpz_init(coeff[i]);
		err = cprng_read(&cprng, pd.degree, coeff[i]);
		if (ERROR_OK != err) {
			return err;
		}
	}
	err = cprng_deinit(&cprng);
	if (ERROR_OK != err) {
		return err;
	}

	mpz_init(x);
	mpz_init(y);
	for(int i = 0; i < number; i++) {
		mpz_set_ui(x, i + 1);
		horner(threshold, y, x, (const mpz_t*)coeff, &pd);
		char buffer[MAXLINELEN];
		err = field_print(buffer, sizeof(buffer), prefix, format_length, i + 1, pd.degree, y, true);
		if (ERROR_OK == err) {
			process_share(data, buffer, strlen(buffer), i + 1, number);
		}
	}
	mpz_clear(x);
	mpz_clear(y);

	for(int i = 0; i < threshold; i++) {
		mpz_clear(coeff[i]);
	}
	field_deinit(&pd);

	return ERROR_OK;
}


// calculate the secret from shares

error_t combine(char *secret, size_t secret_size, read_share_t *get_share, void *data, int threshold, bool diffusion, bool hexmode) {

	mpz_t A[threshold][threshold], y[threshold], x;
	unsigned s = 0;

	poly_degree_t pd;

	mpz_init(x);

	for (int i = 0; i < threshold; i++) {

		char buffer[MAXLINELEN];
		const char *input = get_share(data, i + 1, threshold, sizeof(buffer) - 1);
		if (NULL == input) {
			return ERROR_INPUT_IS_NULL;
		}
		strncpy(buffer, input, sizeof(buffer));
		buffer[sizeof(buffer) - 1] = '\0'; // ensure null terminated
		buffer[strcspn(buffer, "\r\n")] = '\0';

		char *a, *b;

		if (! (a = strchr(buffer, '-'))) {
			return ERROR_INVALID_SYNTAX;
		}
		*a++ = 0;
		if ((b = strchr(a, '-'))) {
			*b++ = 0;
		} else {
			b = a, a = buffer;
		}
		if (! s) {
			s = 4 * strlen(b);
			if (! field_size_valid(s)) {
				return ERROR_SHARE_HAS_ILLEGAL_LENGTH;
			}
			field_init(&pd, s);
		} else {
			if (s != 4 * strlen(b)) {
				return ERROR_SHARES_HAVE_DIFFERENT_SECURITY_LEVELS;
			}
		}
		int j;
		if (! (j = atoi(a))) {
			return ERROR_INVALID_SHARE;
		}
		mpz_set_ui(x, j);
		mpz_init_set_ui(A[threshold - 1][i], 1);
		for(j = threshold - 2; j >= 0; j--) {
			mpz_init(A[j][i]);
			field_mult(A[j][i], A[j + 1][i], x, &pd);
		}
		mpz_init(y[i]);
		field_import(pd.degree, y[i], b, 1);
		field_mult(x, x, A[0][i], &pd);
		field_add(y[i], y[i], x);
	}
	mpz_clear(x);
	if (restore_secret(threshold, A, y, &pd)) {
		return ERROR_SHARES_INCONSISTENT;
	}
	if (diffusion) {
		if (pd.degree >= 64) {
			encode_mpz(pd.degree, y[threshold - 1], DECODE);
		} else {
			return ERROR_SECURITY_LEVEL_TOO_SMALL_FOR_DIFFUSION;

		}
	}

	error_t err = field_print(secret, secret_size, NULL, 0, 0, pd.degree, y[threshold - 1], hexmode);

	// clean up
	for (int i = 0; i < threshold; i++) {
		for (int j = 0; j < threshold; j++) {
			mpz_clear(A[i][j]);
		}
		mpz_clear(y[i]);
	}
	field_deinit(&pd);

	return err;
}
