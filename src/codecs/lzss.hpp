#ifndef __LZSS_HPP_njsaihf9H5
#define __LZSS_HPP_njsaihf9H5

static const int MINMATCH = 3;
static const int MAXMATCH = 63 + MINMATCH;
static const int N = 1 << 25; // Input is split into blocks of N bytes 
static const int W = 1 << 18; // Window size

static const int HASHBITS = 24;
static const int HASHSIZE = 1 << HASHBITS;

class LZSSIM
{
    int head[HASHSIZE];
    int prev[N];
    /**
     * Contains parser paths
     * path[i] describes the best result at the position i
     * path[i][0] - length of the best match at i
     * path[i][1] - offset of the best match at i
     * path[i][2] - cost of path from i to the end of block
     */
    int path[N][3];
public:
    int lzssim_encode(unsigned char* in, unsigned char* out, int size, int max);
    static void lzssim_decode(unsigned char* in, unsigned char* out, int size);
};
#endif // __LZSS_HPP_njsaihf9H5
