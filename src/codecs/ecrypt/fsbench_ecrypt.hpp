#include "abstractCodecs.hpp"

namespace FsBenchECrypt
{
struct ECryptCodec : CodecWithIntModes
{
    ECryptCodec(const std::string& name, const std::string& version,
        encoder_t encode, decoder_t decode,
        intptr_t min_mode, intptr_t max_mode, const std::string& default_mode);
    virtual void init(const std::string& args, unsigned threads_no, size_t isize, bool init_encoder=true, bool init_decoder=true) = 0;
    virtual std::string help() const;
};

#define ECRYPT_CLASS_DECLARATION(CLASS_NAME)                                                                                      \
struct CLASS_NAME : ECryptCodec                                                                                                   \
{                                                                                                                                 \
    CLASS_NAME();                                                                                                                 \
    virtual void init(const std::string& args, unsigned threads_no, size_t isize, bool init_encoder=true, bool init_decoder=true);\
    static size_t encode(char* in, size_t isize, char* out, size_t osize, void* keysize);                                         \
    static size_t decode(char* in, size_t isize, char* out, size_t osize, void* keysize);                                         \
}
ECRYPT_CLASS_DECLARATION(AES128Bernstein);
ECRYPT_CLASS_DECLARATION(AES256Hongjun);
ECRYPT_CLASS_DECLARATION(ChaCha);
ECRYPT_CLASS_DECLARATION(HC128);
ECRYPT_CLASS_DECLARATION(HC256);
ECRYPT_CLASS_DECLARATION(Lex);
ECRYPT_CLASS_DECLARATION(Rabbit);
ECRYPT_CLASS_DECLARATION(RC4);
ECRYPT_CLASS_DECLARATION(Salsa20_8);
ECRYPT_CLASS_DECLARATION(Salsa20_12);
ECRYPT_CLASS_DECLARATION(Salsa20);
ECRYPT_CLASS_DECLARATION(Snow2);
ECRYPT_CLASS_DECLARATION(Sosemanuk);
ECRYPT_CLASS_DECLARATION(Trivium);

} // FsBenchECrypt