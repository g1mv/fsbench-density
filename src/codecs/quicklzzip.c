// QuickLZ ZIP 0.4
// Copyright 2007, Lasse Mikkel Reinhold
// GPL-1 and GPL-2 license. May be used in closed source freeware by contacting author

#include <string.h>

//*****************************************************************************************'
//
// DEFLATE
//
//*****************************************************************************************'

static unsigned int bitoffset2;
static unsigned char *dst2;
static unsigned char *hashtable[8192];

static __inline out(unsigned int data, unsigned int len)
{
	unsigned int *dst = (unsigned int *)(dst2 + (bitoffset2 >> 3));
	unsigned int bo = (bitoffset2 & 0x7);
	*dst = ((*dst) & ((1 << bo) - 1)) | (data << bo);
	bitoffset2 += len;
}

static __inline unsigned int revbits(unsigned int n, int numbits)
{
	static const unsigned char rev5[32] = {0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30, 1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31};
	unsigned int i, result = 0;
	if(numbits == 5)
		return rev5[n];
	else
	{
		for (i = numbits; i; --i)
		{
			result <<= 1;
			if (n & 1)
			result |= 1;
			n >>= 1;
		}
		return result;
	}
}

static __inline encode_offset(unsigned int offset)
{
	static unsigned int o;
	static unsigned int is_init = 0;
	static unsigned int offset_cache[32769];

	if (!is_init)
	{
		static const unsigned short dists[30] = {
			1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
			257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
			8193, 12289, 16385, 24577};
		static const unsigned short dext[30] = {
			0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
			7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
			12, 12, 13, 13};
	        
   		unsigned int i, extra, i2, offset;
		for(offset = 1; offset <= 32768; offset++)
		{
	   		for(i = 0; i < 30 && offset >= dists[i]; i++);
			i--;

			i2 = revbits(i, 5);
			extra = offset - dists[i];
			i2 |= (extra << 5);	
		
			offset_cache[offset] = i2 | ((dext[i] + 5) << 24);
		}
		is_init = 1;
	}
	o = offset_cache[offset];
	out(o & 0xffffff, o >> 24); 
 
}

static __inline encode_literal(unsigned int lit)
{
	static const unsigned short int encoding[256] = {
		12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252, 2, 130, 66, 194, 34, 162,
		98, 226, 18, 146, 82, 210, 50, 178, 114, 242, 10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218,
		 58, 186, 122, 250, 6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246, 14, 142
		, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254, 1, 129, 65, 193, 33, 161, 97, 225
		, 17, 145, 81, 209, 49, 177, 113, 241, 9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185
		, 121, 249, 5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245, 13, 141, 77, 205
		, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253, 19, 275, 147, 403, 83, 339, 211, 467, 51,
		 307, 179, 435, 115, 371, 243, 499, 11, 267, 139, 395, 75, 331, 203, 459, 43, 299, 171, 427, 107, 363
		, 235, 491, 27, 283, 155, 411, 91, 347, 219, 475, 59, 315, 187, 443, 123, 379, 251, 507, 7, 263, 135
		, 391, 71, 327, 199, 455, 39, 295, 167, 423, 103, 359, 231, 487, 23, 279, 151, 407, 87, 343, 215, 471
		, 55, 311, 183, 439, 119, 375, 247, 503, 15, 271, 143, 399, 79, 335, 207, 463, 47, 303, 175, 431,
		111, 367, 239, 495, 31, 287, 159, 415, 95, 351, 223, 479, 63, 319, 191, 447, 127, 383, 255, 511};

	if(lit < 144)
		out(encoding[lit], 8);
	else
		out(encoding[lit], 9);
}

static __inline encode_matchlen(unsigned int matchlen)
{
	unsigned int extra, i, code;
    static const unsigned short lens[29] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const unsigned short lext[29] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
        
	static const unsigned short matchlens[11] = {0, 0, 0, 64, 32, 96, 16, 80, 48, 112, 8};

	if(matchlen <= 10)
	{
		out(matchlens[matchlen], 7);
	}
	else
	{
		for(i = 0; i < 29 && matchlen >= lens[i]; i++);
		i--;
		code = i + 254 + 3; // 257...285 i1 = CODE
		
		if(code < 280)
		{
			code = revbits(code, 7);
			extra = matchlen - lens[i];
			code |= (extra << 7);
			out(code, 7 + lext[i]);
		}
		else
		{
			code = code + 192 - 280;
			code = revbits(code, 8);
			extra = matchlen - lens[i];	
			code |= (extra << 8);
			out(code, 8 + lext[i]);
		}
	}
}


unsigned int qlz_deflate(unsigned char *source, unsigned char *destination, unsigned int bitoffset, unsigned int size, unsigned int last)
{
	static unsigned int i;

	unsigned char *src = source;
	unsigned char *last_source_byte = source + size - 1;
	int literals = 0;


	dst2 = destination;
	bitoffset2 = bitoffset;

	out(last, 1); // last block?
	out(1, 2); // block type 01
	if(size > 0)
	{
		encode_literal(*src);
		src++;
	}
	while(src <= last_source_byte)
	{
		unsigned int hash, fetch;
		unsigned char *o;
		
		fetch = *((unsigned int *)src);
		hash = ((fetch >> 11) ^ fetch) & 0x1fff;
		o = hashtable[hash];
		hashtable[hash] = src;
		
		if(o >= src - 32768 && o < src && o > source && src < last_source_byte - 1 && ((fetch^(*(unsigned int*)o)) & 0xffffff) == 0)
		{
			unsigned int n = 3;
			while(src + n <= last_source_byte && *(o + n) == *(src + n) && n < 257)
				n++;
				
			encode_matchlen(n);
			encode_offset((unsigned int)(src - o));
			src += n;
		}
		else
		{
			encode_literal(*src);
			src++;
		}
	}
	
	out(0, 7); // end of block

	return bitoffset2;
}



//*****************************************************************************************'
//
// ZIP
//
//*****************************************************************************************'

// Unnecessary in FsBench
#if 0

static unsigned char central_dir[100000];
static unsigned int central_dir_size;
static unsigned int central_dir_offset;
static unsigned int local_file_header_offset;
static unsigned char fn[1000];
static unsigned int k, file_source_len, file_compressed_len, remain;
static unsigned int num_files, first_data_add;

unsigned int crc32(unsigned char *data, unsigned int size, unsigned int crc)
{
	static const unsigned int table[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

	while(size > 0)
	{
		size--;
		crc = (((crc >> 8) & 0xffffff) ^ table[(crc ^ *data++) & 0xff]);
	}
	return(crc);
}

void new_zip(void)
{
	num_files = 0;
	central_dir_size = 0;
	central_dir_offset = 0;
	local_file_header_offset = 0;
}

unsigned int add_file(char *filename)
{
	strcpy(fn, filename);
	k = -1;
	file_source_len = 0;
	file_compressed_len = 0;
	num_files++;
	remain = 0;
	
	first_data_add = 1;

	return 30 + strlen(fn); // local file header size
}

unsigned int end_file(char *destination)
{
	// local file header
	*(unsigned int*)destination = 0x04034b50;
	*(unsigned short int*)(destination + 4) = 20; // ver needed to extract
	*(unsigned short int*)(destination + 6) = 00; // general purpose flags
	*(unsigned short int*)(destination + 8) = 8; // deflate
	*(unsigned short int*)(destination + 10) = 0; // time
	*(unsigned short int*)(destination + 12) = 0; // date
	*(unsigned int*)(destination + 14) = k;  // crc32
	*(unsigned int*)(destination + 18) = file_compressed_len; // comp size
	*(unsigned int*)(destination + 22) = file_source_len; // comp size
	*(unsigned short int*)(destination + 26) = strlen(fn); // file name len
	*(unsigned short int*)(destination + 28) = 0; // ext field len

memcpy(destination + 30, fn, strlen(fn));
//	strcpy(destination + 30, fn);

	central_dir_offset += 30 + strlen(fn) + file_compressed_len;
	local_file_header_offset += 30 + strlen(fn) + file_compressed_len;

	return 30 + strlen(fn);
}

unsigned int add_data(char *source, char *destination, unsigned int size, unsigned int eof)
{
	unsigned int ret;
	static unsigned char remain_byte;
	unsigned char *cds;

	if(first_data_add == 1)
		destination += 30 + strlen(fn);

	*destination = remain_byte;
	k = crc32(source, size, k);
	ret = qlz_deflate(source, destination, remain, size, eof);
	remain = ret & 0x7;
	
	if(eof == 0)
		ret = ret >> 3;
	else
		ret = (ret >> 3) + (((ret & 0x7) != 0) ? 1 : 0);

	remain_byte = *(destination + ret);
	file_source_len += size;
	file_compressed_len += ret;

	if(eof == 1)
	{
		// add to central directory
		k = ~k; 
		cds = central_dir + central_dir_size; //destination + ret;
		*(unsigned int*)cds = 0x02014b50;
		*(unsigned short int*)(cds + 4) = 20; // made by 2.0
		*(unsigned short int*)(cds + 6) = 20; // ver needed to extract
		*(unsigned short int*)(cds + 8) = 00; // general purpose flags
		*(unsigned short int*)(cds + 10) = 8; // deflate
		*(unsigned short int*)(cds + 12) = 0; // time
		*(unsigned short int*)(cds + 14) = 0; // date
		*(unsigned int*)(cds + 16) = k;  // crc32
		*(unsigned int*)(cds + 20) = file_compressed_len; // comp size
		*(unsigned int*)(cds + 24) = file_source_len; // decomp size	
		*(unsigned short int*)(cds + 28) = strlen(fn); // file name len
		*(unsigned short int*)(cds + 30) = 0; // ext field len
		*(unsigned short int*)(cds + 32) = 0; // file comment len
		*(unsigned short int*)(cds + 34) = 0; // disk num start
		*(unsigned short int*)(cds + 36) = 1; // internal attr
		*(unsigned int*)(cds + 38) = 32; // external attr
		*(unsigned int*)(cds + 42) = local_file_header_offset; // rel offset
		memcpy(cds + 46, fn, strlen(fn)); // todo: brug strcpy
		central_dir_size += 46 + strlen(fn);
	}

	if(first_data_add == 1)
		ret += 30 + strlen(fn);
	first_data_add = 0;

	return ret;
}
unsigned int end_zip(unsigned char *destination)
{	
	char *endof;
	// end of central directory
	memcpy(destination, central_dir, central_dir_size);
	endof = destination + central_dir_size;
	*(unsigned int*)endof = 0x06054b50;
	*(unsigned short int*)(endof + 4) = 0;
	*(unsigned short int*)(endof + 6) = 0;	
	*(unsigned short int*)(endof + 8) = num_files;	
	*(unsigned short int*)(endof + 10) = num_files;	
	*(unsigned int*)(endof + 12) = central_dir_size; // size of the central directory
	*(unsigned int*)(endof + 16) = central_dir_offset; // offset of start of central directory 
	*(unsigned short int*)(endof + 20) = 0;	// comment len		
	
	return 22 + central_dir_size;
}

#endif