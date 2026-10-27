#pragma once
#include "halo2/original_loading_progress.hpp"
#include <array>
#include <charconv>
#include <string>
namespace halo2 {
struct OriginalLoadingWaitText {
    std::int32_t percent;
    std::array<std::int16_t,4> rectangle; // top,left,bottom,right.
    std::u16string text;
    unsigned font=3,justification=2;
    std::int32_t style=-1;
    std::uint32_t flags=0;
};
// TU1.5 229DB0 (retail2233A0): the universal utility-drive map-copy wait
// callback, reached by actual MP728B0/1916A0/191890/191EF0/166770. Progress,
// language and the current viewport remain explicit native platform inputs.
// This does not activate the separate campaign209 ring compositor or choose
// a dwell, completion signal, clock, audio policy or map percentage.
inline OriginalLoadingWaitText original_loading_wait_text(std::uint32_t progress_bits,
    unsigned language,std::array<std::int16_t,4> viewport,
    const OriginalLoadingFloatingPoint&context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    original_loading_detail::FloatingPointScope scope(context);
    const auto percent=_mm_cvttss_si32(_mm_mul_ss(
        _mm_set_ss(std::bit_cast<float>(progress_bits)),_mm_set_ss(std::bit_cast<float>(0x42c80000u))));
    // ADD WORD preserves the actual source wrap, independently of clipping.
    viewport[0]=std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(viewport[0])+200u));
    std::u16string text;
    switch(language) {
    case 1:text=u"\u8aad\u307f\u8fbc\u3093\u3067\u3044\u307e\u3059";break;
    case 2:text=u"LADEN";break;
    case 3:text=u"CHARGEMENT EN COURS";break;
    case 4:text=u"CARGANDO";break;
    case 5:text=u"CARICAMENTO";break;
    case 6:text=u"\ubd88\ub7ec\uc624\ub294 \uc911";break;
    case 7:text=u"\u8f09\u5165\u4e2d";break;
    default:text=u"LOADING";break;
    }
    std::array<char,12> decimal;const auto end=std::to_chars(decimal.data(),decimal.data()+decimal.size(),percent).ptr;
    text.push_back(u' ');for(const auto*p=decimal.data();p!=end;++p)text.push_back(char16_t(*p));text.push_back(u'%');
    if(text.size()>63)text.resize(63);
    return {percent,viewport,std::move(text)};
#else
    (void)progress_bits;(void)language;(void)viewport;(void)context;
    throw std::runtime_error("Original Xbox loading wait arithmetic backend unavailable");
#endif
}
}
