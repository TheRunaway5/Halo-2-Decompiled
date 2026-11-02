#include "halo2/source_unit_speech_channel_symbols_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

using namespace halo2;
std::unique_ptr<SourceUnitSpeechChannelSymbolsTu15> speech_lease_from_peer(
    const SourcePhysicsContactMemory&,const SourceUnitSpeechChannelSymbolsTu15::Name&);
std::uint32_t speech_query_from_peer(const SourcePhysicsContactMemory&,std::uint32_t,std::uint32_t);
namespace {
struct Memory final:SourcePhysicsContactMemory {
    void read(std::uint32_t,std::span<std::byte>)const override {
        throw std::runtime_error("Namespace must not read authored source storage");
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::runtime_error("Namespace must not write authored source storage");
    }
};
unsigned controls;
void need(bool value){++controls;if(!value)throw std::runtime_error("Unit speech symbol ownership control failed");}
template<class F>void rejects(F&&f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}need(caught);}
// Exact cache names are independently checked against the retained actual
// Original/MCC maps in the associated data-only input proof. This component
// checks lifetime and operand admission, never supplies a graph/clip result.
std::string names(std::uint32_t id){
    if(id==0x08000149u)return "phonemes";
    if(id==0x0c00025bu)return "mouth_closed";
    throw std::runtime_error("Foreign name resolver operand");
}
constexpr std::array pairs{
    std::array{0x1102e9u,0x08000145u,0x08000149u},
    std::array{0x110314u,0x0c00024du,0x0c00025bu}};
void queries(const Memory&memory,bool bound){
    for(auto [pc,raw,translated]:pairs){
        need(source_unit_speech_channel_symbol_tu15(memory,pc,raw)==(bound?translated:raw));
        need(speech_query_from_peer(memory,pc,raw)==(bound?translated:raw));
        for(auto other:{0u,0x1102fau,0x110325u,0x1e53b5u,pc-1,pc+1})
            need(source_unit_speech_channel_symbol_tu15(memory,other,raw)==raw);
        for(auto other:{0u,0xffffffffu,raw^1u,raw^0x01000000u,translated})
            need(source_unit_speech_channel_symbol_tu15(memory,pc,other)==other);
    }
    // The other real speech query site cannot map the first query's literal.
    need(source_unit_speech_channel_symbol_tu15(memory,pairs[0][0],pairs[1][1])==pairs[1][1]);
    need(source_unit_speech_channel_symbol_tu15(memory,pairs[1][0],pairs[0][1])==pairs[0][1]);
}
}
int main()try{
    Memory a,b;queries(a,false);queries(b,false);
    {
        SourceUnitSpeechChannelSymbolsTu15 original(a,false,[](auto)->std::string{
            throw std::runtime_error("Original bank must not query the MCC namespace");});
        queries(a,false);queries(b,false);
    }
    rejects([&]{SourceUnitSpeechChannelSymbolsTu15 missing(a,true,{});});queries(a,false);
    for(auto wrong_id:{0x08000149u,0x0c00025bu}){
        rejects([&]{SourceUnitSpeechChannelSymbolsTu15 wrong(a,true,[&](auto id){
            return id==wrong_id?"foreign-cache-name":names(id);});});
        queries(a,false);
    }
    for(unsigned failure=1;failure<=2;++failure){
        unsigned calls=0;
        rejects([&]{SourceUnitSpeechChannelSymbolsTu15 failed(a,true,[&](auto id){
            if(++calls==failure)throw std::runtime_error("Resolver fault");
            return names(id);});});
        need(calls==failure);queries(a,false);
    }
    {
        std::vector<std::uint32_t> resolved;
        auto lease=speech_lease_from_peer(a,[&](auto id){resolved.push_back(id);return names(id);});
        need(resolved==std::vector<std::uint32_t>({0x08000149u,0x0c00025bu}));
        queries(a,true);queries(b,false);
        rejects([&]{SourceUnitSpeechChannelSymbolsTu15 duplicate(a,true,names);});
        queries(a,true);
        {
            SourceUnitSpeechChannelSymbolsTu15 second(b,true,names);
            queries(a,true);queries(b,true);
        }
        queries(a,true);queries(b,false);
        std::atomic<bool> valid=true;
        std::vector<std::thread> threads;
        for(unsigned i=0;i<4;++i)threads.emplace_back([&]{
            for(unsigned repeat=0;repeat<1000;++repeat)
                for(auto [pc,raw,translated]:pairs)
                    if(speech_query_from_peer(a,pc,raw)!=translated ||
                       source_unit_speech_channel_symbol_tu15(b,pc,raw)!=raw)valid=false;
        });
        for(auto&thread:threads)thread.join();
        need(valid);
        lease.reset();queries(a,false);queries(b,false);
    }
    {auto replacement=speech_lease_from_peer(a,names);queries(a,true);}
    queries(a,false);
    std::cout<<"{\"passed\":true,\"controls\":"<<controls
             <<",\"cross_TU_threads\":4,\"threaded_queries\":16000}\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
