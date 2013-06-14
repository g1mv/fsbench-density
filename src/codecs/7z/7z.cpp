#include "7z.hpp"
#include "codecs.hpp"
#include "misc.hpp"

#include "CPP/7zip/IStream.h"
#include "CPP/7zip/Compress/DeflateDecoder.h"
#include "CPP/7zip/Compress/DeflateEncoder.h"
#include "CPP/7zip/Compress/PpmdEncoder.h"
#include "CPP/7zip/Compress/PpmdDecoder.h"
#if 0
#include "CPP/7zip/Compress/LzxDecoder.h"
#endif

#if __cplusplus >= 201103L // C++ 2011
#include <cstdint>
#else
extern "C"
{
#include <stdint.h>
}
#endif
#include <algorithm>
#include <memory.h>

using namespace std;

namespace FsBench7z
{

struct IOStream : ISequentialInStream, ISequentialOutStream
{
    STDMETHOD(Read)(void * data, UInt32 size, UInt32 * processedSize);
    STDMETHOD(Write)(const void * data, UInt32 size, UInt32 * processedSize);

    IOStream(char * buf_, size_t size_):
        size(size_),
        buf(buf_)
    {}
    size_t size;
private:
    char * buf;
      STDMETHOD(QueryInterface) (REFIID, void**)
      {return S_OK;}
      STDMETHOD_(ULONG, AddRef)()
      {return S_OK;}
      STDMETHOD_(ULONG, Release)()
      {return S_OK;}
};
struct ProgressInfo : ICompressProgressInfo
{
  STDMETHOD(SetRatioInfo)(const UInt64*, const UInt64*)
  {return S_OK;}
  STDMETHOD(SetTotal)(UInt64)
  {return S_OK;}
  STDMETHOD(SetCompleted)(const UInt64 *)
  {return S_OK;}

private:

  STDMETHOD(QueryInterface) (REFIID, void**)
  {return S_OK;}
  STDMETHOD_(ULONG, AddRef)()
  {return S_OK;}
  STDMETHOD_(ULONG, Release)()
  {return S_OK;}
};


static const UInt32 kAlgo1 = 0;
static const UInt32 kAlgo5 = 1;

static const UInt32 kPasses1 = 1;
static const UInt32 kPasses7 = 3;
static const UInt32 kPasses9 = 10;

static const UInt32 kFb1 = 32;
static const UInt32 kFb7 = 64;
static const UInt32 kFb9 = 128;

template<typename T>
HRESULT set_deflate_compress_level(T& coder, int level)
{
#define NUM_PROPS 3
    PROPID PROPIDs[NUM_PROPS];
    PROPVARIANT PROPVARIANTs[NUM_PROPS];

    PROPIDs[0] = NCoderPropID::kAlgorithm;
    PROPVARIANTs[0].vt = VT_UI4;
    PROPVARIANTs[0].ulVal = (level >= 5 ?
        kAlgo5 :
        kAlgo1);

    PROPIDs[1] = NCoderPropID::kNumPasses;
    PROPVARIANTs[1].vt = VT_UI4;
    PROPVARIANTs[1].ulVal =
        (level >= 9 ? kPasses9 :
        (level >= 7 ? kPasses7 :
            kPasses1));
    PROPIDs[2] = NCoderPropID::kNumFastBytes;
    PROPVARIANTs[2].vt = VT_UI4;
    PROPVARIANTs[2].ulVal =
        (level >= 9 ? kFb9 :
        (level >= 7 ? kFb7 :
            kFb1));

    return coder.SetCoderProperties(PROPIDs, PROPVARIANTs, NUM_PROPS);
}

STDMETHODIMP IOStream::Read(void * data, UInt32 size_, UInt32 * processedSize)
{
    UInt32 to_consume = min(this->size, (size_t)size_);
    memcpy(data, buf, to_consume);
    buf += to_consume;
    this->size -= to_consume;
    *processedSize = to_consume;
    return S_OK;
}
STDMETHODIMP IOStream::Write(const void * data, UInt32 size_, UInt32 * processedSize)
{
    if(this->size == 0)
        return E_OUTOFMEMORY;

    UInt32 to_write = min(this->size, (size_t)size_);
    memcpy(buf, data, to_write);
    buf += to_write;
    this->size -= to_write;
    *processedSize = to_write;
    return S_OK;
}

template<typename T>
size_t _decompress(T coder, char * in, size_t isize, char * out, size_t osize, void * _)
{
    ProgressInfo pi;
    IOStream istream(in,  isize);
    IOStream ostream(out, osize);
    const UInt64 inSize  = isize;
    const UInt64 outSize = osize;
    HRESULT ret = coder.Code(&istream, &ostream, &inSize, &outSize, &pi);
    if(ret != S_OK)
        return CODING_ERROR;
    return osize;
}
template<typename T>
size_t _compress(T coder,
                 HRESULT (*set_compress_level)(T& coder, int level),
                 char * in,
                 size_t isize,
                 char * out,
                 size_t osize,
                 void * mode)
{
    ProgressInfo pi;
    IOStream istream(in,  isize);
    IOStream ostream(out, osize);
    HRESULT ret = set_compress_level(coder, *(intptr_t*)mode);
    if(ret != S_OK)
        return CODING_ERROR;
    const UInt64 inSize  = isize;
    const UInt64 outSize = osize;
    ret = coder.Code(&istream, &ostream, &inSize, &outSize, &pi);
    if(ret != S_OK)
        return CODING_ERROR;
    return osize-ostream.size;
}


size_t inflate(char * in, size_t isize, char * out, size_t osize, void * _)
{
    NCompress::NDeflate::NDecoder::CCOMCoder coder;
    return _decompress(coder, in, isize, out, osize, _);
}

size_t deflate(char * in, size_t isize, char * out, size_t osize, void * mode)
{
    NCompress::NDeflate::NEncoder::CCOMCoder coder;
    return _compress(coder,
                     set_deflate_compress_level<NCompress::NDeflate::NEncoder::CCOMCoder>,
                     in,
                     isize,
                     out,
                     osize,
                     mode);
}

size_t inflate64(char * in, size_t isize, char * out, size_t osize, void * _)
{
    NCompress::NDeflate::NDecoder::CCOMCoder64 coder;
    return _decompress(coder, in, isize, out, osize, _);
}

size_t deflate64(char * in, size_t isize, char * out, size_t osize, void * mode)
{
    NCompress::NDeflate::NEncoder::CCOMCoder64 coder;
    return _compress(coder, set_deflate_compress_level<NCompress::NDeflate::NEncoder::CCOMCoder64>, in, isize, out, osize, mode);
}
#if 0
size_t unlzx(char * in, size_t isize, char * out, size_t osize, void * _)
{
    NCompress::NLzx::CDecoder coder;
    return _decompress(coder, in, isize, out, osize, _);
}
#endif


} // FsBench7z
