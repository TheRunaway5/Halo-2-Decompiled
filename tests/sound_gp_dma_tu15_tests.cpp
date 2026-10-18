#include "halo2/sound_gp_dma_tu15.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
using namespace halo2;
using Space=SoundGpSpaceTu15;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F action,const char* message){
    bool caught=false;try{action();}catch(const std::exception&){caught=true;}
    if(!caught)throw std::runtime_error(message);
}
struct Fixture {
    std::array<std::uint32_t,0x1000> x{},p{};
    std::array<std::uint32_t,0x800> y{};
    std::vector<std::byte> scratch=std::vector<std::byte>(0x10000);
    unsigned reads{},writes{};
    std::uint32_t& word(Space space,std::uint32_t at){
        if(space==Space::program)return p.at(at);
        if(space==Space::y)return y.at(at);
        if(at>=0x1400&&at<0x1800)at-=0x800;
        return x.at(at);
    }
    SoundGpDmaResourcesTu15 resources(SoundGpDmaFormat2Tu15 format2=SoundGpDmaFormat2Tu15::unverified){
        return {[this](auto space,auto at){return word(space,at);},
            [this](auto space,auto at,auto value){word(space,at)=value;},
            [this](auto at,auto bytes){check(at<=scratch.size()&&bytes.size()<=scratch.size()-at,"Scratch read exceeds retained input");
                std::copy_n(scratch.begin()+at,bytes.size(),bytes.begin());++reads;},
            [this](auto at,auto bytes){check(at<=scratch.size()&&bytes.size()<=scratch.size()-at,"Scratch write exceeds retained input");
                std::copy(bytes.begin(),bytes.end(),scratch.begin()+at);++writes;},format2};
    }
    void packet(std::uint32_t control,std::uint32_t count,std::uint32_t data,
                std::uint32_t offset,std::uint32_t base=0,std::uint32_t last=0){
        const std::array words{0x4025u,control,count,data,offset,base,last};
        std::copy(words.begin(),words.end(),x.begin()+0x25);
    }
    void put(std::uint32_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)scratch.at(at+i)=std::byte(value>>(8*i));}
    std::uint32_t get(std::uint32_t at)const{std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(scratch.at(at+i))<<(8*i);return v;}
    static SoundGpDmaStateTu15 initial(){return {0,0x20,0x25,0x25,false,false};}
};
void transfers(){
    {
        Fixture f;f.packet(0x59e0,6,0,0x800);
        const std::array header{0x8000u,0x800u,0x818u,0x3f98u,3u,0xffffffu};
        for(unsigned i=0;i<header.size();++i)f.put(0x800+4*i,header[i]);
        SoundGpDmaTu15 dma(f.resources(),f.initial());dma.write(0xffffd6,1);
        check(std::equal(header.begin(),header.end(),f.x.begin()),"Original header input changed its24-bit words");
        check(f.get(0x810)==3&&f.x[4]==3,"DMA supplied a processor acknowledgement");
        check(f.reads==1&&f.writes==0&&dma.state()==SoundGpDmaStateTu15{0,0x10,0x25,0x4025,true,false},"Header completion did not retain actual list state");
        dma.acknowledge_eol();check(!dma.state().eol,"EOL acknowledge did not retire actual completion");
        dma.write(0xffffd6,2);dma.write(0xffffd6,3);check(dma.read(0xffffd6)==0x28,"STOP/FREEZE status differs");
        rejects([&]{dma.write(0xffffd6,1);},"Frozen DMA started");
        dma.write(0xffffd6,4);check(dma.read(0xffffd6)==0x20,"UNFREEZE status differs");
    }
    for(auto control:{0x59e2u,0x49e2u}){
        Fixture f;f.packet(control,4,0x1800,0x900);
        f.y[0]=0x123456;f.y[1]=0x800000;f.y[2]=0xffffff;f.y[3]=0;
        SoundGpDmaTu15 dma(f.resources(SoundGpDmaFormat2Tu15::declared_high24),f.initial());dma.write(0xffffd6,1);
        for(unsigned i=0;i<4;++i)check(f.get(0x900+4*i)==(control==0x49e2?f.y[i]<<8:f.y[i]),"DMA output padding or declared high/low24-bit placement differs");
        check(f.x[0x29]==0x900&&f.writes==1,"Linear packet changed its source offset");
    }
    {
        Fixture f;f.packet(0x59e0,0x100,0x2ff0,0x1000);
        for(unsigned i=0;i<0x100;++i)f.put(0x1000+4*i,0xfedc00+i);
        SoundGpDmaTu15 dma(f.resources(),f.initial());dma.write(0xffffd6,1);
        check(f.p[0x7f0]==0xfedc00&&f.p[0x8ef]==0xfedcff,"DMA program transfer lost its physical P extent");
    }
    for(auto control:{0x59d2u,0x4bd2u}){
        Fixture f;f.packet(control,32,0x1400,0x7c0,0x8000,0x7ff);
        for(unsigned i=0;i<32;++i)f.x[0xc00+i]=0xff0000+i;
        SoundGpDmaTu15 dma(f.resources(SoundGpDmaFormat2Tu15::declared_high24),f.initial());dma.write(0xffffd6,1);
        check(f.x[0x29]==0x40&&f.writes==2,"Circular output did not wrap/write back its actual cursor");
        for(unsigned i=0;i<32;++i)check(f.get(0x8000+((0x7c0+i*4)&0x7ff))==(control==0x4bd2?f.x[0xc00+i]<<8:f.x[0xc00+i]),"Circular mix-alias bytes differ");
    }
    {
        Fixture f;f.packet(0x4bd0,32,0x1810,0x7c0,0x8000,0x7ff);
        for(unsigned i=0;i<32;++i)f.put(0x8000+((0x7c0+i*4)&0x7ff),((0x800000+i)<<8)|0x5a);
        SoundGpDmaTu15 dma(f.resources(SoundGpDmaFormat2Tu15::declared_high24),f.initial());dma.write(0xffffd6,1);
        check(f.reads==2&&f.x[0x29]==0x40,"Circular input did not retain its wrap/cursor");
        for(unsigned i=0;i<32;++i)check(f.y[0x10+i]==0x800000+i,"Circular input did not discard only its low padding byte");
    }
    {
        Fixture f;f.packet(0x59e2,1,0x1800,0x900);f.y[0]=0x654321;f.x[0x25]=0x40;
        const std::array second{0x4040u,0x59e0u,1u,0x1801u,0x900u};std::copy(second.begin(),second.end(),f.x.begin()+0x40);
        SoundGpDmaTu15 dma(f.resources(),f.initial());dma.write(0xffffd6,1);
        check(f.y[1]==0x654321&&dma.state().next==0x4040&&f.reads==1&&f.writes==1,"Linked DMA packet order or final pointer differs");
    }
}
void failures(){
    rejects([]{SoundGpDmaTu15 dma({},Fixture::initial());},"Absent DMA resources accepted");
    Fixture f;
    {auto io=f.resources();io.format2=static_cast<SoundGpDmaFormat2Tu15>(99);
        rejects([&]{SoundGpDmaTu15 dma(io,f.initial());},"Invalid DMA format2 declaration accepted");}
    for(auto initial:{SoundGpDmaStateTu15{1,0x20,0,0,false,false},SoundGpDmaStateTu15{0,0x30,0,0,false,false},SoundGpDmaStateTu15{0,0x20,0,0,false,true}})
        rejects([&]{SoundGpDmaTu15 dma(f.resources(),initial);},"Unverified DMA initial context accepted");
    SoundGpDmaTu15 dma(f.resources(),f.initial());
    rejects([&]{dma.write(0xffffd7,1);},"Unverified DMA configuration accepted");
    rejects([&]{dma.write(0xffffd6,5);},"Unverified DMA command accepted");
    rejects([&]{dma.write(0xffffd4,0x8000);},"Wide DMA next pointer accepted");
    rejects([&]{dma.read(0xffffd4);},"Unverified DMA read accepted");
    bool foreign=false;std::thread other([&]{try{dma.state();}catch(const std::exception&){foreign=true;}});other.join();check(foreign,"Foreign DMA executor accepted");
    check(dma.state()==f.initial(),"Rejected operations changed the retained DMA context");
    for(auto control:{0x49e2u,0x4bd0u,0x4bd2u}){
        Fixture bad;bad.packet(control,1,0x1800,0,0x8000,0x7ff);SoundGpDmaTu15 owner(bad.resources(),bad.initial());
        rejects([&]{owner.write(0xffffd6,1);},"Unresolved format2 silently selected a layout");
        check(!bad.reads&&!bad.writes,"Unresolved format2 transferred scratch bytes");
    }
    for(auto control:{0x59e1u,0x59c0u,0x19e0u}){
        Fixture bad;bad.packet(control,1,0x1800,0);SoundGpDmaTu15 owner(bad.resources(),bad.initial());
        rejects([&]{owner.write(0xffffd6,1);},"Unverified packet semantics executed");
        check(!bad.reads&&!bad.writes,"Rejected control transferred scratch bytes");
        rejects([&]{owner.state();},"Faulted transfer owner resumed");
    }
    for(auto address:{0x1000u,0x2000u,0x3800u}){
        Fixture bad;bad.packet(0x59e0,1,address,0);SoundGpDmaTu15 owner(bad.resources(),bad.initial());
        rejects([&]{owner.write(0xffffd6,1);},"Unmapped GP RAM transfer executed");
        check(!bad.reads&&!bad.writes,"Unmapped GP RAM consumed scratch data before preflight");
    }
    {
        Fixture bad;bad.packet(0x59e0,2,0x1fff,0);SoundGpDmaTu15 owner(bad.resources(),bad.initial());
        rejects([&]{owner.write(0xffffd6,1);},"Crossing Y extent executed");check(!bad.reads,"Crossing extent read scratch");
    }
    {
        Fixture bad;bad.packet(0x59d2,32,0x1400,0x800,0x8000,0x7ff);SoundGpDmaTu15 owner(bad.resources(),bad.initial());
        rejects([&]{owner.write(0xffffd6,1);},"Unverified circular initial cursor executed");check(!bad.writes,"Invalid circular cursor wrote scratch");
    }
    {
        Fixture bad;bad.packet(0x59e2,1,0x1800,0);bad.x[0x25]=0x25;SoundGpDmaTu15 owner(bad.resources(),bad.initial());
        rejects([&]{owner.write(0xffffd6,1);},"Cyclic DMA list completed");check(bad.writes==1,"Fault diagnostic lost actual prior transfer");
        rejects([&]{owner.acknowledge_eol();},"Faulted DMA supplied EOL acknowledgement");
    }
    {
        Fixture bad;bad.packet(0x59e0,1,0x1800,0);SoundGpDmaTu15* active=nullptr;auto io=bad.resources();
        io.read_scratch=[&](auto,auto){active->write(0xffffd6,2);};SoundGpDmaTu15 owner(std::move(io),bad.initial());active=&owner;
        rejects([&]{owner.write(0xffffd6,1);},"DMA callback reentered its owner");
        rejects([&]{owner.write(0xffffd6,2);},"Callback fault did not permanently stop DMA use");
    }
}
}
int main()try{
    transfers();failures();
    std::cout<<"PASS native GP scratch DMA: six observed controls, linked order, P/Y/mix placement, padding, circular wrap/cursors, actual EOL, ownership and persistent faults\n";
    std::cout<<"LIMIT synchronous transfer contract; hardware clock/reset/IRQ and original-program/real-SGE composition are separate gates\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
