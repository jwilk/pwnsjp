#include <wchar.h>

#ifndef ENTITY_HASH_H
#define ENTITY_HASH_H

#define hash_coeff_count 10
static const int hash_coeff[10] = { 171, 251, 155, 61, 92, 101, 229, 154, 189, 171 };
#define hash_size 0x88
static const int entity_hash[hash_size] = { 51, -1, -1, -1, 54, -1, 13, 57, -1, -1, 55, 29, 32, -1, 11, -1, -1, -1, -1, -1, 35, -1, 44, -1, -1, -1, 17, 25, 43, -1, 19, 28, -1, -1, 7, -1, 9, -1, -1, -1, -1, 18, 22, 60, 14, -1, 5, 31, 8, 4, -1, 21, -1, 45, 50, -1, -1, 20, -1, 59, -1, -1, -1, -1, -1, 34, 37, -1, 62, 15, -1, -1, -1, -1, -1, 47, 61, 12, 24, 3, -1, 30, -1, 27, -1, -1, -1, 36, -1, 23, 0, 41, 58, 6, -1, -1, -1, 16, -1, 48, -1, 10, -1, -1, 38, 40, -1, 26, -1, -1, -1, 52, -1, 46, -1, -1, -1, -1, 56, 53, 2, -1, -1, -1, -1, 1, -1, 39, 49, -1, 42, -1, 33, -1, -1, -1 };

#endif
