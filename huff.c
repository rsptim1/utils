// Huffman tree implementation
// TSullivan

#ifndef HUFF
#define HUFF

#include <stdlib.h>
#include <x86gprintrin.h>

#define HUFF_HEADER_BITS 2
#define HUFF_SINGLE 	0b00
#define HUFF_NONE		0b01
#define HUFF_RESERV0 	0b10
#define HUFF_RESERV0 	0b11

// -------------
// Decompression
// -------------

int huffDecode(unsigned int bits, signed int* tree, unsigned int* out) {
	int i, r;
	for (r = 1, i = bits & 1; *(tree + i) < 0; i = (bits >>= 1) & 1, ++r) tree -= *(tree + i);
	*out = (unsigned int)*(tree + i);
	return r;
}

// -----------
// Compression
// -----------

typedef struct _huffCode {
	unsigned int value, code;
	unsigned char bits;
} huffCode;

typedef struct _huffTree {
	huffCode* codes;
	unsigned int cnt;
} huffTree;

typedef struct _huffNode {
	unsigned int val, children;
	float freq;
	struct _huffNode* zero, * one, * parent;
} huffNode;

huffCode* huffEncode(huffTree* t, unsigned int v) {
	huffCode* c;
	// This is brute-forced and slow, but the original compressor wasn't intended to be fast
	for (c = t->codes; c->value != v; ++c);
	return c;
}

huffNode* huffSort(huffNode* nodes, unsigned int fcnt) {
	huffNode* node, * low, * nextlow;
	int i, j;

	if (fcnt == 1) return nodes;

	for (i = fcnt - 1; i--; ++fcnt) {
		// Find lowest node
		for (low = node = nodes, j = fcnt; j--; ++node) {
			if (node->parent) continue;
			if (low->parent || node->freq < low->freq) low = node;
		}
		// Find second-lowest node
		nextlow = nodes + (low == nodes);
		for (node = nodes, j = fcnt; j--; ++node) {
			if (node->parent || node == low) continue;
			if (nextlow->parent || node->freq < nextlow->freq) nextlow = node;
		}
		if (low->children < nextlow->children) { node->zero = low; node->one = nextlow; }
		else { node->zero = nextlow; node->one = low; }
		node->children = nextlow->children + low->children;
		node->freq = low->freq + nextlow->freq;
		node->parent = 0;
		low->parent = nextlow->parent = node;
	}

	return node;
}

void huffAssign(huffNode* head, huffTree* tree) {
	huffNode* node, ** b;
	int i, j, t, h, l, ex, nex;
	unsigned int m, k;

	// Edge case, screws up below formula
	if (tree->cnt == 1) {
		tree->codes[0] = (huffCode){ head->val, 1, 0 };
		return;
	}

	nex = i = k = j = t = 0;
	b = malloc(2 * tree->cnt * sizeof(huffNode**));
	ex = h = 2;
	l = 2 * tree->cnt;
	b[0] = head->zero;
	b[1] = head->one;

	while (t != h) {
		node = b[t++];
		t %= l; --ex;

		if (node->zero) {
			b[h++] = node->zero; h %= l;
			b[h++] = node->one; h %= l;
			nex += 2;

			if (!ex) { // End of row
				++i;
				k <<= 1;
				ex = nex;
				nex = 0;
			}
		}
		else {
			m = k++;
			m = (m & 0x55555555) << 1 | (m & 0xAAAAAAAA) >> 1;
			m = (m & 0x33333333) << 2 | (m & 0xCCCCCCCC) >> 2;
			m = (m & 0x0F0F0F0F) << 4 | (m & 0xF0F0F0F0) >> 4;
			m = (m & 0x00FF00FF) << 8 | (m & 0xFF00FF00) >> 8;
			m = (m & 0x0000FFFF) << 16 | (m & 0xFFFF0000) >> 16;
			m >>= 31 - i;
			tree->codes[j++] = (huffCode){ node->val, m, i + 1 };
		}
	}
	free(b);
}

huffTree* huffConstruct(unsigned int* v, unsigned int n) {
	int i, j;
	struct prob { unsigned int v, f; } *freqs, * q;
	unsigned int cnt, * val;
	huffNode* head, * node;
	huffTree* ret;

	freqs = malloc(sizeof(struct prob) * n);

	for (cnt = 0, i = n, val = v; i--; ++val) {
		for (j = cnt, q = &freqs[0]; j--; ++q) if (q->v == val) { q->f++; goto huffConstructContinue; }
		freqs[cnt++] = (struct prob){ val, 1 };
	huffConstructContinue:
		continue;
	}

	head = malloc(sizeof(huffNode) * 2 * cnt);
	ret = malloc(sizeof(huffTree) + cnt * sizeof(huffCode));
	*ret = (huffTree){ (huffNode*)(ret + 1), cnt };

	for (node = head, q = &freqs[0], i = cnt; i--; ++q) *node++ = (huffNode){ q->v, 1, (float)q->f / n };
	huffAssign(huffSort(head, cnt), ret);
	free(freqs);
	free(head);
	return ret;
}

// -------------
// Serialization
// -------------

#include "bitbuffer.c"

signed int* huffDeserialize(BB* b, int symBits) {
	signed int* ret, * r;
	unsigned int cnts[31], * p, i = 0, j, sum, expected;

	// Header
	BBProcessBits(b, &i, HUFF_HEADER_BITS);
	if (i & 2) return NULL; // Not implemented

	if (i == HUFF_NONE) { // Read just one value
		ret = malloc(sizeof(int) * 2);
		BBProcessBits(b, ret, symBits);
		ret[1] = ret[0];
		return ret;
	}

	// Decode lengths and reconstruct codes
	for (expected = 2, sum = 0, p = &cnts[0]; expected; sum += *p++) {
		*p = 0;
		BBProcessBits(b, p, __bsrd(expected) + 1);
		expected = (expected - *p) * 2;
	}

	// i is index on this level
	// j is index on next level
	ret = malloc(sizeof(int) * 2 * sum - 2);

	for (p = &cnts[0], r = ret, expected = 2; sum; expected *= 2) {
		for (i = *p++, expected -= i, sum -= i; i--; BBProcessBits(b, r++, symBits)) *r = 0;
		for (i = -(j = expected); j && sum; --j) *r++ = i--;
	}

	return ret;
}

void huffSerialize(BB* b, huffTree* t, int symBits) {
	int i, j, k, cnts[31];

	if (t->cnt == 1) {
		k = HUFF_NONE;
		BBProcessBits(b, &k, HUFF_HEADER_BITS);
		i = t->codes[0].value;
		BBProcessBits(b, &i, symBits);
	}
	else {
		k = HUFF_SINGLE;
		BBProcessBits(b, &k, HUFF_HEADER_BITS);
		for (i = 31; i--; cnts[i] = 0);
		for (i = t->cnt; i--; cnts[t->codes[i].bits - 1]++);
		for (i = t->cnt, k = 0; i; i -= cnts[k++]);
		for (i = 0, j = 2; i < k; j = (j - cnts[i++]) * 2) BBProcessBits(b, &cnts[i], __bsrd(j) + 1);
		for (i = 0; i < t->cnt; ++i) BBProcessBits(b, &t->codes[i].value, symBits);
	}
}

#endif
