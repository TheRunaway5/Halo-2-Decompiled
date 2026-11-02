#include "halo2/player_screen_flash.hpp"
#include "halo2/native_campaign_screen_flash_sample_tu15.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace halo2 {
PlayerScreenFlash source_player_screen_flash_tu15(const SourcePhysicsContactMemory& source,
    std::uint32_t user,NativeTu15PhysicsFrameFp& fp) {
    if(user!=0xffffffffu && user>=4)throw std::invalid_argument("Native screen flash requires an actual local-user index");
    class Output final:public SourcePhysicsContactMemory {
        const SourcePhysicsContactMemory& source_;
    public:
        static constexpr std::uint32_t address(){return 0xffffffe0u;}
        std::array<std::byte,24> bytes{};
        explicit Output(const SourcePhysicsContactMemory& source):source_(source){}
        void read(std::uint32_t at,std::span<std::byte> out)const override {
            if(at>=address() && std::uint64_t(at)+out.size()<=std::uint64_t(address())+bytes.size())
                std::copy_n(bytes.begin()+at-address(),out.size(),out.begin());
            else source_.read(at,out);
        }
        void write(std::uint32_t at,std::span<const std::byte> in)override {
            if(at<address() || std::uint64_t(at)+in.size()>std::uint64_t(address())+bytes.size())
                throw std::runtime_error("Native flash sampling attempted a source-input mutation");
            std::copy(in.begin(),in.end(),bytes.begin()+at-address());
        }
    } output(source);
    native_campaign_screen_flash_sample_tu15(output,user,Output::address(),fp);
    const auto word=[&](unsigned at) {
        std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::uint32_t(std::to_integer<unsigned>(output.bytes[at+n]))<<(n*8u);
        return value;
    };
    PlayerScreenFlash result{std::bit_cast<std::int32_t>(word(0)),std::bit_cast<float>(word(4)),{}};
    for(unsigned n=0;n<4;++n)result.color[n]=std::bit_cast<float>(word(8+n*4));return result;
}
}
