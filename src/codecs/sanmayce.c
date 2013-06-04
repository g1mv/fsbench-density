#include "sanmayce.h"

#ifdef FSBENCH_SSE2
#define XMM_KAZE_SSE2
//#define XMM_KAZE_SSE4
//#define XMM_KAZE_AVX
#endif

// "No High-Speed Limit", says Tesla.
// 
// As a youth, Tesla exhibited a peculiar trait that he considered the basis of all his invention.
// He had an abnormal ability, usually involuntary, to visualize scenes, people, and things so vividly that he was sometimes unsure of what was real and what imaginary.
// Strong flashes of light often accompanied these images.
// Tormented, he would move his hand in front of his eyes to determine whether the objects were simply in his mind or outside.
// He considered the strange ability an affliction at first, but for an inventor it could be a gift.
// Tesla wrote of these phenomena and of his efforts to find an explanation for them, since no psychologist or physiologist was ever able to help him.
// "The theory I have formulated," he wrote much later, is that the images were the result of a reflex action from the brain on the retina under great excitation.
// They certainly were not hallucinations, for in other respects I was normal and composed.
// To give an idea of my distress, suppose that I had witnessed a funeral or some such nerve-wracking spectacle.
// Then, inevitably, in the stillness of the night, a vivid picture of the scene would thrust itself before my eyes and persist despite all my efforts to banish it.
// Sometimes it would even remain fixed in space though I pushed my hand through it.
// /Excerpts from 'Tesla: Master of Lightning', by Margaret Cheney and Robert Uth/
// 
// "It is not the time," said Dr. Tesla yesterday, "to go into the details of this thing.
// It is founded on a principle that means great things in peace, it can be used for great things in war.
// But I repeat, this is no time to talk of such things."
// /New York Times, Dec. 8, 1915/
// 
// On the occasion of his 75th birthday, Tesla talked about new developments.
// "I am working now upon two things," he said.
// "First, an explanation based upon pure mathematics of certain things which Professor Einstein has also attempted to explain.
// My conclusions in certain respects differ from and to that extent tend to disprove the Einstein Theory . . . 
// My explanations of natural phenomena are not so involved as his.
// They are simpler, and when I am ready to make a full announcement it will be seen that I have proved my conclusions."
// "Secondly, I am working to develop a new source of power.
// When I say a new source, I mean that I have turned for power to a source which no previous scientist has turned, to the best of my knowledge.
// The conception, the idea when it first burst upon me was a tremendous shock."
// "It will throw light on many puzzling phenomena of the cosmos, and may prove also of great industrial value, particularly in creating a new and virtually unlimited market for steel."
// Tesla said it will come from an entirely new and unsuspected source, and will be for all practical purposes constant day and night, and at all times of the year.
// The apparatus for capturing the energy and transforming it will partake both of mechanical and electrical features, and will be of ideal simplicity.
// Tesla has already conceived a means that will make it possible for man to transmit energy in large amounts, thousands of horsepower, from one planet to another, absolutely regardless of distance.
// He considered that nothing can be more important than interplanetary communication.
// It will certainly come some day, and the certitude that there are other human beings in the universe, working, suffering, struggling, like ourselves, will produce a magic effect on mankind and will form the foundation of a universal brotherhood that will last as long as humanity itself.
// /Time, July 20, 1931/
// 
// Dr. Nikola Tesla asserted in an interview with Hugo Gernsback that speeds greater than that of light, which are considered impossible by the Einstein theory of relativity, have been produced.
// Stating that the Einstein theory is erroneous in many respects, Dr. Tesla stated as early as 1900, in his patent 787,412, that the current of his radio-power transmitter passed over the surface of the earth with a speed of 292,830 miles a second.
// According to the Einstein theory, the highest possible speed is 186,300 miles a second.
// Tesla indicated knowledge of speeds several times greater than light, and had apparatus designed to project so-called electrons with a speed equal to twice that of light.
// /The Literary Digest, Nov. 7, 1931/
// 
// He had discovered the so-called cosmic ray in 1896, at least five years before any other scientist took it up and twenty years before it became popular among scientists, and he is now convinced that many of the cosmic particles travel fifty times faster than light, some of them 500 times faster.
// /New York World-Telegram, July 11, 1935/
 
// Revision 2 (main mix should be refined), written 2012-Jun-16, thanks to Maciej Adamczyk for his testbed and openness.
 
#define ROL64(x, n) (((x) << (n)) | ((x) >> (64-(n))))
#define ROL(x, n) (((x) << (n)) | ((x) >> (32-(n))))
uint64_t FNV1A_Hash_Tesla(const char *str, size_t wrdlen)
{
	const uint64_t PRIME = 11400714819323198393ULL;
	uint64_t hash64 = 2166136261U; // should be much bigger
	uint64_t hash64B = 2166136261U; // should be much bigger
	const char *p = str;
	
	for(; wrdlen >= 2*2*2*sizeof(uint32_t); wrdlen -= 2*2*2*sizeof(uint32_t), p += 2*2*2*sizeof(uint32_t)) {
		hash64 = (hash64 ^ (ROL64(*(unsigned long long *)(p+0),5-0)^*(unsigned long long *)(p+8))) * PRIME;
		hash64B = (hash64B ^ (ROL64(*(unsigned long long *)(p+8+8),5-0)^*(unsigned long long *)(p+8+8+8))) * PRIME;
	}

        hash64 = (hash64 ^ hash64B); // Some mix, the simplest is given, maybe the B-line should be rolled by 32bits before xoring.

	// Cases: 0,1,2,3,4,5,6,7,... 15,... 31
	if (wrdlen & (2*2*sizeof(uint32_t))) {
		hash64 = (hash64 ^ (ROL(*(uint32_t *)(p+0),5-0)^*(uint32_t *)(p+4))) * PRIME;		
		hash64 = (hash64 ^ (ROL(*(uint32_t *)(p+4+4),5-0)^*(uint32_t *)(p+4+4+4))) * PRIME;		
		p += 2*2*sizeof(uint32_t);
	}
	if (wrdlen & (2*sizeof(uint32_t))) {
		hash64 = (hash64 ^ (ROL(*(uint32_t *)p,5-0)^*(uint32_t *)(p+4))) * PRIME;		
		p += 2*sizeof(uint32_t);
	}
	if (wrdlen & sizeof(uint32_t)) {
		hash64 = (hash64 ^ *(uint32_t*)p) * PRIME;
		p += sizeof(uint32_t);
	}
	if (wrdlen & sizeof(uint16_t)) {
		hash64 = (hash64 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
	}
	if (wrdlen & 1) 
		hash64 = (hash64 ^ *p) * PRIME;
	
	return hash64 ^ (hash64 >> 32);
}

uint64_t FNV1A_Hash_Tesla3(const char *str, size_t wrdlen)
{
	const uint64_t PRIME = 11400714819323198393ULL;
	uint64_t hash64 = 2166136261U; // should be much bigger
	uint64_t hash64B = 2166136261U; // should be much bigger
	const char *p = str;
    uint64_t Loop_Counter;
    uint64_t Second_Line_Offset;

if (wrdlen >= 2*2*2*sizeof(uint32_t)) {	
    Second_Line_Offset = wrdlen-((wrdlen>>5)+1)*(2*8); // ((wrdlen>>1)>>4)
    Loop_Counter = (wrdlen>>5);
    Loop_Counter++;
    for(; Loop_Counter; Loop_Counter--, p += 2*2*sizeof(uint32_t)) {
		hash64 = (hash64 ^ (ROL64(*(uint64_t *)(p+0),5) ^ *(uint64_t *)(p+0+Second_Line_Offset))) * PRIME;        
		hash64B = (hash64B ^ (ROL64(*(uint64_t *)(p+8+Second_Line_Offset),5) ^ *(uint64_t *)(p+8))) * PRIME;        
    }
} else {
	// Cases: 0,1,2,3,4,5,6,7,... 15,... 31
	if (wrdlen & (2*2*sizeof(uint32_t))) {
		hash64 = (hash64 ^ (ROL(*(uint32_t *)(p+0),5-0)^*(uint32_t *)(p+4))) * PRIME;		
		hash64 = (hash64 ^ (ROL(*(uint32_t *)(p+4+4),5-0)^*(uint32_t *)(p+4+4+4))) * PRIME;		
		p += 2*2*sizeof(uint32_t);
	}
	if (wrdlen & (2*sizeof(uint32_t))) {
		hash64 = (hash64 ^ (ROL(*(uint32_t *)p,5-0)^*(uint32_t *)(p+4))) * PRIME;		
		p += 2*sizeof(uint32_t);
	}
	if (wrdlen & sizeof(uint32_t)) {
		hash64 = (hash64 ^ *(uint32_t*)p) * PRIME;
		p += sizeof(uint32_t);
	}
	if (wrdlen & sizeof(uint16_t)) {
		hash64 = (hash64 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
	}
	if (wrdlen & 1) 
		hash64 = (hash64 ^ *p) * PRIME;
}
	hash64 = (hash64 ^ ROL64(hash64B,5) ) * PRIME;
	return hash64 ^ (hash64 >> 32);
}

// [North Star One-Sword School]
// - My name is Kanichiro Yoshimura.
//   I'm a new man. Just so you'll know who I am...
//   Saito-sensei.
// - What land are you from?
// - 'Land'?
// - Yes.
// - I was born in Morioka, in Nanbu, Oshu.
//   It's a beautiful place.
//   Please...
//   Away to the south is Mt Hayachine...
//   with Mt Nansho and Mt Azumane to the west.
//   In the north are Mt Iwate and Mt Himekami.
//   Out of the high mountains flows the Nakatsu River...
//   through the castle town into the Kitakami below Sakuranobaba.
//   Ah, it's pretty as a picture!
//   There's nowhere like it in all Japan!
// /Paragon Kiichi Nakai in the paragon piece-of-art 'The Wolves of Mibu' aka 'WHEN THE LAST SWORD IS DRAWN'/
// As I said on one Japanese forum, Kiichi Nakai deserves an award worth his weight in gold, nah-nah, in DIAMONDS!
uint32_t FNV1A_Hash_Yoshimura(const char *str, size_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    const char *p = str;
    uint32_t Loop_Counter;
    uint32_t Second_Line_Offset;

if (wrdlen >= 2*2*sizeof(uint32_t)) {
    Second_Line_Offset = wrdlen-((wrdlen>>4)+1)*(2*4); // ((wrdlen>>1)>>3)
    Loop_Counter = (wrdlen>>4);
    //if (wrdlen%16) Loop_Counter++;
    Loop_Counter++;
    for(; Loop_Counter; Loop_Counter--, p += 2*sizeof(uint32_t)) {
		// revision 1:
		//hash32 = (hash32 ^ (ROL(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
		//hash32B = (hash32B ^ (ROL(*(uint32_t *)(p+0+Second_Line_Offset),5) ^ *(uint32_t *)(p+4+Second_Line_Offset))) * PRIME;        
		// revision 2:
		hash32 = (hash32 ^ (ROL(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+0+Second_Line_Offset))) * PRIME;        
		hash32B = (hash32B ^ (ROL(*(uint32_t *)(p+4+Second_Line_Offset),5) ^ *(uint32_t *)(p+4))) * PRIME;        
    }
} else {
    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
		p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
		hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
		p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;
}
    hash32 = (hash32 ^ ROL(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}



// The jester as a symbol:
// The root of the word "fool" is from the Latin follis, which means "bag of wind" or that which contains air or breath.
// In Tarot, "The Fool" is the first card of the Major Arcana.
// The tarot depiction of the Fool includes a man, (or less often, a woman), juggling unconcernedly or otherwise distracted, with a dog (sometimes cat) at his heels.
// The fool is in the act of unknowingly walking off the edge of a cliff, precipice or other high place.
// Another Tarot character is Death. In the Middle Ages Death is often shown in Jester's garb because "The last laugh is reserved for death." Also, Death humbles everyone just as Jesters make fun of everyone regardless of standing.
// In literature, the jester is symbolic of common sense and of honesty, notably in King Lear, the court jester is a character used for insight and advice on the part of the monarch, taking advantage of his license to mock and speak freely to dispense frank observations and highlight the folly of his monarch.
// This presents a clashing irony as a "greater" man could dispense the same advice and find himself being detained in the dungeons or even executed. Only as the lowliest member of the court can the jester be the monarch's most useful adviser.
// Distinction was made between fools and clowns, or country bumpkins.
// The fool's status was one of privilege within a royal or noble household.
// His folly could be regarded as the raving of a madman but was often deemed to be divinely inspired.
// The 'natural' fool was touched by God. Much to Gonerill's annoyance, Lear's 'all-licensed' Fool enjoys a privileged status. His characteristic idiom suggests he is a 'natural' fool, not an artificial one, though his perceptiveness and wit show that he is far from being an idiot, however 'touched' he might be.
// The position of the Joker playing card, as a wild card which has no fixed place in the hierarchy of King, Queen, Knave, etc. might be a remnant of the position of the court jester.
// This lack of any place in the hierarchy meant Kings could trust the counsel of the jesters, as they had no vested interest in any region, estate or church.
// Source: en.wikipedia.org
uint32_t FNV1A_Hash_Jesteress(const char *str, size_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    const char *p = str;

    // Idea comes from Igor Pavlov's 7zCRC, thanks.
/*
    for(; wrdlen && ((unsigned)(ptrdiff_t)p&3); wrdlen -= 1, p++) {
        hash32 = (hash32 ^ *p) * PRIME;
    }
*/
    for(; wrdlen >= 2*sizeof(uint32_t); wrdlen -= 2*sizeof(uint32_t), p += 2*sizeof(uint32_t)) {
        hash32 = (hash32 ^ (ROL(*(uint32_t *)p,5)^*(uint32_t *)(p+4))) * PRIME;        
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
        hash32 = (hash32 ^ *(uint32_t*)p) * PRIME;
        p += sizeof(uint32_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;
    
    return hash32 ^ (hash32 >> 16);
}

// Meiyan means Beauty, Charming Eyes or most precisely: SOULFUL EYES.
uint32_t FNV1A_Hash_Meiyan(const char *str, size_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    const char *p = str;

    // Idea comes from Igor Pavlov's 7zCRC, thanks.
/*
    for(; wrdlen && ((unsigned)(ptrdiff_t)p&3); wrdlen -= 1, p++) {
        hash32 = (hash32 ^ *p) * PRIME;
    }
*/
    for(; wrdlen >= 2*sizeof(uint32_t); wrdlen -= 2*sizeof(uint32_t), p += 2*sizeof(uint32_t)) {
        hash32 = (hash32 ^ (ROL(*(uint32_t *)p,5)^*(uint32_t *)(p+4))) * PRIME;        
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
//		hash32 = (hash32 ^ *(uint32_t*)p) * PRIME;
//		p += sizeof(uint32_t);
		hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
		hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);        
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;
    
    return hash32 ^ (hash32 >> 16);
}

// Mantis has two(three to be exact) gears: it operates as uint16_t based FNV1A for 1..15 lengths and as QWORD based FNV1A 16.. lengths.
// I see the instant mantis' grasping-and-devouring as MONSTROUS QUADRO-BYTE-PAIRs BAITs(IN-MIX) while target secured within FIRM-GRIP of forelimbs(PRE-MIX & POST-MIX).
// Word 'mantical'(Of or relating to the foretelling of events by or as if by supernatural means) comes from Greek mantikos, from the Greek word mantis, meaning "prophet, seer."
// The Greeks, who made the connection between the upraised front legs of a mantis waiting for its prey and the hands of a prophet in prayer, used the name mantis to mean "the praying mantis."
uint32_t FNV1A_Hash_Mantis(const char *str, size_t wrdlen)
{
	const uint32_t PRIME = 709607;
	uint32_t hash32 = 2166136261;
	const char *p = str;
	// Cases: 0,1,2,3,4,5,6,7
	if (wrdlen & sizeof(uint32_t)) {
		hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
		hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
		//wrdlen -= sizeof(uint32_t);
	}
	if (wrdlen & sizeof(uint16_t)) {
		hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
		p += sizeof(uint16_t);
		//wrdlen -= sizeof(uint16_t);
	}
	if (wrdlen & 1) {
		hash32 = (hash32 ^ *p) * PRIME;
		p += sizeof(char);
		//wrdlen -= sizeof(char);
	}
		wrdlen -= p-str;
// The goal is to avoid the weak range [8, 8+2, 8+1] that is 8..10 in practice 1..15 i.e. 1..8+4+2+1, thus amending FNV1A_Meiyan and FNV1A_Jesteress.
// FNV1A_Jesteress: fastest strong
// FNV1A_Meiyan   : faster  stronger
// FNV1A_Mantis   : fast    strongest
	if (wrdlen) {
	for(; wrdlen > 2*sizeof(uint32_t); wrdlen -= 2*sizeof(uint32_t), p += 2*sizeof(uint32_t)) {
		hash32 = (hash32 ^ (ROL(*(uint32_t *)p,5)^*(uint32_t *)(p+4))) * PRIME;		
	}
		hash32 = (hash32 ^ *(uint16_t*)(p+0*sizeof(uint16_t))) * PRIME;
		hash32 = (hash32 ^ *(uint16_t*)(p+1*sizeof(uint16_t))) * PRIME;
		hash32 = (hash32 ^ *(uint16_t*)(p+2*sizeof(uint16_t))) * PRIME;
		hash32 = (hash32 ^ *(uint16_t*)(p+3*sizeof(uint16_t))) * PRIME;
	} // Bug Fixed!        
	return hash32 ^ (hash32 >> 16);
}


// Notes, 2013-Apr-26: 
// Wanted to see how SIMDed main loop would look like:
// One of the main goals: to stress 128bit registers only and nothing else, for now 6 in total, in fact Intel uses the all 8.
// Current approach: instead of rotating the 5 bits within the DWORD quadruplets I chose to do it within the entire DQWORD i.e. XMMWORD.
// Length of the main loop: 02795 - 0270d + 6 = 142 bytes
// CRASH CARAMBA: My CPU T7500 supports up to SSSE3 but not SSE4.1 and AVX, I need YMM (it reads 'yummy') machine.

// FNV1A_YoshimitsuTRIADiiXMM revision 1+ aka FNV1A_SaberFatigue, copyleft 2013-Apr-26 Kaze.
// Targeted purpose: x-gram table lookups for Leprechaun r17.
// Targeted machine: assuming SSE2 is present always - no non-SSE2 counterpart.
#if defined(XMM_KAZE_SSE2) || defined(XMM_KAZE_SSE4) || defined(XMM_KAZE_AVX)
#ifdef XMM_KAZE_SSE2
#include <emmintrin.h> //SSE2
#endif
#ifdef XMM_KAZE_SSE4
#include <smmintrin.h> //SSE4.1
#endif
#ifdef XMM_KAZE_AVX
#include <immintrin.h> //AVX
#endif

typedef union
{
    __m128i v;
    int32_t a[4];
} U128;

#define xmmload(p) _mm_load_si128((U128 const*)(p))
#define xmmloadu(p) _mm_loadu_si128((U128 const*)(p))
#define _rotl_KAZE128(x, n) _mm_or_si128(_mm_slli_si128(x, n) , _mm_srli_si128(x, 128-n))
#define XMM_KAZE_SSE2
uint32_t FNV1A_Hash_YoshimitsuTRIADiiXMM(const char *str, uint32_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    uint32_t hash32C = 2166136261;
    const char *p = str;
    uint32_t Loop_Counter;
    uint32_t Second_Line_Offset;

#if defined(XMM_KAZE_SSE2) || defined(XMM_KAZE_SSE4) || defined(XMM_KAZE_AVX)
    U128 xmm0;
    U128 xmm1;
    U128 xmm2;
    U128 xmm3;
    U128 xmm4;
    U128 xmm5;
    U128 hash32xmm;
    hash32xmm.v = _mm_set1_epi32(2166136261);
    U128 hash32Bxmm;
    hash32Bxmm.v = _mm_set1_epi32(2166136261);
    U128 hash32Cxmm;
    hash32Cxmm.v = _mm_set1_epi32(2166136261);
    U128 PRIMExmm;
    PRIMExmm.v = _mm_set1_epi32(709607);
#endif

#if defined(XMM_KAZE_SSE2) || defined(XMM_KAZE_SSE4) || defined(XMM_KAZE_AVX)
if (wrdlen >= 4*24) { // Actually 4*24 is the minimum and not useful, 200++ makes more sense.
    Loop_Counter = (wrdlen/(4*24));
    Loop_Counter++;
    Second_Line_Offset = wrdlen-(Loop_Counter)*(4*3*4);
    for(; Loop_Counter; Loop_Counter--, p += 4*3*sizeof(uint32_t)) {
    xmm0.v = xmmloadu(p+0*16);
    xmm1.v = xmmloadu(p+0*16+Second_Line_Offset);
    xmm2.v = xmmloadu(p+1*16);
    xmm3.v = xmmloadu(p+1*16+Second_Line_Offset);
    xmm4.v = xmmloadu(p+2*16);
    xmm5.v = xmmloadu(p+2*16+Second_Line_Offset);
#if defined(XMM_KAZE_SSE2)
        hash32xmm.v = _mm_mullo_epi16(_mm_xor_si128(hash32xmm.v , _mm_xor_si128(_rotl_KAZE128(xmm0.v,5) , xmm1.v)) , PRIMExmm.v);       
        hash32Bxmm.v = _mm_mullo_epi16(_mm_xor_si128(hash32Bxmm.v , _mm_xor_si128(_rotl_KAZE128(xmm3.v,5) , xmm2.v)) , PRIMExmm.v);        
        hash32Cxmm.v = _mm_mullo_epi16(_mm_xor_si128(hash32Cxmm.v , _mm_xor_si128(_rotl_KAZE128(xmm4.v,5) , xmm5.v)) , PRIMExmm.v);      
#else
        hash32xmm.v = _mm_mullo_epi32(_mm_xor_si128(hash32xmm.v , _mm_xor_si128(_rotl_KAZE128(xmm0.v,5) , xmm1.v)) , PRIMExmm.v);       
        hash32Bxmm.v = _mm_mullo_epi32(_mm_xor_si128(hash32Bxmm.v , _mm_xor_si128(_rotl_KAZE128(xmm3.v,5) , xmm2.v)) , PRIMExmm.v);        
        hash32Cxmm.v = _mm_mullo_epi32(_mm_xor_si128(hash32Cxmm.v , _mm_xor_si128(_rotl_KAZE128(xmm4.v,5) , xmm5.v)) , PRIMExmm.v);      
#endif
    }
#if defined(XMM_KAZE_SSE2)
    hash32xmm.v = _mm_mullo_epi16(_mm_xor_si128(hash32xmm.v , hash32Bxmm.v) , PRIMExmm.v);       
    hash32xmm.v = _mm_mullo_epi16(_mm_xor_si128(hash32xmm.v , hash32Cxmm.v) , PRIMExmm.v);       
#else
    hash32xmm.v = _mm_mullo_epi32(_mm_xor_si128(hash32xmm.v , hash32Bxmm.v) , PRIMExmm.v);       
    hash32xmm.v = _mm_mullo_epi32(_mm_xor_si128(hash32xmm.v , hash32Cxmm.v) , PRIMExmm.v);       
#endif
    hash32 = (hash32 ^ hash32xmm.a[0]) * PRIME;
    hash32B = (hash32B ^ hash32xmm.a[3]) * PRIME;
    hash32 = (hash32 ^ hash32xmm.a[1]) * PRIME;
    hash32B = (hash32B ^ hash32xmm.a[2]) * PRIME;
} else if (wrdlen >= 24)
#else
if (wrdlen >= 24)
#endif
{
    Loop_Counter = (wrdlen/24);
    Loop_Counter++;
    Second_Line_Offset = wrdlen-(Loop_Counter)*(3*4);
    for(; Loop_Counter; Loop_Counter--, p += 3*sizeof(uint32_t)) {
        hash32 = (hash32 ^ (ROL(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+0+Second_Line_Offset))) * PRIME;        
        hash32B = (hash32B ^ (ROL(*(uint32_t *)(p+4+Second_Line_Offset),5) ^ *(uint32_t *)(p+4))) * PRIME;        
        hash32C = (hash32C ^ (ROL(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+8+Second_Line_Offset))) * PRIME;        
    }
        hash32 = (hash32 ^ ROL(hash32C,5) ) * PRIME;
} else {
    // 1111=15; 10111=23
    if (wrdlen & 4*sizeof(uint32_t)) {  
        hash32 = (hash32 ^ (ROL(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
        hash32B = (hash32B ^ (ROL(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
        p += 8*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
        hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
        hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
        p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
        hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
        hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
        p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;
}
    hash32 = (hash32 ^ ROL(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}
#endif // defined(XMM_KAZE_SSE2) || defined(XMM_KAZE_SSE4) || defined(XMM_KAZE_AVX)

// "There it now stands for ever. Black on white.
// I can't get away from it. Ahoy, Yorikke, ahoy, hoy, ho!
// Go to hell now if you wish. What do I care? It's all the same now to me.
// I am part of you now. Where you go I go, where you leave I leave, when you go to the devil I go. Married.
// Vanished from the living. Damned and doomed. Of me there is not left a breath in all the vast world.
// Ahoy, Yorikke! Ahoy, hoy, ho!
// I am not buried in the sea,
// The death ship is now part of me
// So far from sunny New Orleans
// So far from lovely Louisiana."
// /An excerpt from 'THE DEATH SHIP - THE STORY OF AN AMERICAN SAILOR' by B.TRAVEN/
// 
// "Walking home to our good old Yorikke, I could not help thinking of this beautiful ship, with a crew on board that had faces as if they were seeing ghosts by day and by night.
// Compared to that gilded Empress, the Yorikke was an honorable old lady with lavender sachets in her drawers.
// Yorikke did not pretend to anything she was not. She lived up to her looks. Honest to her lowest ribs and to the leaks in her bilge.
// Now, what is this? I find myself falling in love with that old jane.
// All right, I cannot pass by you, Yorikke; I have to tell you I love you. Honest, baby, I love you.
// I have six black finger-nails, and four black and green-blue nails on my toes, which you, honey, gave me when necking you.
// Grate-bars have crushed some of my toes. And each finger-nail has its own painful story to tell.
// My chest, my back, my arms, my legs are covered with scars of burns and scorchings.
// Each scar, when it was being created, caused me pains which I shall surely never forget.
// But every outcry of pain was a love-cry for you, honey.
// You are no hypocrite. Your heart does not bleed tears when you do not feel heart-aches deeply and truly.
// You do not dance on the water if you do not feel like being jolly and kicking chasers in the pants.
// Your heart never lies. It is fine and clean like polished gold. Never mind the rags, honey dear.
// When you laugh, your whole soul and all your body is laughing.
// And when you weep, sweety, then you weep so that even the reefs you pass feel like weeping with you.
// I never want to leave you again, honey. I mean it. Not for all the rich and elegant buckets in the world.
// I love you, my gypsy of the sea!"
// /An excerpt from 'THE DEATH SHIP - THE STORY OF AN AMERICAN SAILOR' by B.TRAVEN/
uint32_t FNV1A_Hash_Yorikke(const char *str, size_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    const char *p = str;

    for(; wrdlen >= 2*2*sizeof(uint32_t); wrdlen -= 2*2*sizeof(uint32_t), p += 2*2*sizeof(uint32_t)) {
        hash32 = (hash32 ^ (ROL(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
        hash32B = (hash32B ^ (ROL(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
    }

    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
        hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
        hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
        p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
        hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
        hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
        p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;

    hash32 = (hash32 ^ ROL(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}
uint32_t FNV1A_Hash_YoshimitsuTRIAD(const char *str, size_t wrdlen)
{
    const uint32_t PRIME = 709607;
    uint32_t hash32 = 2166136261;
    uint32_t hash32B = 2166136261;
    uint32_t hash32C = 2166136261;
    //uint32_t hash32D = 2166136261;
    const char *p = str;

    for(; wrdlen >= 3*2*sizeof(uint32_t); wrdlen -= 3*2*sizeof(uint32_t), p += 3*2*sizeof(uint32_t)) {
        hash32 = (hash32 ^ (ROL(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
        hash32B = (hash32B ^ (ROL(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
        hash32C = (hash32C ^ (ROL(*(uint32_t *)(p+16),5) ^ *(uint32_t *)(p+20))) * PRIME;        
        //hash32D = (hash32D ^ (ROL(*(uint32_t *)(p+24),5) ^ *(uint32_t *)(p+28))) * PRIME;        

/*
// Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 16.00.30319.01 for 80x86 gave this:
// 12d-0f4+2= 59 bytes, No CARAMBA anymore.
$LL9@FNV1A_Hash@2:

; 160  :        hash32 = (hash32 ^ (_rotl(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        

  000f4 8b 01        mov     eax, DWORD PTR [ecx]
  000f6 c1 c0 05     rol     eax, 5
  000f9 33 41 04     xor     eax, DWORD PTR [ecx+4]
  000fc 83 eb 18     sub     ebx, 24            ; 00000018H
  000ff 33 f0        xor     esi, eax

; 161  :        hash32B = (hash32B ^ (_rotl(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        

  00101 8b 41 08     mov     eax, DWORD PTR [ecx+8]
  00104 69 f6 e7 d3 0a
    00       imul    esi, 709607        ; 000ad3e7H
  0010a c1 c0 05     rol     eax, 5
  0010d 33 41 0c     xor     eax, DWORD PTR [ecx+12]
  00110 83 c1 18     add     ecx, 24            ; 00000018H
  00113 33 f8        xor     edi, eax

; 162  :        hash32C = (hash32C ^ (_rotl(*(uint32_t *)(p+16),5) ^ *(uint32_t *)(p+20))) * PRIME;        

  00115 8b 41 f8     mov     eax, DWORD PTR [ecx-8]
  00118 69 ff e7 d3 0a
    00       imul    edi, 709607        ; 000ad3e7H
  0011e c1 c0 05     rol     eax, 5
  00121 33 41 fc     xor     eax, DWORD PTR [ecx-4]
  00124 33 e8        xor     ebp, eax
  00126 69 ed e7 d3 0a
    00       imul    ebp, 709607        ; 000ad3e7H
  0012c 4a       dec     edx
  0012d 75 c5        jne     SHORT $LL9@FNV1A_Hash@2
*/

/*
// Intel(R) C++ Compiler XE for applications running on IA-32, Version 12.1.1.258 Build 20111011 gave this:
// 216a-212f+2= 61 bytes, No CARAMBA anymore.
;;;     for(; wrdlen >= 3*2*sizeof(uint32_t); wrdlen -= 3*2*sizeof(uint32_t), p += 3*2*sizeof(uint32_t)) {

  02127 83 fa 18         cmp edx, 24                            
  0212a 72 43            jb .B4.5 ; Prob 10%                    
                                ; LOE eax edx ecx ebx ebp esi edi
.B4.2:                          ; Preds .B4.1
  0212c 89 34 24         mov DWORD PTR [esp], esi               ;
                                ; LOE eax edx ecx ebx ebp edi
.B4.3:                          ; Preds .B4.2 .B4.3

;;;         hash32 = (hash32 ^ (ROL(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        

  0212f 8b 31            mov esi, DWORD PTR [ecx]               
  02131 83 c2 e8         add edx, -24                           
  02134 c1 c6 05         rol esi, 5                             
  02137 33 71 04         xor esi, DWORD PTR [4+ecx]             
  0213a 33 de            xor ebx, esi                           

;;;         hash32B = (hash32B ^ (ROL(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        

  0213c 8b 71 08         mov esi, DWORD PTR [8+ecx]             
  0213f c1 c6 05         rol esi, 5                             
  02142 33 71 0c         xor esi, DWORD PTR [12+ecx]            
  02145 33 fe            xor edi, esi                           

;;;         hash32C = (hash32C ^ (ROL(*(uint32_t *)(p+16),5) ^ *(uint32_t *)(p+20))) * PRIME;        

  02147 8b 71 10         mov esi, DWORD PTR [16+ecx]            
  0214a c1 c6 05         rol esi, 5                             
  0214d 33 71 14         xor esi, DWORD PTR [20+ecx]            
  02150 83 c1 18         add ecx, 24                            
  02153 33 ee            xor ebp, esi                           
  02155 69 db e7 d3 0a 
        00               imul ebx, ebx, 709607                  
  0215b 69 ff e7 d3 0a 
        00               imul edi, edi, 709607                  
  02161 69 ed e7 d3 0a 
        00               imul ebp, ebp, 709607                  
  02167 83 fa 18         cmp edx, 24                            
  0216a 73 c3            jae .B4.3 ; Prob 82%                   
                                ; LOE eax edx ecx ebx ebp edi
.B4.4:                          ; Preds .B4.3
  0216c 8b 34 24         mov esi, DWORD PTR [esp]               ;
                                ; LOE eax edx ecx ebx ebp esi edi
.B4.5:                          ; Preds .B4.1 .B4.4
*/

    }
    if (p != str) {
        hash32 = (hash32 ^ ROL(hash32C,5) ) * PRIME;
        //hash32B = (hash32B ^ ROL(hash32D,5) ) * PRIME;
    }

    // 1111=15; 10111=23
    if (wrdlen & 4*sizeof(uint32_t)) {  
        hash32 = (hash32 ^ (ROL(*(uint32_t *)(p+0),5) ^ *(uint32_t *)(p+4))) * PRIME;        
        hash32B = (hash32B ^ (ROL(*(uint32_t *)(p+8),5) ^ *(uint32_t *)(p+12))) * PRIME;        
        p += 8*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7,...,15
    if (wrdlen & 2*sizeof(uint32_t)) {
        hash32 = (hash32 ^ *(uint32_t*)(p+0)) * PRIME;
        hash32B = (hash32B ^ *(uint32_t*)(p+4)) * PRIME;
        p += 4*sizeof(uint16_t);
    }
    // Cases: 0,1,2,3,4,5,6,7
    if (wrdlen & sizeof(uint32_t)) {
        hash32 = (hash32 ^ *(uint16_t*)(p+0)) * PRIME;
        hash32B = (hash32B ^ *(uint16_t*)(p+2)) * PRIME;
        p += 2*sizeof(uint16_t);
    }
    if (wrdlen & sizeof(uint16_t)) {
        hash32 = (hash32 ^ *(uint16_t*)p) * PRIME;
        p += sizeof(uint16_t);
    }
    if (wrdlen & 1) 
        hash32 = (hash32 ^ *p) * PRIME;

    hash32 = (hash32 ^ ROL(hash32B,5) ) * PRIME;
    return hash32 ^ (hash32 >> 16);
}
