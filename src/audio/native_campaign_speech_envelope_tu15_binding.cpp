#include "halo2/native_campaign_speech_envelope_tu15_binding.hpp"
#include "halo2/native_campaign_speech_envelope_tu15.hpp"
#include <bit>
namespace halo2 { namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
}
}
void native_campaign_speech_envelope_tu15_run(SourcePhysicsContactMemory &memory,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fallback) {
    const auto policy=abi.floating_point.value_or(fallback);
    NativeCampaignAudioFp fp{policy.x87_control_word,abi.x87_status,policy.mxcsr};
    struct Publication {
        SourcePhysicsContactRuntimeAbi &abi;NativeCampaignAudioFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
    } publication{abi,fp};
    const auto sp=abi.gp[4];
    const auto sample=std::bit_cast<std::int16_t>(std::uint16_t(read(memory,sp+4u)));
    const auto range=std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[2]));
    const auto permutation=std::bit_cast<std::int16_t>(std::uint16_t(abi.gp[1]));
    NativeCampaignSpeechEnvelopeSampleTu15 result;
    try {result=native_campaign_speech_envelope_tu15(memory,abi.gp[0],range,permutation,sample,fp);}
    catch(const NativeCampaignSpeechEnvelopePendingErrorTu15 &error){abi.xmm[0][0]=error.converted_sample_bits();throw;}
    // CVTSI2SS/MULSS retain the incoming upper lanes for an actual sample.
    // The refusal branch's XORPS clears all four lanes, including zero-byte
    // valid samples being observably different from an absent envelope.
    if(result.present)abi.xmm[0][0]=result.bits;
    else abi.xmm[0].fill(0);
    (void)read(memory,sp);abi.gp[4]=sp+8u;
}
}
