/*
 * test_bearssl_patterns.c - compile-only smokes for AC fixes needed by
 * AmiTLS BearSSL (T0 bytecode, PRF seed init, sizeof-cast macros, etc.).
 *
 * Mirrors patterns from third_party/bearssl ssl/x509 sources without
 * pulling that tree into the unittest suite.
 */
typedef unsigned long u32;

typedef struct {
	unsigned char pad[32];
	int x;
} br_ctx;

/* BearSSL-style CTX cast: sizeof CTX(p)->pad must parse as sizeof((cast)->pad). */
#define CTX(p) ((br_ctx *)(void *)(p))

/* T0 7-bit encoding macros (ssl_hs_client.c / x509_*.c). */
#define T0_VBYTE(x, n)   (unsigned char)((((u32)(x) >> (n)) & 0x7F) | 0x80)
#define T0_FBYTE(x, n)   (unsigned char)(((u32)(x) >> (n)) & 0x7F)
#define T0_INT1(x)       T0_FBYTE(x, 0)
#define T0_INT2(x)       T0_VBYTE(x, 7), T0_FBYTE(x, 0)

#define BR_KEYTYPE_RSA    1
#define BR_KEYTYPE_EC     2
#define BR_KEYTYPE_KEYX   0x10
#define BR_KEYTYPE_SIGN   0x20
#define BR_X509_BUFSIZE_SIG 512

/* BR_HASHDESC_* style: shifts and ORs must fold as 32-bit, not 16-bit. */
#define BR_HASHDESC(id, out, state, blen) \
	(((u32)(id)) \
	 | (((u32)(out)) << 8) \
	 | (((u32)(state)) << 16) \
	 | (((u32)(blen)) << 24))

typedef struct {
	const void *data;
	unsigned long len;
} br_tls_prf_seed_chunk;

static void
hs_callback(ctx)
	void *ctx;
{
	(void) ctx;
}

/*
 * Exceeds the former MAX_INIT_ELEMS (512) safety cap that broke T0 tables.
 */
static const unsigned char t0_pad[520] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
	0, 1, 2, 3, 4, 5, 6, 7,
};

/* T0_INT1 with bitwise OR; T0_INT2 expands to two comma-separated bytes. */
static const unsigned char t0_code[] = {
	T0_INT1(BR_KEYTYPE_SIGN),
	T0_INT1(BR_KEYTYPE_RSA | BR_KEYTYPE_KEYX),
	T0_INT1(BR_KEYTYPE_RSA | BR_KEYTYPE_SIGN),
	T0_INT1(BR_KEYTYPE_EC | BR_KEYTYPE_SIGN),
	T0_INT1(BR_KEYTYPE_EC | BR_KEYTYPE_KEYX),
	T0_INT2(BR_X509_BUFSIZE_SIG),
	0x00
};

static const u32 hashdesc = BR_HASHDESC(6, 32, 32, 64);

static int
switch_decl_before_case(v)
	int v;
{
	switch (v) {
		int y;
	case 0:
		y = 10;
		return y;
	case 1:
		y = 20;
		return y;
	default:
		return -1;
	}
}

int
main(void)
{
	br_ctx st;
	void *p;
	void (*fp)(void *);
	br_tls_prf_seed_chunk seed[2] = {
		{ (const void *) &st, sizeof st.pad },
		{ (const void *) &st, sizeof(int) }
	};
	int n;

	p = (void *) &st;
	/* Function designator compatible with void (*)(void *). */
	fp = hs_callback;
	fp(p);

	n = (int) sizeof CTX(p)->pad;
	n = n + (int) seed[0].len + (int) seed[1].len;
	n = n + (int) t0_code[0] + (int) t0_code[1];
	n = n + (int) t0_pad[0] + (int) t0_pad[519];
	n = n + switch_decl_before_case(0);
	n = n + (int) (hashdesc >> 16);
	n = n + (int) (1L << 20);
	return n;
}
