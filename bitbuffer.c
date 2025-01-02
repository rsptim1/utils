// Bitbuffer implementation
// TSullivan

#ifndef BITBUFFER
#define BITBUFFER

#define BB_WRITE 0
#define BB_READ  1
#define BB_PEEK  2

#define BB_ASCIIBITS 7

typedef struct _BitBuffer {
	unsigned char* chunks;
	unsigned int r, w;
	unsigned char mode;
} BB;

void BBWrite(BB* buff, void* value, int bits) {
	unsigned short head, result;
	unsigned char* chunks, * v;
	int i, u;

	u = buff->w & 0x07;
	i = buff->w >> 3;

	v = (unsigned char*)value;
	chunks = buff->chunks + i;
	head = *chunks & ((1 << u) - 1);
	result = head | ((unsigned short)(*v & ((1 << bits) - 1)) << u);

	*chunks++ = (unsigned char)result;
	*chunks = (*chunks & ~(((1 << (u + bits)) - 1) >> 8)) | (unsigned char)(result >> 8);
	buff->w += bits;
}

void BBPeek(BB* buff, void* value, int bits) {
	unsigned short head;
	unsigned char* v;
	int u;

	u = buff->r & 0x07;
	v = buff->chunks + (buff->r >> 3);
	head = *v | ((unsigned short)*(v + 1) << 8);
	*(v = value) = (unsigned char)((head & (((1 << bits) - 1) << u)) >> u);
}

void BBRead(BB* b, void* val, int bits) {
	BBPeek(b, val, bits);
	b->r += bits;
}

int BBLength(unsigned int rw) {
	return ((rw - 1) >> 3) + 1;
}

typedef void(*BBOp)(BB*, void*, int);
BBOp bufferOps[] = {
	BBWrite, BBRead, BBPeek
};

void BBProcess(BB* b, void* val, unsigned char bytes) {
	unsigned char* v = val;
	for (v += bytes - 1; bytes--; bufferOps[b->mode](b, v--, 8));
}

void BBProcessBits(BB* b, void* val, unsigned char bits) {
	unsigned char* v = val, n;

	v += (bits >> 3) + ((((bits & 7) + 7) & ~7) >> 3) - 1;
	if (n = bits & 7) bufferOps[b->mode](b, v--, n);
	for (bits -= n; bits; bits -= 8) bufferOps[b->mode](b, v--, 8);
}

void BBProcessStr(BB* b, char* s) {
	do bufferOps[b->mode](b, s, BB_ASCIIBITS);
	while (*s++);
}

#endif
