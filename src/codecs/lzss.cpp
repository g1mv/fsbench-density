// lzss.cpp
#define _CRT_DISABLE_PERFCRIT_LOCKS


#include "lzss.hpp"

#include <stdio.h>
#include <memory.h>
#include <algorithm>
/**
 * Compresses a buffer
 * 
 * @param in: Data to be compressed
 * @param out: Compressed data goes here, It must be big enough, don't ask me how much is it.
 * @param size: size of input data
 * @param max: maximum compression mode. Stronger, slower.
 * @return output size
 */
int LZSSIM::lzssim_encode(unsigned char* in, unsigned char* out, int size, bool max) {
  unsigned char* buf = new unsigned char[N+MINMATCH];
  
  unsigned char* out0 = out;

  int n;
  // Input is split to blocks of at most N bytes,
  // each gets processed independently
  while ((n = std::min(N, size)) > 0) {
    memcpy(buf, in, n);
    in += n;
    size -= n;
    int i=0;

    memset(&head, 0, sizeof(head));
    memset(&path, 0, sizeof(path));

    // filling a hash chain match finder,
    // it stores the best match at each position
    for (i=0; i<n; i++) {

      int len=MINMATCH-1;
      int offset=W;

      int end=i+MAXMATCH;
      if (end>n)
        end=n;
      unsigned int hash=reinterpret_cast<unsigned int&>(buf[i])
          &0xffffff;

      int pos=head[hash];
      int k=max?1<<14:4; // we'll try this many matches

      while ((pos)&&(k--)) {
        const int x=i-pos;
        if (x>W)
          break; // out of window

        if (buf[pos+len]==buf[i+len]) {
          int p=pos;
          int j=i;
          while ((buf[p++]==buf[j])&&(++j<end));

          if ((j-=i)>len) {
            // we found the longest match yet
            offset=x-1;
            if ((i+(len=j))==end)
              break;
          }
        }

        pos=prev[pos];
      }

      path[i][0]=len;
      path[i][1]=offset;

      prev[i]=head[hash];
      head[hash]=i;
    }

    // parsing
    for (i=n-2; i>0; i--) {
      const int len=path[i][0];

      // literal cost, in bits
      const int c1=path[i+1][2] // cost of the end of the path
                             +1 // literal flag
                             +8;// character 

      if (len>=MINMATCH) {
        // match cost
        int c2=1    // match flag
                +6  // length
                +18;// offset

        const int p=i+len;
        if (p<n)
          c2+=path[p][2]; // cost of the rest of the path

        if (c1<c2) {
          path[i][0]=0;
          path[i][2]=c1;
        }
        else
          path[i][2]=c2;
      }
      else
        path[i][2]=c1;
    }
    
    // encoding...
    unsigned char t[25];
    t[0]=0;
    int ptr=1;
    int cnt=0;

    i=0;
    while (i<n) {
      const int len=path[i][0];
      const int offset=path[i][1];

      if (len>=MINMATCH) {
        t[0]+=(1<<cnt);
        t[ptr++]=((offset&3)<<6)+(len-MINMATCH);
        t[ptr++]=offset>>2;
        t[ptr++]=offset>>10;
        i+=len;
      }
      else
        t[ptr++]=buf[i++];

      if (++cnt==8) {
        memcpy(out, t, ptr);
        out += ptr;
        t[0]=0;
        ptr=1;
        cnt=0;
      }
    }
    
    if (cnt) {
      memcpy(out, t, ptr);
      out += ptr;
    }
  }
  delete[] buf;
  return (int) (out-out0);
}


void LZSSIM::lzssim_decode(unsigned char* in, unsigned char* out, int size) {

  unsigned char* buf = new unsigned char[N+MINMATCH];
  while (size>0) {
    int i=0;
    int tag=2;

    while ((i<N)&&(i<size)) {
      if ((tag>>=1)==1)
        tag=256+*in++;

      if (tag&1) {
        int len=*in++;
        unsigned char tmp = (*in++);
        int p=i-(1+(len>>6)+(tmp<<2)+((*in++)<<10));
        len&=63;
        buf[i++]=buf[p++];
        buf[i++]=buf[p++];
        buf[i++]=buf[p++];
        while (len--)
          buf[i++]=buf[p++];
      }
      else
        buf[i++]=*in++;
    }

    memcpy(out, buf, i);
    out += i;
    size-=i;
  }
  delete[] buf;
}
/*
int main(int argc, char **argv) {
  printf("lzss v0.01 by encode\n");

  if ((argc!=4)||((argv[1][0]&254)!='d')) {
    fprintf(stderr, "usage: lzss e[x]|d in out\n");
    return (1);
  }

  if (!(in=fopen(argv[2], "rb"))) {
    perror(argv[2]);
    return (1);
  }
  if (!(out=fopen(argv[3], "wb"))) {
    perror(argv[3]);
    return (1);
  }

  if (argv[1][0]&1)
    encode(argv[1][1]=='x');
  else
    decode();

  fclose(out);
  fclose(in);

  printf("done\n");

  return (0);
}*/
