// PpmdDecoder.h
// 2009-03-11 : Igor Pavlov : Public domain

#ifndef __COMPRESS_PPMD_DECODER_H
#define __COMPRESS_PPMD_DECODER_H

#include "../../../C/Ppmd7.h"

#include "../../Common/MyCom.h"

#include "../Common/CWrappers.h"

#include "../ICoder.h"

namespace NCompress {
namespace NPpmd {

class CDecoder :
  public CMyUnknownImp
{
  Byte *_outBuf;
  CPpmd7z_RangeDec _rangeDec;
  CByteInBufWrap _inStream;
  CPpmd7 _ppmd;

  Byte _order;
  bool _outSizeDefined;
  int _status;
  UInt64 _outSize;
  UInt64 _processedSize;

  HRESULT CodeSpec(Byte *memStream, UInt32 size);

public:

  #ifndef NO_READ_FROM_CODER
  CMyComPtr<ISequentialInStream> InSeqStream;
  #endif

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
  STDMETHOD(SetOutStreamSize)(const UInt64 *outSize);

  #ifndef NO_READ_FROM_CODER
  STDMETHOD(SetInStream)(ISequentialInStream *inStream);
  STDMETHOD(ReleaseInStream)();
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  #endif

  CDecoder(): _outBuf(NULL), _outSizeDefined(false)
  {
    Ppmd7z_RangeDec_CreateVTable(&_rangeDec);
    _rangeDec.Stream = &_inStream.p;
    Ppmd7_Construct(&_ppmd);
  }

  ~CDecoder();
};

}}

#endif
