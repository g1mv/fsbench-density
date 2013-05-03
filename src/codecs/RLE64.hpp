#ifndef __RLE_64_HPP_KDSK97F
#define __RLE_64_HPP_KDSK97F

unsigned long long RLE64_Compress64(const void *pacSource, void *pacDest, unsigned long long plSize);
unsigned long long RLE64_Uncompress64(const void *pacSource, void *pacDest, unsigned long long plSize);
unsigned int RLE64_Compress32(const void *paiSource, void *paiDest, unsigned int piSize);
unsigned int RLE64_Uncompress32(const void *paiSource, void *paiDest, unsigned int piSize);
unsigned int RLE64_Compress16(const void *paiSource, void *paiDest, unsigned int piSize);
unsigned int RLE64_Uncompress16(const void *paiSource, void *paiDest, unsigned int piSize);
unsigned int RLE64_Compress8(const void *pacSource, void *pacDest, unsigned int piSize);
unsigned int RLE64_Uncompress8(const void *pacSource, void *pacDest, unsigned int piSize);

#endif //__RLE_64_HPP_KDSK97F