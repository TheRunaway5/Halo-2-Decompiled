#include "halo2/source_bsp_part_commands.hpp"
#include <bit>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
void require(bool v,const char* why){if(!v)throw FormatError(why);}
struct Reader {
    std::span<const std::uint32_t> data;unsigned cursor{};
    std::uint32_t word(){require(cursor<data.size(),"BSP frame fixture truncated");return data[cursor++];}
    Words bank(){Words out(word());for(auto& v:out)v=word();return out;}
};
std::uint16_t half(const Words& w,unsigned offset) {
    std::uint16_t v;std::memcpy(&v,reinterpret_cast<const std::byte*>(w.data())+offset,2);return v;
}
void half(Words& w,unsigned offset,std::uint16_t v) {
    std::memcpy(reinterpret_cast<std::byte*>(w.data())+offset,&v,2);
}
void emit(Words& out,const Words& bank){out.push_back(bank.size());out.insert(out.end(),bank.begin(),bank.end());}
Words solve(std::span<const std::uint32_t> input) {
    Reader r{input};const auto kind=r.word();Words out;
    if(kind==0) {
        auto bytes=r.bank();require(bytes.size()*4==0x4224,"material visibility guarded allocation differs");
        auto allocation=std::as_writable_bytes(std::span(bytes)).subspan(16,0x4204);
        source_bsp_material_visibility_initialize(allocation);emit(out,bytes);
    } else if(kind==1) {
        auto context=r.bank();require(context.size()*4==0x2ae0,"query reset context extent differs");
        std::array<Words,4> queues;for(auto& q:queues){q=r.bank();require(q.size()==6,"query header extent differs");}
        SourceBspQueryResetState state;
        for(unsigned i=0;i<4;++i)state.queue_counts[i]=half(queues[i],4);
        std::copy_n(context.begin()+0x21c/4,16,state.query_mask.begin());
        std::copy_n(context.begin()+0x25c/4,512,state.arena.words.begin());
        state.arena.used=half(context,0xa5c);state.association_count=half(context,0xa5e);
        std::memcpy(state.cluster_entries.data(),reinterpret_cast<const std::byte*>(context.data())+0x1c,512);
        source_bsp_query_begin(state);
        for(unsigned i=0;i<4;++i)half(queues[i],4,state.queue_counts[i]);
        std::copy(state.query_mask.begin(),state.query_mask.end(),context.begin()+0x21c/4);
        std::copy(state.arena.words.begin(),state.arena.words.end(),context.begin()+0x25c/4);
        half(context,0xa5c,state.arena.used);half(context,0xa5e,state.association_count);
        std::memcpy(reinterpret_cast<std::byte*>(context.data())+0x1c,state.cluster_entries.data(),512);
        emit(out,context);for(const auto& q:queues)emit(out,q);
    } else if(kind==2) {
        const auto mode=r.word(),selector=r.word();SourceBspPartCommandState state;
        for(auto& b:state.banks) {
            for(auto& v:b.header)v=r.word();b.storage.resize(r.word());
            for(auto& c:b.storage)for(auto& v:c.words)v=r.word();
        }
        for(auto& v:state.main_object_counts)v=r.word();
        state.special_count=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));
        for(auto& u:state.special_units)for(auto& v:u.words)v=r.word();
        const auto mask=source_part_commands_begin(state,mode,selector);
        out.push_back(mask);
        for(const auto& b:state.banks) {
            out.insert(out.end(),b.header.begin(),b.header.end());out.push_back(b.storage.size());
            for(const auto& c:b.storage)out.insert(out.end(),c.words.begin(),c.words.end());
        }
        out.insert(out.end(),state.main_object_counts.begin(),state.main_object_counts.end());
        out.push_back(std::uint16_t(state.special_count));
        for(const auto& u:state.special_units)out.insert(out.end(),u.words.begin(),u.words.end());
        require(state.calls==Words{0x40e90},"command reset entry differs");
    } else throw FormatError("BSP frame fixture kind differs");
    require(r.cursor==r.data.size(),"BSP frame fixture trailing input");return out;
}
}
int main(int argc,char** argv)try {
    const auto path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_bsp_frame.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"BSP frame fixture absent");
    const auto size=f.tellg();require(size>=12 && size%4==0,"BSP frame fixture extent");Words data(std::size_t(size)/4);
    f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"BSP frame fixture read");Reader r{data};
    require(r.word()==0x46505342 && r.word()==1,"BSP frame fixture schema");const auto count=r.word();std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i) {
        const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"BSP frame row extent");
        const auto result=solve(r.data.subspan(r.cursor,ni));r.cursor+=ni;
        require(result.size()==no,"BSP frame output extent");
        for(unsigned j=0;j<no;++j)if(result[j]!=r.word())throw FormatError("BSP frame case"+std::to_string(i)+" word"+std::to_string(j)+" differs");
        checked+=no;
    }
    require(r.cursor==data.size(),"BSP frame fixture trailing words");
    std::cout<<"PASS "<<count<<" unchanged183EC0/1348E0/40E90 reset cases / "<<checked<<" exact retained words\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
