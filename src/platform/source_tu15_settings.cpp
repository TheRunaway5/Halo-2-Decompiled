#include "halo2/source_tu15_settings.hpp"
#include <fstream>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[p+i])<<(8*i);
    return value;
}
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) { return word(m.view(p,4),0); }
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t value,
    const SourcePhysicsContactRecordObserver &observe,unsigned width=4) {
    const auto old=m.view(p,width);
    SourcePhysicsContactRecordStore change{0x2e101c,p,{old.begin(),old.end()},std::vector<std::byte>(width)};
    for(unsigned i=0;i<width;++i)change.written[i]=std::byte(value>>(8*i));
    m.write(p,change.written);if(observe)observe(change);
}
}
SourceTu15UserSettings::SourceTu15UserSettings(std::array<std::byte,96> bytes,std::string owner)
    :bytes_(bytes),owner_(std::move(owner)) {
    if(owner_.empty())throw std::invalid_argument("TU1.5 settings require an actual named resource");
}
SourceTu15UserSettings SourceTu15UserSettings::load(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in || in.tellg()!=256)throw std::runtime_error("TU1.5 settings require the actual 256-byte EEPROM resource");
    std::array<std::byte,96> bytes;in.seekg(96);
    if(!in.read(reinterpret_cast<char *>(bytes.data()),bytes.size()))throw std::runtime_error("Truncated TU1.5 EEPROM UserSettings");
    return {bytes,path.string()+" UserSettings at96,96"};
}
bool SourceTu15UserSettings::checksum_valid() const {
    std::uint64_t checksum=0;std::uint32_t carries=0;
    for(unsigned i=0;i<96;i+=4) {
        checksum+=word(bytes_,i);
        if(checksum>0xffffffffu) {++carries;checksum&=0xffffffffu;}
    }
    checksum+=carries;if(checksum>0xffffffffu)++checksum;
    return std::uint32_t(checksum)==0xffffffffu;
}
std::uint32_t SourceTu15UserSettings::miscellaneous_flags() const { return word(bytes_,88); }
std::uint32_t SourceTu15UserSettings::audio_flags() const { return word(bytes_,56); }
bool source_try_tu15_settings_query(SourceRuntimeMemory &m,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &abi,
    const SourceTu15UserSettings &settings,const SourcePhysicsContactRecordObserver &observe) {
    if(call.instruction!=0x2e101c || call.target!=0x80000018)return false;
    const auto sp=abi.gp[4],bp=abi.gp[5];m.view(sp,24);m.view(bp,28);
    if(!call.tail || word(m,sp)!=0x2e0ff9 || sp!=bp-24 || word(m,sp+4)!=17 || word(m,sp+16)!=4)
        throw std::runtime_error("TU1.5 settings query requires its actual wrapper frame");
    for(unsigned i=1;i<=5;++i)
        if(word(m,sp+i*4)!=word(m,bp+4+i*4))throw std::runtime_error("TU1.5 settings query arguments lost their source owner");
    const auto type=word(m,sp+8),value=word(m,sp+12),length=word(m,sp+20);
    m.mutable_view(length,4);
    const bool valid=settings.checksum_valid();
    if(valid) {m.mutable_view(type,4);m.mutable_view(value,4);}
    put(m,length,4,observe);
    if(valid) {
        put(m,type,4,observe);
        for(unsigned i=0;i<4;++i)put(m,value+i,0,observe,1);
        for(unsigned i=0;i<4;++i)put(m,value+i,settings.miscellaneous_flags()>>(8*i),observe,1);
    }
    // Actual branches of the pinned ExQueryNonVolatileSetting expression.
    // These are query results, never supplied settings-success defaults.
    abi.gp[0]=valid?0u:0xc000009cu;abi.gp[4]=sp+24;
    return true;
}
bool source_try_tu15_audio_settings_query(SourceRuntimeMemory &m,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &abi,
    const SourceTu15UserSettings &settings,const SourcePhysicsContactRecordObserver &observe) {
    if(call.instruction!=0x2e101cu || call.target!=word(m,0x41b6f0u))return false;
    const auto sp=abi.gp[4],bp=abi.gp[5];m.view(sp,36);
    if(!call.tail || word(m,sp)!=0x37dab2u || sp!=bp-32u || word(m,sp+4u)!=9u ||
       word(m,sp+8u)!=bp-8u || word(m,sp+12u)!=bp-4u || word(m,sp+16u)!=4u || word(m,sp+20u)!=0u)
        throw std::runtime_error("TU1.5 audio settings query requires its actual37DA9A frame");
    const auto type=bp-8u,value=bp-4u;const bool valid=settings.checksum_valid();
    if(valid) {
        // Validate every actual output before the first publication. The
        // null ResultLength receives no store, including on checksum failure.
        m.mutable_view(type,4);m.mutable_view(value,4);
        put(m,type,4,observe);
        for(unsigned i=0;i<4;++i)put(m,value+i,0,observe,1);
        const auto flags=settings.audio_flags();
        for(unsigned i=0;i<4;++i)put(m,value+i,flags>>(8*i),observe,1);
    }
    abi.gp[0]=valid?0u:0xc000009cu;abi.gp[4]=sp+24u;return true;
}
} // namespace halo2
