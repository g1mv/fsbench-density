// Nakamichi is 100% FREE LZSS SUPERFAST decompressor.

// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX, written by Kaze, babealicious suggestion by m^2 enforced.
// Change #1: Nasty bug in Swampshine was fixed.
// Change #2: Sanity check in compression section was added thus avoiding 'index-Min_Match_Length' going below 0.

// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy, written by Kaze, babealicious suggestion by m^2 enforced.
// Change #1: 'memcpy' replaced by GP/XMM/YMM TRIADs.
// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD, written by Kaze.
// Change #1: Decompression fetches WORD instead of BYTE+BYTE.
// Change #2: Decompression stores three times 64bit instead of memcpy() for all transfers <=24 bytes.
// Change #3: Fifteenth bit is used and then unused, 16KB -> 32KB -> 16KB.
// 32KB window disappoints speedwise, also sizewise:
/*
D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik8
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Compressing 100000000 bytes ...
-; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 130 KB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik8.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Decompressing 65693566 bytes ...
RAM-to-RAM performance: 358 MB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik9
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Compressing 1000000000 bytes ...
/; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 150 KB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik9.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Decompressing 609319736 bytes ...
RAM-to-RAM performance: 379 MB/s.
*/
// 1-RSSBO_1GB vs 1-RSSBO_1GB_15bit_Wordfetcher (16KB/32KB respectively):
// 069,443,065 vs 065,693,566
// 641,441,055 vs 609,319,736

// Nakamichi, revision 1-RSSBO_1GB, written by Kaze.
// Based on Nobuo Ito's source, thanks Ito.
// The main goal of Nakamichi is to allow supersimple and superfast decoding for English x-grams (mainly) in pure C, or not, heh-heh.
// Natively Nakamichi is targeted as 64bit tool with 16 threads, helping Kazahana to traverse faster when I/O is not superior.
// In short, Nakamichi is intended as x-gram decompressor.

// Eightfold Path ~ the Buddhist path to nirvana, comprising eight aspects in which an aspirant must become practised; 
// eightfold way ~ (Physics), the grouping of hadrons into supermultiplets by means of SU(3)); (b) adverb to eight times the number or quantity: OE.

// Note1: Fifteenth bit is not used, making the window wider by 1bit i.e. 32KB is not tempting, rather I think to use it as a flag: 8bytes/16bytes.
// Note2: English x-grams are as English texts but more redundant, in other words they are phraselists in most cases, sometimes wordlists.
// Note3: On OSHO.TXT, being a typical English text, Nakamichi's compression ratio is among the worsts:
//        206,908,949 OSHO.TXT
//        125,022,859 OSHO.TXT.Nakamichi
//        It struggles with English texts but decomprression speed is quite sweet (Core 2 T7500 2200MHz, 32bit code):
//        Nakamichi, revision 1-, written by Kaze.
//        Decompressing 125022859 bytes ...
//        RAM-to-RAM performance: 477681 KB/s.      
// Note4: Also I wanted to see how my 'Railgun_Swampshine_BailOut', being a HEAVYGUN i.e. with big overhead and latency, hits in a real-world application.

// Quick notes on PAGODAs (the padded x-gram lists):
// Every single word in English has its own PAGODA, in example below 'on' PAGODA is given (Kazahana_on.PAGODA-order-5.txt):
// PAGODA order 5 (i.e. with 5 tiers) has 5*(5+1)/2=15 subtiers, they are concatenated and space-padded in order to form the pillar 'on':
/*
D:\_KAZE\Nakamichi_r1-RSSBO>dir \_GW\ka*

04/12/2014  05:07 AM                14 Kazahana_on.1-1.txt
04/12/2014  05:07 AM         1,635,389 Kazahana_on.2-1.txt
04/12/2014  05:07 AM         1,906,734 Kazahana_on.2-2.txt
04/12/2014  05:07 AM        10,891,415 Kazahana_on.3-1.txt
04/12/2014  05:07 AM        15,797,703 Kazahana_on.3-2.txt
04/12/2014  05:07 AM        20,419,280 Kazahana_on.3-3.txt
04/12/2014  05:07 AM        22,141,823 Kazahana_on.4-1.txt
04/12/2014  05:07 AM        36,002,113 Kazahana_on.4-2.txt
04/12/2014  05:07 AM        33,236,772 Kazahana_on.4-3.txt
04/12/2014  05:07 AM        33,902,425 Kazahana_on.4-4.txt
04/12/2014  05:07 AM        24,795,989 Kazahana_on.5-1.txt
04/12/2014  05:07 AM        30,766,220 Kazahana_on.5-2.txt
04/12/2014  05:07 AM        38,982,816 Kazahana_on.5-3.txt
04/12/2014  05:07 AM        38,089,575 Kazahana_on.5-4.txt
04/12/2014  05:07 AM        34,309,057 Kazahana_on.5-5.txt
04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.1-1.txt
9,999,999       on

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.2-1.txt
9,999,999       on_the
1,148,054       on_his
0,559,694       on_her
0,487,856       on_this
0,399,485       on_your
0,381,570       on_my
0,367,282       on_their
...

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.2-2.txt
0,545,191       based_on
0,397,408       and_on
0,334,266       go_on
0,329,561       went_on
0,263,035       was_on
0,246,332       it_on
0,229,041       down_on
0,202,151       going_on
...

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.5-5.txt
0,083,564       foundation_osho_s_books_on
0,012,404       medium_it_may_be_on
0,012,354       if_you_received_it_on
0,012,152       medium_they_may_be_on
0,012,144       agree_to_also_provide_on
0,012,139       a_united_states_copyright_on
0,008,067       we_are_constantly_working_on
0,008,067       questions_we_have_received_on
0,006,847       file_was_first_posted_on
0,006,441       of_we_are_already_on
0,006,279       you_received_this_ebook_on
0,005,865       you_received_this_etext_on
0,005,833       to_keep_an_eye_on
...

D:\_KAZE\Nakamichi_r1-RSSBO>dir

04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt

D:\_KAZE\Nakamichi_r1-RSSBO>Nakamichi.exe Kazahana_on.PAGODA-order-5.txt
Nakamichi, revision 1-RSSBO, written by Kaze.
Compressing 846351894 bytes ...
/; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 512 KB/s.

D:\_KAZE\Nakamichi_r1-RSSBO>dir

04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt
04/15/2014  06:30 PM       293,049,398 Kazahana_on.PAGODA-order-5.txt.Nakamichi

D:\_KAZE\Nakamichi_r1-RSSBO>Nakamichi.exe Kazahana_on.PAGODA-order-5.txt.Nakamichi
Nakamichi, revision 1-RSSBO, written by Kaze.
Decompressing 293049398 bytes ...
RAM-to-RAM performance: 607 MB/s.

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 4096
YAPPY: [b 4K] bytes 846351894 -> 191149889  22.6%  comp  33.8 MB/s  uncomp 875.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 8192
YAPPY: [b 8K] bytes 846351894 -> 184153244  21.8%  comp  35.0 MB/s  uncomp 898.3 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 16384
YAPPY: [b 16K] bytes 846351894 -> 180650931  21.3%  comp  28.8 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 32768
YAPPY: [b 32K] bytes 846351894 -> 178902966  21.1%  comp  35.0 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 65536
YAPPY: [b 64K] bytes 846351894 -> 178027899  21.0%  comp  34.5 MB/s  uncomp 914.6 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 131072
YAPPY: [b 128K] bytes 846351894 -> 177591807  21.0%  comp  34.9 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint64_t needed
#include <time.h>
#include <string.h>

//#include <emmintrin.h> // SSE2 intrinsics
//#include <smmintrin.h> // SSE4.1 intrinsics
//#include <immintrin.h> // AVX intrinsics
//#include <zmmintrin.h> // AVX2 intrinsics, definitions and declarations for use with 512-bit compiler intrinsics.

//void SlowCopy128bit (const char *SOURCE, char *TARGET) { _mm_storeu_si128((__m128i *)(TARGET), _mm_loadu_si128((const __m128i *)(SOURCE))); }
/*
 * Move Unaligned Packed Integer Values
 * **** VMOVDQU ymm1, m256
 * **** VMOVDQU m256, ymm1
 * Moves 256 bits of packed integer values from the source operand to the
 * destination
 */
//extern __m256i __ICL_INTRINCC _mm256_loadu_si256(__m256i const *);
//extern void    __ICL_INTRINCC _mm256_storeu_si256(__m256i *, __m256i);
//void SlowCopy256bit (const char *SOURCE, char *TARGET) { _mm256_storeu_si256((__m256i *)(TARGET), _mm256_loadu_si256((const __m256i *)(SOURCE))); }
//extern __m512i __ICL_INTRINCC _mm512_loadu_si512(void const*);
//extern void    __ICL_INTRINCC _mm512_storeu_si512(void*, __m512i);
//void SlowCopy512bit (const char *SOURCE, char *TARGET) { _mm512_storeu_si512((__m512i *)(TARGET), _mm512_loadu_si512((const __m512i *)(SOURCE))); }

// During compilation use one of these, the granularity of the padded 'memcpy', 4x2x8/2x2x16/1x2x32/1x1x64 respectively as GP/XMM/YMM/ZMM, the maximum literal length reduced from 127 to 63:
//#define _N_GP
//#define _N_XMM
//#define _N_YMM
//#define _N_ZMM

//icl /O3 Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.c -D_N_GP /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif_GP.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif_GP.exe
//icl /O3 /QxSSE2 Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.c -D_N_XMM /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif_XMM.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif_XMM.exe
//icl /O3 /QxAVX Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.c -D_N_YMM /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif_YMM.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif_YMM.exe
//icl /O3 /QxCORE-AVX2 Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.c -D_N_ZMM /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif_ZMM.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_NOif_ZMM.exe

#ifndef NULL
#define NULL ((void*)0)
#endif

// Comment it to see how slower 'BruteForce' is, for Wikipedia 100MB the ratio is 41KB/s versus 197KB/s.
#define ReplaceBruteForceWithRailgunSwampshineBailOut

static void SearchIntoSlidingWindow(unsigned int* retIndex, unsigned int* retMatch, char* refStart,char* refEnd,char* encStart,char* encEnd);
static unsigned int SlidingWindowVsLookAheadBuffer(char* refStart, char* refEnd, char* encStart, char* encEnd);
unsigned int Compress(char* ret, char* src, unsigned int srcSize);
unsigned int Decompress(char* ret, char* src, unsigned int srcSize);
static char * Railgun_Swampshine_BailOut(char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern);
static char * Railgun_Doublet (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern);

// Min_Match_Length=THRESHOLD=4 means 4 and bigger are to be encoded:
#define Min_Match_BAILOUT_Length (8)
#define Min_Match_Length (8)
#define OffsetBITS (14)
#define LengthBITS (1)

//12bit
//#define REF_SIZE (4095+Min_Match_Length)
#define REF_SIZE ( ((1<<OffsetBITS)-1) + Min_Match_Length )
//3bit
//#define ENC_SIZE (7+Min_Match_Length)
#define ENC_SIZE ( ((1<<LengthBITS)-1) + Min_Match_Length )
/*
int main( int argc, char *argv[] ) {
	FILE *fp;
	int SourceSize;
	int TargetSize;
	char* SourceBlock=NULL;
	char* TargetBlock=NULL;
	char* Nakamichi = ".Nakamichi\0";
	char NewFileName[256];
	clock_t clocks1, clocks2;

	printf("Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX, written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.\n");
	if (argc==1) {
		printf("Usage: Nakamichi filename\n"); exit(13);
	}
	if ((fp = fopen(argv[1], "rb")) == NULL) {
		printf("Nakamichi: Can't open '%s' file.\n", argv[1]); exit(13);
	}
	fseek(fp, 0, SEEK_END);
	SourceSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	// If filename ends in '.Nakamichi' then mode is decompression otherwise compression.
	if (strcmp(argv[1]+(strlen(argv[1])-strlen(Nakamichi)), Nakamichi) == 0) {
	SourceBlock = (char*)malloc(SourceSize+512);
	//TargetBlock = (char*)malloc(5*SourceSize+512);
	TargetBlock = (char*)malloc(1024*1024*1024+512);
	fread(SourceBlock, 1, SourceSize, fp);
	fclose(fp);
		printf("Decompressing %d bytes ...\n", SourceSize );
		clocks1 = clock();
		TargetSize = Decompress(TargetBlock, SourceBlock, SourceSize);
		clocks2 = clock();
		printf("RAM-to-RAM performance: %d MB/s.\n", ((TargetSize/(clocks2 - clocks1 + 1))*(long)1000)>>20);
		strcpy(NewFileName, argv[1]);
		*( NewFileName + strlen(argv[1])-strlen(Nakamichi) ) = '\0';
	} else {
	SourceBlock = (char*)malloc(SourceSize+512);
	TargetBlock = (char*)malloc(SourceSize+512);
	fread(SourceBlock, 1, SourceSize, fp);
	fclose(fp);
		printf("Compressing %d bytes ...\n", SourceSize );
		clocks1 = clock();
		TargetSize = Compress(TargetBlock, SourceBlock, SourceSize);
		clocks2 = clock();
		printf("RAM-to-RAM performance: %d KB/s.\n", ((SourceSize/(clocks2 - clocks1 + 1))*(long)1000)>>10);
		strcpy(NewFileName, argv[1]);
		strcat(NewFileName, Nakamichi);
	}
	if ((fp = fopen(NewFileName, "wb")) == NULL) {
		printf("Nakamichi: Can't write '%s' file.\n", NewFileName); exit(13);
	}
	fwrite(TargetBlock, 1, TargetSize, fp);
	fclose(fp);
	free(TargetBlock);
	free(SourceBlock);
	exit(0);
}*/

void SearchIntoSlidingWindow(unsigned int* retIndex, unsigned int* retMatch, char* refStart,char* refEnd,char* encStart,char* encEnd){
	char* FoundAtPosition;
	unsigned int match=0;
	*retIndex=0;
	*retMatch=0;
#ifdef ReplaceBruteForceWithRailgunSwampshineBailOut
	if (refStart < refEnd) {
		FoundAtPosition = Railgun_Swampshine_BailOut(refStart, encStart, (uint32_t)(refEnd-refStart), 8);
		//FoundAtPosition = Railgun_Doublet(refStart, encStart, (uint32_t)(refEnd-refStart), 8);
		// For bigger windows 'Doublet' is slower:
		// Nakamichi, revision 1-RSSBO_1GB_15bit performance with 'Swampshine':
		// Compressing 846351894 bytes ...
		// RAM-to-RAM performance: 370 KB/s.
		// Nakamichi, revision 1-RSSBO_1GB_15bit performance with 'Doublet':
		// Compressing 846351894 bytes ...
		// RAM-to-RAM performance: 213 KB/s.
		if (FoundAtPosition!=NULL) {
			// Stupid sanity check, in next revision I will discard 'Min_Match_Length' additions/subtractions altogether:
			if ( refEnd-FoundAtPosition >= Min_Match_Length ) {
				*retMatch=8;
				*retIndex=refEnd-FoundAtPosition;
			}
		}
	}
#else				
	while(refStart < refEnd){
		match=SlidingWindowVsLookAheadBuffer(refStart,refEnd,encStart,encEnd);
		if(match > *retMatch){
			*retMatch=match;
			*retIndex=refEnd-refStart;
		}
		if(*retMatch >= Min_Match_BAILOUT_Length) break;
		refStart++;
	}
#endif
}

unsigned int SlidingWindowVsLookAheadBuffer( char* refStart, char* refEnd, char* encStart,char* encEnd){
	int ret = 0;
	while(refStart[ret] == encStart[ret]){
		if(&refStart[ret] >= refEnd) break;
		if(&encStart[ret] >= encEnd) break;
		ret++;
		if(ret >= Min_Match_BAILOUT_Length) break;
	}
	return ret;
}

unsigned int CompressNoMemcpy(char* ret, char* src, unsigned int srcSize){
	unsigned int srcIndex=0;
	unsigned int retIndex=0;
	unsigned int index=0;
	unsigned int match=0;
	unsigned int notMatch=0;
	unsigned char* notMatchStart=NULL;
	char* refStart=NULL;
	char* encEnd=NULL;
	//int Melnitchka=0;
	//char *Auberge[4] = {"|\0","/\0","-\0","\\\0"};
	//int ProgressIndicator;

	while(srcIndex < srcSize){
		if(srcIndex>=REF_SIZE)
			refStart=&src[srcIndex-REF_SIZE];
		else
			refStart=src;
		if(srcIndex>=srcSize-ENC_SIZE)
			encEnd=&src[srcSize];
		else
			encEnd=&src[srcIndex+ENC_SIZE];

		SearchIntoSlidingWindow(&index,&match,refStart,&src[srcIndex],&src[srcIndex],encEnd);
		//if ( match<Min_Match_Length ) {
		//if ( match<Min_Match_Length || match<8 ) {
  		if ( match==0 ) {
			if(notMatch==0){
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			else if (notMatch==127) {
				*notMatchStart=(unsigned char)((127)<<1);
				notMatch=0;
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			ret[retIndex]=src[srcIndex];
			retIndex++;
			notMatch++;
			srcIndex++;
			/*if ((srcIndex-1) % (1<<17) > srcIndex % (1<<17)) {
				ProgressIndicator = (int)( (srcIndex+1)*(float)100/(srcSize+1) );
				printf("%s; Each rotation means 128KB are encoded; Done %d%%\r", Auberge[Melnitchka++], ProgressIndicator );
				Melnitchka = Melnitchka & 3; // 0 1 2 3: 00 01 10 11
			}*/
		} else {
			if(notMatch > 0){
				*notMatchStart=(unsigned char)((notMatch)<<1);
				notMatch=0;
			}
// ---------------------| 
//                     \ /

			//ret[retIndex] = 0x80; // Assuming seventh/fifteenth bit is zero i.e. LONG MATCH i.e. Min_Match_BAILOUT_Length*4
	  		//if ( match==Min_Match_BAILOUT_Length ) ret[retIndex] = 0xC0; // 8bit&7bit set, SHORT MATCH if seventh/fifteenth bit is not zero i.e. Min_Match_BAILOUT_Length
//                     / \
// ---------------------|
			ret[retIndex] = 0x01; // Assuming seventh/fifteenth bit is zero i.e. LONG MATCH i.e. Min_Match_BAILOUT_Length*4
	  		//if ( match==Min_Match_BAILOUT_Length ) ret[retIndex] = 0x03; // 2bit&1bit set, SHORT MATCH if 2bit is not zero i.e. Min_Match_BAILOUT_Length
			// 1bit+3bits+12bits:
			//ret[retIndex] = ret[retIndex] | ((match-Min_Match_Length)<<4);
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x0F00)>>8);
			// 1bit+1bit+14bits:
			//ret[retIndex] = ret[retIndex] | ((match-Min_Match_Length)<<(8-(LengthBITS+1))); // No need to set the matchlength
// The fragment below is outrageously ineffective - instead of 8bit&7bit I have to use the lower TWO bits i.e. 2bit&1bit as flags, thus in decompressing one WORD can be fetched instead of two BYTE loads followed by SHR by 2.
// ---------------------| 
//                     \ /
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x3F00)>>8); // 2+4+8=14
			//retIndex++;
			//ret[retIndex] = (char)((index-Min_Match_Length) & 0x00FF);
			//retIndex++;
//                     / \
// ---------------------|
			// Now the situation is like LOW:HIGH i.e. FF:3F i.e. 0x3FFF, 16bit&15bit used as flags,
			// should become LOW:HIGH i.e. FC:FF i.e. 0xFFFC, 2bit&1bit used as flags.
			ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x00FF)<<2); // 6+8=14
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x00FF)<<1); // 7+8=15
			retIndex++;
			ret[retIndex] = (char)(((index-Min_Match_Length) & 0x3FFF)>>6);
			//ret[retIndex] = (char)(((index-Min_Match_Length) & 0x7FFF)>>7);
			retIndex++;
//                     / \
// ---------------------|
			srcIndex+=match;
			/*if ((srcIndex-match) % (1<<17) > srcIndex % (1<<17)) {
				ProgressIndicator = (int)( (srcIndex+1)*(float)100/(srcSize+1) );
				printf("%s; Each rotation means 128KB are encoded; Done %d%%\r", Auberge[Melnitchka++], ProgressIndicator );
				Melnitchka = Melnitchka & 3; // 0 1 2 3: 00 01 10 11
			}*/
		}
	}
	if(notMatch > 0){
		*notMatchStart=(unsigned char)((notMatch)<<1);
	}
	//printf("%s; Each rotation means 128KB are encoded; Done %d%%\n", Auberge[Melnitchka], 100 );
	return retIndex;
}

unsigned int DecompressNoMemcpy(char* ret, char* src, unsigned int srcSize){
	unsigned int srcIndex=0;
	unsigned int retIndex=0;
	unsigned int index=0;
	unsigned int match=0;
	unsigned int WORDpair;
	signed int mm;
	unsigned int Fantagiro;

	while(srcIndex < srcSize){
		//if((unsigned char)src[srcIndex] <= 127){
		WORDpair = *(unsigned short int*)&src[srcIndex];
		if((WORDpair & 0x01) == 0){

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 735 MB/s:
/*
				*(uint64_t*)(ret+retIndex+8*0) = *(uint64_t*)(src+srcIndex+1+8*0);
				SlowCopy128bit((src+srcIndex+1+8*1), (ret+retIndex+8*1));
			if ( ((WORDpair & 0xFF)>>1) > 8+16 ) {
				memcpy(&ret[retIndex]+8+16,&src[srcIndex+1]+8+16,((WORDpair & 0xFF)>>1)-(8+16)); // Use padding and replace 'memcpy' with loop of 4 or 4+4 transfers/stores i.e. *()=DWORD
			}
*/
			// For enwik9 (as 64bit code) RAM-to-RAM performance: 782 MB/s:
/*
				*(uint64_t*)(ret+retIndex+8*0) = *(uint64_t*)(src+srcIndex+1+8*0);
				*(uint64_t*)(ret+retIndex+8*1) = *(uint64_t*)(src+srcIndex+1+8*1);
				*(uint64_t*)(ret+retIndex+8*2) = *(uint64_t*)(src+srcIndex+1+8*2);
			if ( ((WORDpair & 0xFF)>>1) > 8*3 ) {
				memcpy(&ret[retIndex]+8*3,&src[srcIndex+1]+8*3,((WORDpair & 0xFF)>>1)-8*3); // Use padding and replace 'memcpy' with loop of 4 or 4+4 transfers/stores i.e. *()=DWORD
			}
*/
			// For enwik9 (as 64bit code) RAM-to-RAM performance: 763 MB/s:
/*
			if ( ((WORDpair & 0xFF)>>1) > 8*4 ) {
				memcpy(&ret[retIndex],&src[srcIndex+1],(WORDpair & 0xFF)>>1); // Use padding and replace 'memcpy' with loop of 4 or 4+4 transfers/stores i.e. *()=DWORD
			} else {
				//SlowCopy128bit((src+srcIndex+1+16*0), (ret+retIndex+16*0));
				//SlowCopy128bit((src+srcIndex+1+16*1), (ret+retIndex+16*1));
				*(uint64_t*)(ret+retIndex+8*0) = *(uint64_t*)(src+srcIndex+1+8*0);
				*(uint64_t*)(ret+retIndex+8*1) = *(uint64_t*)(src+srcIndex+1+8*1);
				*(uint64_t*)(ret+retIndex+8*2) = *(uint64_t*)(src+srcIndex+1+8*2);
				*(uint64_t*)(ret+retIndex+8*3) = *(uint64_t*)(src+srcIndex+1+8*3);
				// Funny, Nikola Tesla is always right, triads rule, for 'Kazahana_on.PAGODA-order-5.txt.Nakamichi':
				// For 8*3 GP  : RAM-to-RAM performance: 876 MB/s.
				// For 8*4 GP  : RAM-to-RAM performance: 833 MB/s.
				// For 16*2 XMM: RAM-to-RAM performance: 820 MB/s.
			}
*/
			// For enwik9 (as 64bit code) RAM-to-RAM performance: 793 MB/s:
/*
			if ( ((WORDpair & 0xFF)>>1) > 8*3 ) {
				memcpy(&ret[retIndex],&src[srcIndex+1],(WORDpair & 0xFF)>>1); // Use padding and replace 'memcpy' with loop of 4 or 4+4 transfers/stores i.e. *()=DWORD
			} else {
				//SlowCopy128bit((src+srcIndex+1+16*0), (ret+retIndex+16*0));
				//SlowCopy128bit((src+srcIndex+1+16*1), (ret+retIndex+16*1));
				*(uint64_t*)(ret+retIndex+8*0) = *(uint64_t*)(src+srcIndex+1+8*0);
				*(uint64_t*)(ret+retIndex+8*1) = *(uint64_t*)(src+srcIndex+1+8*1);
				*(uint64_t*)(ret+retIndex+8*2) = *(uint64_t*)(src+srcIndex+1+8*2);
				//*(uint64_t*)(ret+retIndex+8*3) = *(uint64_t*)(src+srcIndex+1+8*3);
				// Funny, Nikola Tesla is always right, triads rule, for 'Kazahana_on.PAGODA-order-5.txt.Nakamichi':
				// For 8*3 GP  : RAM-to-RAM performance: 876 MB/s.
				// For 8*4 GP  : RAM-to-RAM performance: 833 MB/s.
				// For 16*2 XMM: RAM-to-RAM performance: 820 MB/s.
			}
*/
			// For enwik9 (as 64bit code) RAM-to-RAM performance: 727 MB/s:
/*
			if ( ((WORDpair & 0xFF)>>1) > 8*2 ) {
				memcpy(&ret[retIndex],&src[srcIndex+1],(WORDpair & 0xFF)>>1); // Use padding and replace 'memcpy' with loop of 4 or 4+4 transfers/stores i.e. *()=DWORD
			} else {
				//SlowCopy128bit((src+srcIndex+1+16*0), (ret+retIndex+16*0));
				//SlowCopy128bit((src+srcIndex+1+16*1), (ret+retIndex+16*1));
				*(uint64_t*)(ret+retIndex+8*0) = *(uint64_t*)(src+srcIndex+1+8*0);
				*(uint64_t*)(ret+retIndex+8*1) = *(uint64_t*)(src+srcIndex+1+8*1);
				// Funny, Nikola Tesla is always right, triads rule, for 'Kazahana_on.PAGODA-order-5.txt.Nakamichi':
				// For 8*3 GP  : RAM-to-RAM performance: 876 MB/s.
				// For 8*4 GP  : RAM-to-RAM performance: 833 MB/s.
				// For 16*2 XMM: RAM-to-RAM performance: 820 MB/s.
			}
*/
			// For enwik9 (as 64bit code) RAM-to-RAM performance: 629 MB/s:
/*
			if ( ((WORDpair & 0xFF)>>1) > 8*1 ) {
				memcpy(&ret[retIndex],&src[srcIndex+1],(WORDpair & 0xFF)>>1); // Use padding and replace 'memcpy' with loop of 4 or 4+4 transfers/stores i.e. *()=DWORD
			} else {
				//SlowCopy128bit((src+srcIndex+1+16*0), (ret+retIndex+16*0));
				//SlowCopy128bit((src+srcIndex+1+16*1), (ret+retIndex+16*1));
				*(uint64_t*)(ret+retIndex+8*0) = *(uint64_t*)(src+srcIndex+1+8*0);
				// Funny, Nikola Tesla is always right, triads rule, for 'Kazahana_on.PAGODA-order-5.txt.Nakamichi':
				// For 8*3 GP  : RAM-to-RAM performance: 876 MB/s.
				// For 8*4 GP  : RAM-to-RAM performance: 833 MB/s.
				// For 16*2 XMM: RAM-to-RAM performance: 820 MB/s.
			}
*/

			// The approach below was proposed by m^2, many-many thanks:

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 686 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(8*1);
//			gogogo:
//				*(uint64_t*)(ret+retIndex+8*(0+1*mm)) = *(uint64_t*)(src+srcIndex+1+8*(0+1*mm));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 754 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(8*2);
//			gogogo:
//				*(uint64_t*)(ret+retIndex+8*(0+2*mm)) = *(uint64_t*)(src+srcIndex+1+8*(0+2*mm));
//				*(uint64_t*)(ret+retIndex+8*(1+2*mm)) = *(uint64_t*)(src+srcIndex+1+8*(1+2*mm));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 753 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(8*3);
//			gogogo:
//				*(uint64_t*)(ret+retIndex+8*(0+3*mm)) = *(uint64_t*)(src+srcIndex+1+8*(0+3*mm));
//				*(uint64_t*)(ret+retIndex+8*(1+3*mm)) = *(uint64_t*)(src+srcIndex+1+8*(1+3*mm));
//				*(uint64_t*)(ret+retIndex+8*(2+3*mm)) = *(uint64_t*)(src+srcIndex+1+8*(2+3*mm));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 763 MB/s:
//			for (mm=0; mm <= ((WORDpair & 0xFF)>>1)/(8*3); mm++) {
//				*(uint64_t*)(ret+retIndex+8*(0+3*mm)) = *(uint64_t*)(src+srcIndex+1+8*(0+3*mm));
//				*(uint64_t*)(ret+retIndex+8*(1+3*mm)) = *(uint64_t*)(src+srcIndex+1+8*(1+3*mm));
//				*(uint64_t*)(ret+retIndex+8*(2+3*mm)) = *(uint64_t*)(src+srcIndex+1+8*(2+3*mm));
//			}

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 773 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(8*4);
//			gogogo:
//				*(uint64_t*)(ret+retIndex+8*(0+4*mm)) = *(uint64_t*)(src+srcIndex+1+8*(0+4*mm));
//				*(uint64_t*)(ret+retIndex+8*(1+4*mm)) = *(uint64_t*)(src+srcIndex+1+8*(1+4*mm));
//				*(uint64_t*)(ret+retIndex+8*(2+4*mm)) = *(uint64_t*)(src+srcIndex+1+8*(2+4*mm));
//				*(uint64_t*)(ret+retIndex+8*(3+4*mm)) = *(uint64_t*)(src+srcIndex+1+8*(3+4*mm));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 727 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(8*5);
//			gogogo:
//				*(uint64_t*)(ret+retIndex+8*(0+5*mm)) = *(uint64_t*)(src+srcIndex+1+8*(0+5*mm));
//				*(uint64_t*)(ret+retIndex+8*(1+5*mm)) = *(uint64_t*)(src+srcIndex+1+8*(1+5*mm));
//				*(uint64_t*)(ret+retIndex+8*(2+5*mm)) = *(uint64_t*)(src+srcIndex+1+8*(2+5*mm));
//				*(uint64_t*)(ret+retIndex+8*(3+5*mm)) = *(uint64_t*)(src+srcIndex+1+8*(3+5*mm));
//				*(uint64_t*)(ret+retIndex+8*(4+5*mm)) = *(uint64_t*)(src+srcIndex+1+8*(4+5*mm));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 664 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(8*8);
//			gogogo:
//				*(uint64_t*)(ret+retIndex+8*(0+8*mm)) = *(uint64_t*)(src+srcIndex+1+8*(0+8*mm));
//				*(uint64_t*)(ret+retIndex+8*(1+8*mm)) = *(uint64_t*)(src+srcIndex+1+8*(1+8*mm));
//				*(uint64_t*)(ret+retIndex+8*(2+8*mm)) = *(uint64_t*)(src+srcIndex+1+8*(2+8*mm));
//				*(uint64_t*)(ret+retIndex+8*(3+8*mm)) = *(uint64_t*)(src+srcIndex+1+8*(3+8*mm));
//				*(uint64_t*)(ret+retIndex+8*(4+8*mm)) = *(uint64_t*)(src+srcIndex+1+8*(4+8*mm));
//				*(uint64_t*)(ret+retIndex+8*(5+8*mm)) = *(uint64_t*)(src+srcIndex+1+8*(5+8*mm));
//				*(uint64_t*)(ret+retIndex+8*(6+8*mm)) = *(uint64_t*)(src+srcIndex+1+8*(6+8*mm));
//				*(uint64_t*)(ret+retIndex+8*(7+8*mm)) = *(uint64_t*)(src+srcIndex+1+8*(7+8*mm));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 702 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(16*1);
//			gogogo:
//				SlowCopy128bit((src+srcIndex+1+16*(0+1*mm)), (ret+retIndex+16*(0+1*mm)));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 710 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(16*2);
//			gogogo:
//				SlowCopy128bit((src+srcIndex+1+16*(0+2*mm)), (ret+retIndex+16*(0+2*mm)));
//				SlowCopy128bit((src+srcIndex+1+16*(1+2*mm)), (ret+retIndex+16*(1+2*mm)));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 593 MB/s:
//			mm = ((WORDpair & 0xFF)>>1)/(16*4);
//			gogogo:
//				SlowCopy128bit((src+srcIndex+1+16*(0+4*mm)), (ret+retIndex+16*(0+4*mm)));
//				SlowCopy128bit((src+srcIndex+1+16*(1+4*mm)), (ret+retIndex+16*(1+4*mm)));
//				SlowCopy128bit((src+srcIndex+1+16*(2+4*mm)), (ret+retIndex+16*(2+4*mm)));
//				SlowCopy128bit((src+srcIndex+1+16*(3+4*mm)), (ret+retIndex+16*(3+4*mm)));
//			if (mm--) goto gogogo;

			// For enwik9 (as 64bit code) RAM-to-RAM performance: 793 MB/s:
			mm = ((WORDpair & 0xFF)>>1);
			Fantagiro = 0;
			gogogo:
				*(uint64_t*)(ret+retIndex+8*(0+Fantagiro)) = *(uint64_t*)(src+srcIndex+1+8*(0+Fantagiro));
				*(uint64_t*)(ret+retIndex+8*(1+Fantagiro)) = *(uint64_t*)(src+srcIndex+1+8*(1+Fantagiro));
				*(uint64_t*)(ret+retIndex+8*(2+Fantagiro)) = *(uint64_t*)(src+srcIndex+1+8*(2+Fantagiro));
				// 3 x GP 793MB/s
				// 2 x GP 773MB/s
				// 1 x GP 718MB/s
				// 3 x XMM 664MB/s
				// 2 x XMM 727MB/s
				// 1 x XMM 727MB/s
				Fantagiro = Fantagiro + 3;
				mm = mm - 24;
			if (mm>0) goto gogogo;

			retIndex+=(WORDpair & 0xFF)>>1;
			srcIndex+=(((WORDpair & 0xFF)>>1)+1);
		}
		else{
			// 1bit+3bits+12bits:
			//match = ((src[srcIndex] & 0x7F) >> 4)+Min_Match_Length;
			//index = (src[srcIndex] & 0x0F) << 8;
			// 1bit+1bit+14bits:
			//match = ((src[srcIndex] & 0x4F) >> 4)+Min_Match_Length; // In fact, not needed when eightfoldness is commenced, match is 8.
// The fragment below is outrageously ineffective - it can be done in one WORD operation instead of two BYTE operations.
// ---------------------| 
//                     \ /
			//index = (src[srcIndex] & 0x3F) << 8;
			//srcIndex++;
			//index = (index | (unsigned int)(0x00FF & src[srcIndex])) + Min_Match_Length;
			//srcIndex++;
//                     / \
// ---------------------|
// ---------------------| 
//                     \ /
			index = (WORDpair>>2) + Min_Match_Length; // 14bit
			//index = (WORDpair>>1) + Min_Match_Length; // 15bit
			srcIndex=srcIndex+2;
//                     / \
// ---------------------|
			//if (src[srcIndex-1-1] & 0x40) { // 4 if seventh/fifteenth bit is not zero
//			if (WORDpair & 0x02) { // 4 if 2/14 bit is not zero
//				match = Min_Match_BAILOUT_Length;
//				//memcpy(&ret[retIndex],&ret[retIndex-index],match);
//				*(uint32_t*)(ret+retIndex) = *(uint32_t*)(ret+retIndex-index);
//			} else {
//				match = Min_Match_BAILOUT_Length*4;
//				//*(uint32_t*)(ret+retIndex) = *(uint32_t*)(ret+retIndex-index);
//				//*(uint32_t*)(ret+retIndex+4) = *(uint32_t*)(ret+retIndex-index+4);
//				//*(uint32_t*)(ret+retIndex+8) = *(uint32_t*)(ret+retIndex-index+8);
//				//*(uint32_t*)(ret+retIndex+12) = *(uint32_t*)(ret+retIndex-index+12);
//				SlowCopy128bit((ret+retIndex-index), (ret+retIndex));
//			}
			match = Min_Match_BAILOUT_Length;
			*(uint64_t*)(ret+retIndex) = *(uint64_t*)(ret+retIndex-index);
			retIndex+=match;
		}
	}
	return retIndex;
}

// Decompression main loop:
/*
; mark_description "Intel(R) C++ Intel(R) 64 Compiler XE for applications running on Intel(R) 64, Version 12.1.1.258 Build 20111";
; mark_description "-O3 -FAcs";

.B7.3::                         
  0002e 48 03 c2         add rax, rdx                           
  00031 44 0f b7 18      movzx r11d, WORD PTR [rax]             
  00035 41 f7 c3 01 00 
        00 00            test r11d, 1                           
  0003c 0f 85 82 00 00 
        00               jne .B7.10 ; Prob 10%                  
.B7.4::                         
  00042 45 0f b6 db      movzx r11d, r11b                       
  00046 45 33 e4         xor r12d, r12d                         
  00049 41 d1 eb         shr r11d, 1                            
  0004c 4a 8d 2c 09      lea rbp, QWORD PTR [rcx+r9]            
  00050 45 89 dd         mov r13d, r11d                         
.B7.5::                         
  00053 41 83 c5 e8      add r13d, -24                          
  00057 46 8d 34 e5 00 
        00 00 00         lea r14d, DWORD PTR [r12*8]            
  0005f 4e 8b 7c 30 01   mov r15, QWORD PTR [1+rax+r14]         
  00064 4d 89 3c 2e      mov QWORD PTR [r14+rbp], r15           
  00068 46 8d 3c e5 08 
        00 00 00         lea r15d, DWORD PTR [8+r12*8]          
  00070 4d 8b 74 07 01   mov r14, QWORD PTR [1+r15+rax]         
  00075 4d 89 34 2f      mov QWORD PTR [r15+rbp], r14           
  00079 46 8d 34 e5 10 
        00 00 00         lea r14d, DWORD PTR [16+r12*8]         
  00081 41 83 c4 03      add r12d, 3                            
  00085 45 85 ed         test r13d, r13d                        
  00088 4d 8b 7c 06 01   mov r15, QWORD PTR [1+r14+rax]         
  0008d 4d 89 3c 2e      mov QWORD PTR [r14+rbp], r15           
  00091 7f c0            jg .B7.5 ; Prob 82%                    
.B7.6::                         
  00093 43 8d 44 1a 01   lea eax, DWORD PTR [1+r10+r11]         
  00098 45 03 cb         add r9d, r11d                          
.B7.7::                         
  0009b 41 89 c2         mov r10d, eax                          
  0009e 45 3b d0         cmp r10d, r8d                          
  000a1 72 8b            jb .B7.3 ; Prob 82%                    
.B7.8::                         
  000a3 4c 8b 64 24 20   mov r12, QWORD PTR [32+rsp]            
  000a8 4c 8b 6c 24 28   mov r13, QWORD PTR [40+rsp]            
  000ad 4c 8b 74 24 30   mov r14, QWORD PTR [48+rsp]            
  000b2 4c 8b 7c 24 38   mov r15, QWORD PTR [56+rsp]            
  000b7 48 8b 6c 24 40   mov rbp, QWORD PTR [64+rsp]            
.B7.9::                         
  000bc 44 89 c8         mov eax, r9d                           
  000bf 48 83 c4 48      add rsp, 72                            
  000c3 c3               ret                                    
.B7.10::                        
  000c4 41 c1 eb 02      shr r11d, 2                            
  000c8 41 83 c2 02      add r10d, 2                            
  000cc 41 83 c3 08      add r11d, 8                            
  000d0 49 f7 db         neg r11                                
  000d3 4c 03 d9         add r11, rcx                           
  000d6 44 89 d0         mov eax, r10d                          
  000d9 4b 8b 2c 19      mov rbp, QWORD PTR [r9+r11]            
  000dd 49 89 2c 09      mov QWORD PTR [r9+rcx], rbp            
  000e1 41 83 c1 08      add r9d, 8                             
  000e5 eb b4            jmp .B7.7 ; Prob 100%                  
*/

// Yappy vs Nakamichi 'Speed Showdown' on Core2 T7500 2200MHz with enwik9:
/*
D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_vs_Nakamichi_SpeedShowdown_on_enwik9.bat
Yappy vs Nakamichi 'Speed Showdown' on enwik9 ...

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_64bit.exe enwik9 512
YAPPY: [b 0K] bytes 1000000000 -> 779421758  77.9%  comp  40.9 MB/s  uncomp 702.8 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_64bit.exe enwik9 1024
YAPPY: [b 1K] bytes 1000000000 -> 714236729  71.4%  comp  41.8 MB/s  uncomp 657.3 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_64bit.exe enwik9 2048
YAPPY: [b 2K] bytes 1000000000 -> 655356839  65.5%  comp  40.1 MB/s  uncomp 611.3 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_64bit.exe enwik9 4096
YAPPY: [b 4K] bytes 1000000000 -> 593819184  59.4%  comp  36.7 MB/s  uncomp 566.0 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_64bit.exe enwik9 8192
YAPPY: [b 8K] bytes 1000000000 -> 544342520  54.4%  comp  33.6 MB/s  uncomp 545.9 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_64bit.exe enwik9 16384
YAPPY: [b 16K] bytes 1000000000 -> 519654588  52.0%  comp  32.3 MB/s  uncomp 540.9 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_64bit.exe enwik9 32768
YAPPY: [b 32K] bytes 1000000000 -> 507264601  50.7%  comp  32.1 MB/s  uncomp 541.2 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_64bit.exe enwik9 65536
YAPPY: [b 64K] bytes 1000000000 -> 501106828  50.1%  comp  32.2 MB/s  uncomp 540.9 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_32bit.exe enwik9 512
YAPPY: [b 0K] bytes 1000000000 -> 779421758  77.9%  comp  43.0 MB/s  uncomp 710.6 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_32bit.exe enwik9 1024
YAPPY: [b 1K] bytes 1000000000 -> 714236729  71.4%  comp  42.2 MB/s  uncomp 657.7 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_32bit.exe enwik9 2048
YAPPY: [b 2K] bytes 1000000000 -> 655356839  65.5%  comp  39.3 MB/s  uncomp 611.3 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_32bit.exe enwik9 4096
YAPPY: [b 4K] bytes 1000000000 -> 593819184  59.4%  comp  36.3 MB/s  uncomp 571.4 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_32bit.exe enwik9 8192
YAPPY: [b 8K] bytes 1000000000 -> 544342520  54.4%  comp  33.5 MB/s  uncomp 550.9 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_32bit.exe enwik9 16384
YAPPY: [b 16K] bytes 1000000000 -> 519654588  52.0%  comp  32.2 MB/s  uncomp 550.6 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_32bit.exe enwik9 32768
YAPPY: [b 32K] bytes 1000000000 -> 507264601  50.7%  comp  32.0 MB/s  uncomp 545.9 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Yappy_32bit.exe enwik9 65536
YAPPY: [b 64K] bytes 1000000000 -> 501106828  50.1%  comp  32.2 MB/s  uncomp 545.9 MB/s

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_64bit.exe enwik9.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD, written by Kaze, based on Nobuo Ito's LZSS source.
Decompressing 641441055 bytes ...
RAM-to-RAM performance: 772 MB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_32bit.exe enwik9.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD, written by Kaze, based on Nobuo Ito's LZSS source.
Decompressing 641441055 bytes ...
RAM-to-RAM performance: 678 MB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_64bit.exe enwik9.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy, written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Decompressing 641441055 bytes ...
RAM-to-RAM performance: 783 MB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_32bit.exe enwik9.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy, written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Decompressing 641441055 bytes ...
RAM-to-RAM performance: 678 MB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy>
*/

// In my opinion Hamid Buzidi is the best, therefore his lzturbo v1.1 reference results are given below:
/*
D:\_KAZE\Nakamichi_r1-RSSBO>timer32 lzturbo.exe -19 -p0 enwiki-20140304-pages-articles.7z.001 .

Kernel  Time =     0.982 =    0%
User    Time =   152.537 =   99%
Process Time =   153.520 =  100%    Virtual  Memory =    429 MB
Global  Time =   153.519 =  100%    Physical Memory =    407 MB

D:\_KAZE\Nakamichi_r1-RSSBO>timer32.exe lzturbo.exe -d enwiki-20140304-pages-articles.7z.001.lzt .

Kernel  Time =     0.234 =   62%
User    Time =     0.187 =   50%
Process Time =     0.421 =  112%    Virtual  Memory =     98 MB
Global  Time =     0.374 =  100%    Physical Memory =     70 MB

D:\_KAZE\Nakamichi_r1-RSSBO>dir

04/15/2014  08:05 AM       104,857,600 enwiki-20140304-pages-articles.7z.001
04/15/2014  08:04 AM        41,984,881 enwiki-20140304-pages-articles.7z.001.lzt

D:\_KAZE\Nakamichi_r1-RSSBO>timer32 lzturbo.exe -11 -p0 enwiki-20140304-pages-articles.7z.001 .

Kernel  Time =     0.171 =    9%
User    Time =     1.622 =   90%
Process Time =     1.794 =  100%    Virtual  Memory =     58 MB
Global  Time =     1.794 =  100%    Physical Memory =     39 MB

D:\_KAZE\Nakamichi_r1-RSSBO>timer32.exe lzturbo.exe -d enwiki-20140304-pages-articles.7z.001.lzt .

Kernel  Time =     0.249 =   41%
User    Time =     0.140 =   23%
Process Time =     0.390 =   64%    Virtual  Memory =     98 MB
Global  Time =     0.608 =  100%    Physical Memory =     73 MB

D:\_KAZE\Nakamichi_r1-RSSBO>dir

04/15/2014  08:05 AM       104,857,600 enwiki-20140304-pages-articles.7z.001
04/15/2014  08:05 AM        47,685,453 enwiki-20140304-pages-articles.7z.001.lzt

D:\_KAZE\Nakamichi_r1-RSSBO>
*/


// Railgun_Swampshine_BailOut, copyleft 2014-Apr-27, Kaze.
// 2014-Apr-27: The nasty SIGNED/UNSIGNED bug in 'Swampshines' which I illustrated several months ago in my fuzzy search article now is fixed here too:
/*
The bug is this (the variables 'i' and 'PRIMALposition' are uint32_t):
Next line assumes -19 >= 0 is true:
if ( (i-(PRIMALposition-1)) >= 0) printf ("THE NASTY BUG AGAIN: %d >= 0\n", i-(PRIMALposition-1));
Next line assumes -19 >= 0 is false:
if ( (signed int)(i-(PRIMALposition-1)) >= 0) printf ("THE NASTY BUG AGAIN: %d >= 0\n", i-(PRIMALposition-1));
And the actual fix:
...
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4
// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {
...
*/
// Railgun_Swampshine_BailOut, copyleft 2014-Jan-31, Kaze.
// Caution: For better speed the case 'if (cbPattern==1)' was removed, so Pattern must be longer than 1 char.
#define NeedleThreshold2vs4swampLITE 9+10 // Should be bigger than 9. BMH2 works up to this value (inclusive), if bigger then BMH4 takes over.
char * Railgun_Swampshine_BailOut (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern)
{
	char * pbTargetMax = pbTarget + cbTarget;
	register uint32_t ulHashPattern;
	signed long count;

	unsigned char bm_Horspool_Order2[256*256]; // Bitwise soon...
	uint32_t i, Gulliver;

	uint32_t PRIMALposition, PRIMALpositionCANDIDATE;
	uint32_t PRIMALlength, PRIMALlengthCANDIDATE;
	uint32_t j, FoundAtPosition;

	if (cbPattern > cbTarget) return(NULL);

	if ( cbPattern<4 ) { 
		// SSE2 i.e. 128bit Assembly rules here:
		// ...
        	pbTarget = pbTarget+cbPattern;
		ulHashPattern = ( (*(char *)(pbPattern))<<8 ) + *(pbPattern+(cbPattern-1));
		if ( cbPattern==3 ) {
			for ( ;; ) {
				if ( ulHashPattern == ( (*(char *)(pbTarget-3))<<8 ) + *(pbTarget-1) ) {
					if ( *(char *)(pbPattern+1) == *(char *)(pbTarget-2) ) return((pbTarget-3));
				}
				if ( (char)(ulHashPattern>>8) != *(pbTarget-2) ) { 
					pbTarget++;
					if ( (char)(ulHashPattern>>8) != *(pbTarget-2) ) pbTarget++;
				}
				pbTarget++;
				if (pbTarget > pbTargetMax) return(NULL);
			}
		} else {
		}
		for ( ;; ) {
			if ( ulHashPattern == ( (*(char *)(pbTarget-2))<<8 ) + *(pbTarget-1) ) return((pbTarget-2));
			if ( (char)(ulHashPattern>>8) != *(pbTarget-1) ) pbTarget++;
			pbTarget++;
			if (pbTarget > pbTargetMax) return(NULL);
		}
	} else { //if ( cbPattern<4 )
		if ( cbPattern<=NeedleThreshold2vs4swampLITE ) { 
			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
							if ( count <= 0 ) return(pbTarget+i);
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
		} else { // if ( cbPattern<=NeedleThreshold2vs4swampLITE )

// Swampwalker_BAILOUT heuristic order 4 (Needle should be bigger than 4) [
// Needle: 1234567890qwertyuiopasdfghjklzxcv            PRIMALposition=01 PRIMALlength=33  '1234567890qwertyuiopasdfghjklzxcv'
// Needle: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv            PRIMALposition=29 PRIMALlength=04  'vvvv'
// Needle: vvvvvvvvvvBOOMSHAKALAKAvvvvvvvvvv            PRIMALposition=08 PRIMALlength=20  'vvvBOOMSHAKALAKAvvvv'
// Needle: Trollland                                    PRIMALposition=01 PRIMALlength=09  'Trollland'
// Needle: Swampwalker                                  PRIMALposition=01 PRIMALlength=11  'Swampwalker'
// Needle: licenselessness                              PRIMALposition=01 PRIMALlength=15  'licenselessness'
// Needle: alfalfa                                      PRIMALposition=02 PRIMALlength=06  'lfalfa'
// Needle: Sandokan                                     PRIMALposition=01 PRIMALlength=08  'Sandokan'
// Needle: shazamish                                    PRIMALposition=01 PRIMALlength=09  'shazamish'
// Needle: Simplicius Simplicissimus                    PRIMALposition=06 PRIMALlength=20  'icius Simplicissimus'
// Needle: domilliaquadringenquattuorquinquagintillion  PRIMALposition=01 PRIMALlength=32  'domilliaquadringenquattuorquinqu'
// Needle: boom-boom                                    PRIMALposition=02 PRIMALlength=08  'oom-boom'
// Needle: vvvvv                                        PRIMALposition=01 PRIMALlength=04  'vvvv'
// Needle: 12345                                        PRIMALposition=01 PRIMALlength=05  '12345'
// Needle: likey-likey                                  PRIMALposition=03 PRIMALlength=09  'key-likey'
// Needle: BOOOOOM                                      PRIMALposition=03 PRIMALlength=05  'OOOOM'
// Needle: aaaaaBOOOOOM                                 PRIMALposition=02 PRIMALlength=09  'aaaaBOOOO'
// Needle: BOOOOOMaaaaa                                 PRIMALposition=03 PRIMALlength=09  'OOOOMaaaa'
PRIMALlength=0;
for (i=0+(1); i < cbPattern-((4)-1)+(1)-(1); i++) { // -(1) because the last BB order 4 has no counterpart(s)
	FoundAtPosition = cbPattern - ((4)-1) + 1;
	PRIMALpositionCANDIDATE=i;
	while ( PRIMALpositionCANDIDATE <= (FoundAtPosition-1) ) {
		j = PRIMALpositionCANDIDATE + 1;
		while ( j <= (FoundAtPosition-1) ) {
			if ( *(uint32_t *)(pbPattern+PRIMALpositionCANDIDATE-(1)) == *(uint32_t *)(pbPattern+j-(1)) ) FoundAtPosition = j;
			j++;
		}
		PRIMALpositionCANDIDATE++;
	}
	PRIMALlengthCANDIDATE = (FoundAtPosition-1)-i+1 +((4)-1);
	if (PRIMALlengthCANDIDATE >= PRIMALlength) {PRIMALposition=i; PRIMALlength = PRIMALlengthCANDIDATE;}
	if (cbPattern-i+1 <= PRIMALlength) break;
	if (PRIMALlength > 128) break; // Bail Out for 129[+]
}
// Swampwalker_BAILOUT heuristic order 4 (Needle should be bigger than 4) ]

// Here we have 4 or bigger NewNeedle, apply order 2 for pbPattern[i+(PRIMALposition-1)] with length 'PRIMALlength' and compare the pbPattern[i] with length 'cbPattern':
PRIMALlengthCANDIDATE = cbPattern;
cbPattern = PRIMALlength;
pbPattern = pbPattern + (PRIMALposition-1);

// Revision 2 commented section [
/*
if (cbPattern-1 <= 255) {
// BMH Order 2 [
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]= cbPattern-1;} // cbPattern-(Order-1) for Horspool; 'memset' if not optimized
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=i; // Rightmost appearance/position is needed
			i=0;
			while (i <= cbTarget-cbPattern) { 
				Gulliver = bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]];
				if ( Gulliver != cbPattern-1 ) { // CASE #2: if equal means the pair (char order 2) is not found i.e. Gulliver remains intact, skip the whole pattern and fall back (Order-1) chars i.e. one char for Order 2
				if ( Gulliver == cbPattern-2 ) { // CASE #1: means the pair (char order 2) is found
					if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) {
						count = cbPattern-4+1; 
						while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
							count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
					}
					Gulliver = 1;
				} else
					Gulliver = cbPattern - Gulliver - 2; // CASE #3: the pair is found and not as suffix i.e. rightmost position
				}
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
// BMH Order 2 ]
} else {
			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
}
*/
// Revision 2 commented section ]

		if ( cbPattern<=NeedleThreshold2vs4swampLITE ) { 

			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);

		} else { // if ( cbPattern<=NeedleThreshold2vs4swampLITE )

			// BMH pseudo-order 4, needle should be >=8+2:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			// In line below we "hash" 4bytes to 2bytes i.e. 16bit table, how to compute TOTAL number of BBs, 'cbPattern - Order + 1' is the number of BBs for text 'cbPattern' bytes long, for example, for cbPattern=11 'fastest fox' and Order=4 we have BBs = 11-4+1=8:
			//"fast"
			//"aste"
			//"stes"
			//"test"
			//"est "
			//"st f"
			//"t fo"
			//" fox"
			//for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( *(unsigned short *)(pbPattern+i+0) + *(unsigned short *)(pbPattern+i+2) ) & ( (1<<16)-1 )]=1;
			//for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( (*(uint32_t *)(pbPattern+i+0)>>16)+(*(uint32_t *)(pbPattern+i+0)&0xFFFF) ) & ( (1<<16)-1 )]=1;
			// Above line is replaced by next one with better hashing:
			for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( (*(uint32_t *)(pbPattern+i+0)>>(16-1))+(*(uint32_t *)(pbPattern+i+0)&0xFFFF) ) & ( (1<<16)-1 )]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1;
				//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]&0xFFFF) ) & ( (1<<16)-1 )] != 0 ) { // DWORD #1
				// Above line is replaced by next one with better hashing:
				if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]&0xFFFF) ) & ( (1<<16)-1 )] != 0 ) { // DWORD #1
					//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = cbPattern-(2-1)-2-4; else {
					// Above line is replaced in order to strengthen the skip by checking the middle DWORD,if the two DWORDs are 'ab' and 'cd' i.e. [2x][2a][2b][2c][2d] then the middle DWORD is 'bc'.
					// The respective offsets (backwards) are: -10/-8/-6/-4 for 'xa'/'ab'/'bc'/'cd'.
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-4-2; else {
					// Above line is replaced by next one with better hashing:
					// When using (16-1) right shifting instead of 16 we will have two different pairs (if they are equal), the highest bit being lost do the job especialy for ASCII texts with no symbols in range 128-255.
					// Example for genomesque pair TT+TT being shifted by (16-1):
					// T            = 01010100
					// TT           = 01010100 01010100
					// TTTT         = 01010100 01010100 01010100 01010100
					// TTTT>>16     = 00000000 00000000 01010100 01010100
					// TTTT>>(16-1) = 00000000 00000000 10101000 10101000 <--- Due to the left shift by 1, the 8th bits of 1st and 2nd bytes are populated - usually they are 0 for English texts & 'ACGT' data.
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-4-2; else {
					// 'Maximus' uses branched 'if', again.
					if ( \
					( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6 +1]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6 +1]&0xFFFF) ) & ( (1<<16)-1 )] ) == 0 \
					|| ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4 +1]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4 +1]&0xFFFF) ) & ( (1<<16)-1 )] ) == 0 \
					) Gulliver = cbPattern-(2-1)-2-4-2 +1; else {
					// Above line is not optimized (several a SHR are used), we have 5 non-overlapping WORDs, or 3 overlapping WORDs, within 4 overlapping DWORDs so:
// [2x][2a][2b][2c][2d]
// DWORD #4
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>16) =     !SHR to be avoided! <--
// [2x] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) =                        |
//     DWORD #3                                                                       |
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16) =     !SHR to be avoided!   |<--
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) = ------------------------  |
//         DWORD #2                                                                      |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>16) =     !SHR to be avoided!      |<--
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) = ---------------------------  |
//             DWORD #1                                                                     |
// [2d] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]>>16) =                                 |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) = ------------------------------
//
// So in order to remove 3 SHR instructions the equal extractions are:
// DWORD #4
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) =  !SHR to be avoided! <--
// [2x] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) =                        |
//     DWORD #3                                                                       |
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) =  !SHR to be avoided!   |<--
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) = ------------------------  |
//         DWORD #2                                                                      |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) =  !SHR to be avoided!      |<--
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) = ---------------------------  |
//             DWORD #1                                                                     |
// [2d] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]>>16) =                                 |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) = ------------------------------
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-6; else {
// Since the above Decumanus mumbo-jumbo (3 overlapping lookups vs 2 non-overlapping lookups) is not fast enough we go DuoDecumanus or 3x4:
// [2y][2x][2a][2b][2c][2d]
// DWORD #3
//         DWORD #2
//                 DWORD #1
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-8]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-8]&0xFFFF) ) & ( (1<<16)-1 )] ) < 2 ) Gulliver = cbPattern-(2-1)-2-8; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) {
							// Order 4 [
						// Let's try something "outrageous" like comparing with[out] overlap BBs 4bytes long instead of 1 byte back-to-back:
						// Inhere we are using order 4, 'cbPattern - Order + 1' is the number of BBs for text 'cbPattern' bytes long, for example, for cbPattern=11 'fastest fox' and Order=4 we have BBs = 11-4+1=8:
						//0:"fast" if the comparison failed here, 'count' is 1; 'Gulliver' is cbPattern-(4-1)-7
						//1:"aste" if the comparison failed here, 'count' is 2; 'Gulliver' is cbPattern-(4-1)-6
						//2:"stes" if the comparison failed here, 'count' is 3; 'Gulliver' is cbPattern-(4-1)-5
						//3:"test" if the comparison failed here, 'count' is 4; 'Gulliver' is cbPattern-(4-1)-4
						//4:"est " if the comparison failed here, 'count' is 5; 'Gulliver' is cbPattern-(4-1)-3
						//5:"st f" if the comparison failed here, 'count' is 6; 'Gulliver' is cbPattern-(4-1)-2
						//6:"t fo" if the comparison failed here, 'count' is 7; 'Gulliver' is cbPattern-(4-1)-1
						//7:" fox" if the comparison failed here, 'count' is 8; 'Gulliver' is cbPattern-(4-1)
							count = cbPattern-4+1; 
							// Below comparison is UNIdirectional:
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// count = cbPattern-4+1 = 23-4+1 = 20
// boomshakalakaZZZZZZ[ZZZZ] 20
// boomshakalakaZZ[ZZZZ]ZZZZ 20-4
// boomshakala[kaZZ]ZZZZZZZZ 20-8 = 12
// boomsha[kala]kaZZZZZZZZZZ 20-12 = 8
// boo[msha]kalakaZZZZZZZZZZ 20-16 = 4

// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}

							// In order to avoid only-left or only-right WCS the memcmp should be done as left-to-right and right-to-left AT THE SAME TIME.
							// Below comparison is BIdirectional. It pays off when needle is 8+++ long:
//							for (count = cbPattern-4+1; count > 0; count = count-4) {
//								if ( *(uint32_t *)(pbPattern+count-1) != *(uint32_t *)(&pbTarget[i]+(count-1)) ) {break;};
//								if ( *(uint32_t *)(pbPattern+(cbPattern-4+1)-count) != *(uint32_t *)(&pbTarget[i]+(cbPattern-4+1)-count) ) {count = (cbPattern-4+1)-count +(1); break;} // +(1) because two lookups are implemented as one, also no danger of 'count' being 0 because of the fast check outwith the 'while': if ( *(uint32_t *)&pbTarget[i] == ulHashPattern)
//							}
//							if ( count <= 0 ) return(pbTarget+i);
								// Checking the order 2 pairs in mismatched DWORD, all the 3:
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1]] == 0 ) Gulliver = count; // 1 or bigger, as it should
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1]] == 0 ) Gulliver = count+1; // 1 or bigger, as it should
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1+1]] == 0 ) Gulliver = count+1+1; // 1 or bigger, as it should
							//	if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1]] + bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1]] + bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1+1]] < 3 ) Gulliver = count; // 1 or bigger, as it should, THE MIN(count,count+1,count+1+1)
								// Above compound 'if' guarantees not that Gulliver > 1, an example:
								// Needle:    fastest tax
								// Window: ...fastast tax...
								// After matching ' tax' vs ' tax' and 'fast' vs 'fast' the mismathced DWORD is 'test' vs 'tast':
								// 'tast' when factorized down to order 2 yields: 'ta','as','st' - all the three when summed give 1+1+1=3 i.e. Gulliver remains 1.
								// Roughly speaking, this attempt maybe has its place in worst-case scenarios but not in English text and even not in ACGT data, that's why I commented it in original 'Shockeroo'.
								//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+count-1]>>16)+(*(uint32_t *)&pbTarget[i+count-1]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = count; // 1 or bigger, as it should
								// Above line is replaced by next one with better hashing:
//								if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+count-1]>>(16-1))+(*(uint32_t *)&pbTarget[i+count-1]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = count; // 1 or bigger, as it should
							// Order 4 ]
						}
					}
				} else Gulliver = cbPattern-(2-1)-2; // -2 because we check the 4 rightmost bytes not 2.
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);

		} // if ( cbPattern<=NeedleThreshold2vs4swampLITE )
		} // if ( cbPattern<=NeedleThreshold2vs4swampLITE )
	} //if ( cbPattern<4 )
}

// Fixed version from 2012-Feb-27.
// Caution: For better speed the case 'if (cbPattern==1)' was removed, so Pattern must be longer than 1 char.
char * Railgun_Doublet (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern)
{
	char * pbTargetMax = pbTarget + cbTarget;
	register uint32_t ulHashPattern;
	uint32_t ulHashTarget, count, countSTATIC;

	if (cbPattern > cbTarget) return(NULL);

	countSTATIC = cbPattern-2;

	pbTarget = pbTarget+cbPattern;
	ulHashPattern = (*(uint16_t *)(pbPattern));

	for ( ;; ) {
		if ( ulHashPattern == (*(uint16_t *)(pbTarget-cbPattern)) ) {
			count = countSTATIC;
			while ( count && *(char *)(pbPattern+2+(countSTATIC-count)) == *(char *)(pbTarget-cbPattern+2+(countSTATIC-count)) ) {
				count--;
			}
			if ( count == 0 ) return((pbTarget-cbPattern));
		}
		pbTarget++;
		if (pbTarget > pbTargetMax) return(NULL);
	}
}

// Last change: 2014-Apr-27
// If you want to help me to improve it, email me at: sanmayce@sanmayce.com
// Enfun!
