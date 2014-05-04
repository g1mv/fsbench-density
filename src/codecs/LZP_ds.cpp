typedef unsigned char byte;
typedef unsigned int  uint;

#include <climits>
#include <cstring>

#define _rotr(x, r) ((x >> r) | (x << CHAR_BIT * sizeof(x) - r))

const int MinMatchLen = 32;

template <class T>
inline T CLAMP(const T& X,const T& LoX,const T& HiX) { return (X >= LoX)?((X <= HiX)?(X):(HiX)):(LoX); }

enum { H_BITS=17, H_SIZE=1 << H_BITS, LZP_MATCH_FLAG=0xB5 };

const byte MO2MML[4] = {5,11,19,44};
uint GetMinMatchLen(int MaxOrder) {
  return (MaxOrder < 6)?(MO2MML[MaxOrder-2]):(CLAMP(10*MaxOrder-15,51,475));
}

uint& lzpC(byte* p) { return *(uint*)(p-4); }

uint lzpH(uint c,byte* p) {
//    return (c+11*(c >> 15)+13*lzpC(p-1)) & (H_SIZE-1);
    return (c+5*_rotr(c,17)+3*lzpC(p-1)) & (H_SIZE-1);
}

#define LZP_INIT(Pattern)                                 \
  uint i, k, n1=1, n=1;                                   \
  byte* p;                                                \
  byte* InEnd=In+Size;                                    \
  byte* OutStart=Out;                                     \
  byte* HTable[H_SIZE];                                   \
  for( i=0; i<H_SIZE; i++ ) HTable[i]=Pattern+5;          \
  lzpC(Out+4) = lzpC(In+4);                               \
  lzpC(Out+8) = lzpC(In+8);                               \
  i = lzpC(Out+=12)=lzpC(In += 12);                       \
  k = lzpH(i,Out);                                        \

int LZPEncode( byte* In, uint Size, byte* Out, int MinLen ) {
  byte* OutEnd=Out+Size;
  LZP_INIT(In);
  do {
    p=HTable[k];
    if ( !--n )  { HTable[k]=In;        n=n1; }
    if (i != lzpC(p))                   *Out++ = *In++;
    else if (In+MinLen <= InEnd && lzpC(p+MinLen) == lzpC(In+MinLen)) {
      for (i=4;In+i <= InEnd && lzpC(p+i) == lzpC(In+i);i += 4)
              ;
      for (i -= 4;In+i < InEnd && In[i] == p[i];i++)
              ;
      if (i < MinLen)                 goto MATCH_NOT_FOUND;
      HTable[k]=In;                   n1 += (In-p > (n1+1)*H_SIZE && n1 < 7);
      *Out++ = LZP_MATCH_FLAG;        In += (k=i);
      for (i -= MinLen;i >= 254;i -= 254)
              *--OutEnd = 0;
      *--OutEnd = i+1;
      while(int(k -= 2*n1+1) > 0)     HTable[lzpH(lzpC(In-k),In-k)]=In-k;
    } else {
MATCH_NOT_FOUND:
      if ((*Out++ = *In++) == LZP_MATCH_FLAG) *--OutEnd = 255;
    }
    k=lzpH(i=lzpC(In),In);
  } while (In < InEnd);
  if( Out+256+(Size >> 7) > OutEnd ) return -1;
  memmove(Out,OutEnd,OutStart+Size-OutEnd);
  return Size-(OutEnd-Out);
}

int LZPDecode( byte* In, uint Size, byte* Out, int MinLen ) {
  LZP_INIT(Out);
  do {
      p=HTable[k];
      if ( !--n )  { HTable[k]=Out;       n=n1; }
      if (*In++ != LZP_MATCH_FLAG || i != lzpC(p) || *--InEnd == 255)
              *Out++ = In[-1];
      else {
          HTable[k]=Out;                  n1 += (Out-p > (n1+1)*H_SIZE && n1 < 7);
          for (i=MinLen-1;*InEnd == 0;InEnd--)
                  i += 254;
          i += *InEnd;                    k=2*n1+2;
          do {
              if ( !--k ) { k=2*n1+1;     HTable[lzpH(lzpC(Out),Out)]=Out; }
              *Out++ = *p++;
          } while ( --i );
      }
      k=lzpH(i=lzpC(Out),Out);
  } while (In < InEnd);
  return (Out-OutStart);
}

/*
void ProcessFile(const char* FName,int MinMatchLen) {
  char WrkStr[256], Ext[4];

  FILE * fp = fopen(FName,"rb");
  fseek(fp,0,SEEK_END); int InLen=ftell(fp); fseek(fp,0,SEEK_SET); // InLen = filelength
  byte* InBuf  = new byte[InLen];
  fread(InBuf,sizeof(byte),InLen,fp);
  fclose(fp);

  int OutLen, WrkLen;
  byte* OutBuf = new byte[InLen]; 
  byte* WrkBuf = new byte[InLen+1];

  OutLen = LZPEncode( InBuf, InLen, OutBuf, MinMatchLen );

  fp = fopen( FName1,"wb" );
  if( OutLen<0 ) {
    fwrite( InBuf,  sizeof(byte), InLen,  fp );
  } else {
    fwrite( OutBuf, sizeof(byte), OutLen, fp );
    WrkLen=LZPDecode( OutBuf, OutLen, WrkBuf, MinMatchLen );
    printf("%5.2f, %5.2f\n",float(t0)/CLOCKS_PER_SEC,float(t1)/CLOCKS_PER_SEC);
    if (InLen != WrkLen || memcmp(InBuf,WrkBuf,InLen))
            printf("\nComparision failed\n");
  }

  fclose(fp);
  delete InBuf;
  delete OutBuf;
  delete WrkBuf;
}
*/
