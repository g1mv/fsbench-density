#include "stddef.h"
#include "stdint.h"

uint32_t FNV1A_Hash_Jesteress(const char *str, size_t wrdlen);
uint32_t FNV1A_Hash_Mantis   (const char *str, size_t wrdlen);
uint32_t FNV1A_Hash_Meiyan   (const char *str, size_t wrdlen);
uint64_t FNV1A_Hash_Tesla    (const char *str, size_t wrdlen);
uint64_t FNV1A_Hash_Tesla3   (const char *str, size_t wrdlen);
uint32_t FNV1A_Hash_Yorikke(const char *str, size_t wrdlen);
uint32_t FNV1A_Hash_YoshimitsuTRIAD(const char *str, size_t wrdlen);
uint32_t FNV1A_Hash_Yoshimura(const char *str, size_t wrdlen);
