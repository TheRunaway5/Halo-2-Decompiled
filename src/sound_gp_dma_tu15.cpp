#include "halo2/sound_gp_dma_tu15.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace halo2 {
struct SoundGpDmaTu15::Impl {
    SoundGpDmaResourcesTu15 io;
    SoundGpDmaStateTu15 registers;
    const std::thread::id thread=std::this_thread::get_id();
    bool busy{},fault{};
    static constexpr std::uint32_t frozen=8,running=16,stopped=32,eol=0x4000;
    explicit Impl(SoundGpDmaResourcesTu15 resources,SoundGpDmaStateTu15 initial)
        :io(std::move(resources)),registers(initial){
        if(!io.read_word||!io.write_word||!io.read_scratch||!io.write_scratch)
            throw std::invalid_argument("GP DMA requires actual word and scratch owners");
        if(io.format2!=SoundGpDmaFormat2Tu15::unverified&&
           io.format2!=SoundGpDmaFormat2Tu15::declared_low24&&
           io.format2!=SoundGpDmaFormat2Tu15::declared_high24)
            throw std::invalid_argument("GP DMA format2 declaration is invalid");
        if(initial.configuration||initial.control&~(frozen|running|stopped)||
           (initial.control&running&&initial.control&stopped)||initial.start>0x7fff||initial.next>0x7fff||initial.error)
            throw std::invalid_argument("GP DMA initial context is outside the verified source domain");
    }
    void require_owner()const{
        if(std::this_thread::get_id()!=thread)throw std::runtime_error("GP DMA requires its creating executor");
        if(fault)throw std::runtime_error("GP DMA owner is faulted");
        if(busy)throw std::runtime_error("GP DMA cannot re-enter its transfer owner");
    }
    std::uint32_t x(std::uint32_t at)const{
        if(at>=0xc00)throw std::out_of_range("GP DMA descriptor is outside its verified X-node region");
        const auto value=io.read_word(SoundGpSpaceTu15::x,at);
        if(value>0xffffff)throw std::invalid_argument("GP DMA word exceeds24 bits");
        return value;
    }
    struct Address {SoundGpSpaceTu15 space;std::uint32_t word,limit;};
    static Address address(std::uint32_t encoded){
        if(encoded<0x1800)return{SoundGpSpaceTu15::x,encoded,0x1800};
        if(encoded<0x2000)return{SoundGpSpaceTu15::y,encoded-0x1800,0x800};
        if(encoded>=0x2800&&encoded<0x3800)return{SoundGpSpaceTu15::program,encoded-0x2800,0x1000};
        throw std::out_of_range("GP DMA data address has no physical GP RAM space");
    }
    void transfer(std::uint32_t node){
        const auto control=x(node+1),count=x(node+2);const auto data=address(x(node+3));
        // Observed original source packets. Bit9 is retained as opaque
        // source metadata: this owner asserts no notification/IRQ meaning.
        switch(control){
        case 0x59e0:case 0x59e2:case 0x59d2:case 0x49e2:case 0x4bd0:case 0x4bd2:break;
        default:throw std::invalid_argument("Original GP DMA control is outside the verified scratch packet domain");
        }
        if(!count||count>data.limit-data.word)
            throw std::out_of_range("GP DMA sample extent exceeds its physical RAM space");
        const bool output=(control&2)!=0,circular=((control>>5)&15)==14,format2=((control>>10)&7)==2;
        if(format2&&io.format2==SoundGpDmaFormat2Tu15::unverified)
            throw std::runtime_error("GP DMA format2 byte placement remains unverified");
        const bool msb=format2&&io.format2==SoundGpDmaFormat2Tu15::declared_high24;
        const auto offset=x(node+4);const auto base=circular?x(node+5):0u;
        const std::uint64_t capacity=circular?std::uint64_t(x(node+6))+1:0;
        if(circular&&offset>=capacity)throw std::out_of_range("GP DMA circular start is outside the verified interval");
        const auto bytes=std::size_t(count)*4;
        if(!circular&&std::uint64_t(offset)+bytes>std::uint64_t(std::numeric_limits<std::uint32_t>::max())+1)
            throw std::out_of_range("GP DMA linear scratch extent overflows its address");
        // Read/preflight actual GP RAM before starting this node's data I/O.
        // A callback failure retains partial external effects diagnostically;
        // no rollback or successful source completion is manufactured.
        std::vector<std::byte> payload(bytes);
        for(std::uint32_t i=0;i<count;++i){
            const auto value=io.read_word(data.space,data.word+i);
            if(value>0xffffff)throw std::invalid_argument("GP DMA RAM source exceeds24 bits");
            if(output){const auto encoded=msb?value<<8:value;
                for(unsigned b=0;b<4;++b)payload[i*4+b]=std::byte(encoded>>(8*b));}
        }
        std::size_t consumed{};std::uint64_t cursor=offset;
        while(consumed<bytes){
            const auto chunk=circular?std::min<std::uint64_t>(bytes-consumed,capacity-cursor):bytes-consumed;
            const auto physical=std::uint64_t(base)+cursor;
            if(physical>0xffffffffu||chunk>0x100000000ull-physical)
                throw std::out_of_range("GP DMA scratch chunk overflows its actual address");
            auto part=std::span(payload).subspan(consumed,std::size_t(chunk));
            if(output)io.write_scratch(std::uint32_t(physical),part);
            else io.read_scratch(std::uint32_t(physical),part);
            consumed+=std::size_t(chunk);cursor+=chunk;if(circular&&cursor==capacity)cursor=0;
        }
        if(!output)for(std::uint32_t i=0;i<count;++i){
            std::uint32_t value{};for(unsigned b=0;b<4;++b)value|=std::to_integer<std::uint32_t>(payload[i*4+b])<<(8*b);
            io.write_word(data.space,data.word+i,msb?value>>8:value&0xffffff);
        }
        if(control&16)io.write_word(SoundGpSpaceTu15::x,node+4,std::uint32_t(cursor));
    }
    void execute(){
        std::array<bool,0xc00> visited{};
        while(!(registers.next&eol)){
            const auto node=registers.next;
            if(node>=visited.size()||visited[node])throw std::runtime_error("GP DMA list has an unsupported node or cycle");
            visited[node]=true;const auto next=x(node);
            if(next>0x7fff)throw std::invalid_argument("GP DMA next pointer has unsupported bits");
            transfer(node);registers.next=next;
        }
        registers.eol=true;
    }
};
SoundGpDmaTu15::SoundGpDmaTu15(SoundGpDmaResourcesTu15 resources,SoundGpDmaStateTu15 state)
    :impl_(std::make_unique<Impl>(std::move(resources),state)){}
SoundGpDmaTu15::~SoundGpDmaTu15()=default;
std::uint32_t SoundGpDmaTu15::read(std::uint32_t at)const{
    auto& s=*impl_;s.require_owner();
    if(at!=0xffffd6)throw std::invalid_argument("GP DMA read is outside the observed status register");
    return s.registers.control;
}
SoundGpDmaStateTu15 SoundGpDmaTu15::state()const{impl_->require_owner();return impl_->registers;}
void SoundGpDmaTu15::acknowledge_eol(){impl_->require_owner();impl_->registers.eol=false;}
void SoundGpDmaTu15::write(std::uint32_t at,std::uint32_t value){
    auto& s=*impl_;s.require_owner();if(value>0xffffff)throw std::invalid_argument("GP DMA register value exceeds24 bits");
    if(at==0xffffd7){if(value)throw std::invalid_argument("GP DMA configuration is not verified");s.registers.configuration=value;return;}
    if(at==0xffffd4||at==0xffffd5){if(value>0x7fff)throw std::invalid_argument("GP DMA node pointer has unsupported bits");
        (at==0xffffd4?s.registers.next:s.registers.start)=value;return;}
    if(at!=0xffffd6||value<1||value>4)throw std::invalid_argument("GP DMA command is not verified");
    switch(value){
    case 2:s.registers.control=(s.registers.control&~Impl::running)|Impl::stopped;s.registers.eol=false;return;
    case 3:s.registers.control|=Impl::frozen;return;
    case 4:s.registers.control&=~Impl::frozen;return;
    default:break;
    }
    if(s.registers.control&Impl::frozen)throw std::runtime_error("GP DMA start requires actual unfreeze");
    s.registers.control=(s.registers.control&~Impl::stopped)|Impl::running;s.busy=true;
    try{s.execute();s.busy=false;}
    catch(...){s.busy=false;s.fault=true;s.registers.error=true;throw;}
}
} // namespace halo2
