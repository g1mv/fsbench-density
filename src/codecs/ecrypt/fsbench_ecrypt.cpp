#include "fsbench_ecrypt.hpp"
#include "tools.hpp"

extern "C"
{
#include "ecrypt-sync-aes128-bernstein.h"
#include "ecrypt-sync-aes256-hongjun.h"
#include "ecrypt-sync-chacha.h"
#include "ecrypt-sync-hc-128.h"
#include "ecrypt-sync-hc-256.h"
#include "ecrypt-sync-lex.h"
#include "ecrypt-sync-rabbit.h"
#include "ecrypt-sync-rc4.h"
#include "ecrypt-sync-salsa20-8.h"
#include "ecrypt-sync-salsa20-12.h"
#include "ecrypt-sync-salsa20.h"
#include "ecrypt-sync-snow-2.0.h"
#include "ecrypt-sync-sosemanuk.h"
#include "ecrypt-sync-trivium.h"
}

#include <climits>
using namespace std;


template< typename ECRYPT_ctx>
static inline size_t process(char* in, size_t isize, char* out, size_t osize,
    void (*ECRYPT_keysetup)(
        ECRYPT_ctx* ctx,
        const u8* key,
        u32 _keysize,
        u32 _ivsize),
    void (*ECRYPT_ivsetup)(
        ECRYPT_ctx* ctx,
        const u8* iv),
    void (*ECRYPT_process_bytes)(
      ECRYPT_ctx* ctx,
      const u8* in,
      u8* out,
      u32 msglen),
    size_t ivsize,
    size_t keysize
    )
{
    ECRYPT_ctx ctx;
    u8* key = new u8[keysize/CHAR_BIT];
    memset(key, 0, keysize/CHAR_BIT);
    u8* iv = new u8[ivsize/CHAR_BIT];
    memset(iv, 0, ivsize/CHAR_BIT);
    ECRYPT_keysetup(&ctx, key, keysize, ivsize);
    ECRYPT_ivsetup(&ctx, iv);
    ECRYPT_process_bytes(&ctx, (const u8*) in, (u8*)out, isize);
    delete[] key;
    delete[] iv;
	return isize;
}
template<typename ECRYPT_ctx>
static inline size_t process2(char* in, size_t isize, char* out, size_t osize,
    void (*ECRYPT_keysetup)(
        ECRYPT_ctx* ctx,
        const u8* key,
        u32 _keysize,
        u32 _ivsize),
    void (*ECRYPT_ivsetup)(
        ECRYPT_ctx* ctx,
        const u8* iv),
    void (*ECRYPT_process_bytes)(
        int action,
        ECRYPT_ctx* ctx,
        const u8* in,
        u8* out,
        u32 msglen),
    int action,
    size_t ivsize,
    size_t keysize
    )
{
    ECRYPT_ctx ctx;
    u8* key = new u8[keysize/CHAR_BIT];
    memset(key, 0, keysize/CHAR_BIT);
    u8* iv = new u8[ivsize/CHAR_BIT];
    memset(iv, 0, ivsize/CHAR_BIT);
    ECRYPT_keysetup(&ctx, key, keysize, ivsize);
    ECRYPT_ivsetup(&ctx, iv);
    ECRYPT_process_bytes(action, &ctx, (const u8*) in, (u8*)out, isize);
    delete[] key;
    delete[] iv;
	return isize;
}

static const string invalid_keysize("Invalid keysize");
#define INIT(KEYSIZE, MAXKEYSIZE, INIT)                                          \
    INIT();                                                                      \
    CodecWithIntModes::init(args, threads_no, isize, init_encoder, init_decoder);\
    intptr_t keysize = (intptr_t)(eparams()[0]);                                 \
    for (int i=0; KEYSIZE(i) <= MAXKEYSIZE;++i)                                  \
    {                                                                            \
        if(keysize == KEYSIZE(i))                                                \
            return;                                                              \
    }                                                                            \
    throw InvalidParams(invalid_keysize);

#define ENCODER_DECODER_DEFINITION(CLASS_NAME)                                                           \
size_t CLASS_NAME::encode(char* in, size_t isize, char* out, size_t osize, void* keysize)                \
{                                                                                                        \
    return process<CLASS_NAME##_ctx>(in, isize, out, osize,                                              \
        CLASS_NAME##_keysetup, CLASS_NAME##_ivsetup, CLASS_NAME##_encrypt_bytes, CLASS_NAME##_MAXIVSIZE, \
        *(intptr_t*)keysize);                                                                            \
}                                                                                                        \
size_t CLASS_NAME::decode(char* in, size_t isize, char* out, size_t osize, void* keysize)                \
{                                                                                                        \
    return process<CLASS_NAME##_ctx>(in, isize, out, osize,                                              \
        CLASS_NAME##_keysetup, CLASS_NAME##_ivsetup, CLASS_NAME##_decrypt_bytes, CLASS_NAME##_MAXIVSIZE, \
        *(intptr_t*)keysize);                                                                            \
}

#define ENCODER_DECODER_DEFINITION2(CLASS_NAME)                                                          \
size_t CLASS_NAME::encode(char* in, size_t isize, char* out, size_t osize, void* keysize)                \
{                                                                                                        \
    return process2<CLASS_NAME##_ctx>(in, isize, out, osize,                                             \
        CLASS_NAME##_keysetup, CLASS_NAME##_ivsetup, CLASS_NAME##_process_bytes,                         \
        0,                                                                                               \
        CLASS_NAME##_MAXIVSIZE, *(intptr_t*)keysize);                                                    \
}                                                                                                        \
size_t CLASS_NAME::decode(char* in, size_t isize, char* out, size_t osize, void* keysize)                \
{                                                                                                        \
    return process2<CLASS_NAME##_ctx>(in, isize, out, osize,                                             \
        CLASS_NAME##_keysetup, CLASS_NAME##_ivsetup, CLASS_NAME##_process_bytes,                         \
        1,                                                                                               \
        CLASS_NAME##_MAXIVSIZE, *(intptr_t*)keysize);                                                    \
}

#define CLASS_INITS(CLASS_NAME, DAFULT_KEYSIZE, VERSION)                                                                   \
    CLASS_NAME::CLASS_NAME():                                                                                              \
        ECryptCodec(#CLASS_NAME, VERSION, encode, decode, CLASS_NAME##_KEYSIZE(0), CLASS_NAME##_MAXKEYSIZE, DAFULT_KEYSIZE)\
    {}                                                                                                                     \
void CLASS_NAME::init(const std::string& args, unsigned threads_no, size_t isize, bool init_encoder, bool init_decoder)    \
{                                                                                                                          \
    INIT(CLASS_NAME##_KEYSIZE, CLASS_NAME##_MAXKEYSIZE, CLASS_NAME##_init)                                                 \
}

#define ECRYPT_CLASS_DEFINITION(CLASS_NAME, DAFULT_KEYSIZE, VERSION)\
    CLASS_INITS(CLASS_NAME, DAFULT_KEYSIZE, VERSION)                \
    ENCODER_DECODER_DEFINITION(CLASS_NAME)
    
#define ECRYPT_CLASS_DEFINITION2(CLASS_NAME, DAFULT_KEYSIZE, VERSION)\
    CLASS_INITS(CLASS_NAME, DAFULT_KEYSIZE, VERSION)                 \
    ENCODER_DECODER_DEFINITION2(CLASS_NAME)
        
namespace FsBenchECrypt
{
ECryptCodec::ECryptCodec(const std::string& name, const std::string& version,
        encoder_t encode, decoder_t decode,
        intptr_t min_mode, intptr_t max_mode, const std::string& default_mode):
    CodecWithIntModes(name, version, encode, decode, min_mode, max_mode, default_mode, no_blowup, moving, moving, false)
    {}
    
string ECryptCodec::help() const                                                                                \
{                                                                                                                      \
    return this->name + " " + this->version + "\nYou can use key length as a parameter, but only some are accepted\n"; \
}

ECRYPT_CLASS_DEFINITION2(AES128Bernstein, "128", "little-4")
ECRYPT_CLASS_DEFINITION2(AES256Hongjun,   "256", "v1")
ECRYPT_CLASS_DEFINITION (ChaCha,          "128", "20080118")
ECRYPT_CLASS_DEFINITION2(HC128,           "128", "2007-01b")
ECRYPT_CLASS_DEFINITION2(HC256,           "256", "2007-01")
ECRYPT_CLASS_DEFINITION2(Lex,             "128", "v2")
ECRYPT_CLASS_DEFINITION2(Rabbit,          "128", "opt2")
ECRYPT_CLASS_DEFINITION2(RC4,             "128", "2005-08-21")
ECRYPT_CLASS_DEFINITION (Salsa20_8,       "256", "merged")
ECRYPT_CLASS_DEFINITION (Salsa20_12,      "256", "merged")
ECRYPT_CLASS_DEFINITION (Salsa20,         "256", "merged")
ECRYPT_CLASS_DEFINITION2(Snow2,           "256", "fast")
ECRYPT_CLASS_DEFINITION2(Sosemanuk,       "256", "2005-04-26")
ECRYPT_CLASS_DEFINITION2(Trivium,          "80", "2006-02-23")

} // FsBenchECrypt