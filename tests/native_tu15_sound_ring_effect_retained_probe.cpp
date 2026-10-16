#include "halo2/native_tu15_sound_ring_effect.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <iostream>
#include <stdexcept>
using namespace halo2::tu15_sound_ring_effect;
namespace {
void require(bool v,const char* text){if(!v)throw std::runtime_error(text);}
Word read(){Word x;require(std::fread(&x,sizeof x,1,stdin)==1,"truncated probe input");return x;}
void write(Word x){require(std::fwrite(&x,sizeof x,1,stdout)==1,"probe output error");}
constexpr std::array<unsigned,14> sizes{6000,6000,6000,6000,680,612,640,720,1392,1684,2044,2476,24000,24000};
constexpr std::array<unsigned,31> route{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,2,2,3,3,4,5,6,7,8,9,10,11,12,13};
constexpr std::array<unsigned,14> write_block{2,16,18,20,3,4,5,6,7,8,9,10,11,12};
}
#include "native_tu15_sound_ring_effect_retained_owners.hpp"
int main(){try{ for(;;){
    Word next;if(std::fread(&next,sizeof next,1,stdin)!=1)break;const unsigned statebase=next;require(statebase==0x22d||statebase==0x457,"unsupported probe source record");
    std::vector<unsigned> state_patches;for(unsigned n=read();n;n--)state_patches.push_back(read());
    std::vector<std::array<unsigned,2>> history_patches;for(unsigned n=read();n;n--)history_patches.push_back({read(),read()});
    Controls c;c.entry_status=read();for(auto& v:c.modifiers)v=read();
    std::array<Word,554> raw{};for(auto& v:raw){v=read();require(v<=0xffffffu,"source state word outside24bit");}
    const bool cold=!(raw[4]&1u),update=raw[4]&4u;
    State s;Workspace workspace{};s.flags=raw[4];s.cross_gain=raw[97];s.mono_gain=raw[98];s.output_mode=raw[105];s.smoothing_count=raw[526];
    for(unsigned j=0;j<19;j++)s.delays[j]=raw[70+j];for(unsigned j=0;j<8;j++)s.reflection_gains[j]=raw[89+j];
    for(unsigned j=0;j<6;j++){s.target_gains[j]=raw[99+j];s.current_gains[j]=raw[504+j];}
    for(unsigned j=0;j<10;j++)s.filters[j]={raw[106+j*3],raw[107+j*3],raw[108+j*3]};
    for(unsigned j=0;j<7;j++)s.diffusion_gains[j]=raw[136+j];for(unsigned j=0;j<4;j++)s.wet_gains[j]=raw[143+j];for(unsigned j=0;j<12;j++)s.cluster_taps[j]=raw[147+j];for(unsigned j=0;j<10;j++)s.dormant_biquad_coefficients[j]=raw[179+j];
    auto& f=s.fade;f.pending_mask=raw[510];f.sample_count=raw[511];f.step=raw[512];f.amplitude=raw[513];f.write_stride=raw[516]/7;require(raw[516]%7==0,"invalid source write stride");
    f.bindings_initialized=raw[514]!=0;require(bool(raw[515])==f.bindings_initialized&&bool(raw[517])==f.bindings_initialized,"partial source transition bindings");
    if(f.bindings_initialized){require(raw[514]>=statebase+287&&(raw[514]-statebase-287)%7==0,"invalid source selected read");f.selected_read=(raw[514]-statebase-287)/7;require(raw[515]>=statebase+189&&(raw[515]-statebase-189)%7==0,"invalid source selected write");f.selected_write=(raw[515]-statebase-189)/7;if(raw[517]>=statebase+70&&raw[517]<=statebase+89){f.tap_cursor=raw[517]-statebase-70;f.tap_owner=TapOwner::targets;}else {require(raw[517]>=statebase+518&&raw[517]<=statebase+526,"invalid source selected tap");f.tap_cursor=raw[517]-statebase-518;f.tap_owner=TapOwner::adjusted;}}
    for(unsigned j=0;j<8;j++)f.adjusted_taps[j]=raw[518+j];
    for(unsigned j=0;j<14;j++){s.rings[j].write_byte_cursor=raw[189+j*7+4];s.rings[j].words.resize(sizes[j]);}for(unsigned j=0;j<31;j++)s.taps[j].read_byte_cursor=raw[287+j*7+4];
    for(auto& b:workspace)for(auto& v:b)v=read();
    std::array<Word,32> input{};for(auto& v:input)v=read();std::array<std::array<Word,32>,4> output{};for(auto& o:output)for(auto& v:o)v=read();
    for(auto& ring:s.rings)for(auto& v:ring.words)v=read();
    require(raw[11]==0x81a&&raw[6]==0x7fa,"unsupported source workspace owner");
    require(raw[5]==(statebase==0x22d?0x1580u:0x1560u),"unsupported source input binding");
    for(unsigned j=0;j<4;j++)require(raw[7+j]==(statebase==0x22d?0xe60u:0xde0u)+j*32,"unsupported source output binding");
    unsigned expected_base=raw[0];
    for(unsigned j=0;j<14;j++){
        require(raw[43+j*2]+1==sizes[j]*4,"unsupported source ring size declaration");
        if(!cold)require(raw[42+j*2]==expected_base,"unsupported source ring pair owner");
        if(!cold){const unsigned offset=189+j*7;require(raw[offset]==(j==13?0x4000u:statebase+offset+7)&&raw[offset+5]==expected_base,"unsupported source write chain or ring owner");}
        expected_base+=sizes[j]*4;
    }
    if(!cold)for(unsigned j=0;j<31;j++){const unsigned offset=287+j*7;require(raw[offset]==(j==30?0x4000u:statebase+offset+7)&&raw[offset+5]==raw[42+route[j]*2],"unsupported source read chain or ring owner");}
    if(!cold){for(unsigned j=0;j<31;j++)require(raw[11+j]==0x7fa+(2+j)*16,"unsupported source workspace pointer table");for(unsigned j=0;j<14;j++){require(raw[189+j*7+1]==0x4bd2&&raw[189+j*7+2]==16&&raw[189+j*7+3]==0x7fa+write_block[j]*16&&raw[189+j*7+6]+1==sizes[j]*4,"unsupported source write descriptor");}for(unsigned j=0;j<31;j++)require(raw[287+j*7+1]==0x4bd0&&raw[287+j*7+2]==16&&raw[287+j*7+3]==0x7fa+(2+j)*16&&raw[287+j*7+6]+1==sizes[route[j]]*4,"unsupported source read descriptor");}
    require(reinterpret_cast<std::uintptr_t>(&s)>0xffffffffu&&reinterpret_cast<std::uintptr_t>(&workspace)>0xffffffffu,"probe native owner not above4GiB");
    for(const auto& ring:s.rings)require(reinterpret_cast<std::uintptr_t>(ring.words.data())>0xffffffffu,"probe native ring not above4GiB");
    std::array<std::span<Word,32>,4> spans{output[0],output[1],output[2],output[3]};ArithmeticTrace trace;
    const unsigned owner_index=statebase==0x22d?0:1;
    State& live=retain(s,owner_index,state_patches,history_patches);
    if(owner_index==0&&!retained[1].initialized)shared_workspace=workspace;
    else require(shared_workspace==workspace,"shared workspace diverged across retained calls");
    process(live,shared_workspace,input,spans,c,&trace);
    stable_addresses(owner_index);workspace=shared_workspace;
    raw[4]=live.flags;raw[526]=live.smoothing_count;
    for(unsigned j=0;j<6;j++)raw[504+j]=live.current_gains[j];for(unsigned j=0;j<10;j++){raw[106+j*3]=live.filters[j].previous;raw[107+j*3]=live.filters[j].feedback;raw[108+j*3]=live.filters[j].feedforward;}for(unsigned j=0;j<10;j++)raw[179+j]=live.dormant_biquad_coefficients[j];
    raw[510]=live.fade.pending_mask;raw[511]=live.fade.sample_count;raw[512]=live.fade.step;raw[513]=live.fade.amplitude;raw[516]=live.fade.write_stride*7;
    if(live.fade.bindings_initialized){raw[514]=statebase+287+live.fade.selected_read*7;raw[515]=statebase+189+live.fade.selected_write*7;raw[517]=statebase+(live.fade.tap_owner==TapOwner::targets?70+live.fade.tap_cursor:518+live.fade.tap_cursor);}
    for(unsigned j=0;j<8;j++)raw[518+j]=live.fade.adjusted_taps[j];
    if(cold||update)for(unsigned j=0;j<31;j++)raw[11+j]=0x7fa+(2+j)*16;
    if(cold){unsigned base=raw[0];for(unsigned j=0;j<14;j++){raw[42+j*2]=base;base+=sizes[j]*4;const unsigned offset=189+j*7;raw[offset]=j==13?0x4000:statebase+offset+7;raw[offset+1]=0x4bd2;raw[offset+2]=16;raw[offset+3]=0x7fa+write_block[j]*16;raw[offset+5]=raw[42+j*2];raw[offset+6]=sizes[j]*4-1;}for(unsigned j=0;j<31;j++){const unsigned offset=287+j*7;raw[offset]=j==30?0x4000:statebase+offset+7;raw[offset+1]=0x4bd0;raw[offset+2]=16;raw[offset+3]=0x7fa+(2+j)*16;raw[offset+5]=raw[42+route[j]*2];raw[offset+6]=sizes[route[j]]*4-1;}}
    for(unsigned j=0;j<14;j++)raw[189+j*7+4]=live.rings[j].write_byte_cursor;for(unsigned j=0;j<31;j++)raw[287+j*7+4]=live.taps[j].read_byte_cursor;
    for(auto v:raw)write(v);for(const auto& b:workspace)for(auto v:b)write(v);for(auto v:input)write(v);for(const auto& o:output)for(auto v:o)write(v);for(const auto& ring:live.rings)for(auto v:ring.words)write(v);write(trace.count);for(unsigned j=0;j<trace.count;j++)write(trace.values[j]);
std::fflush(stdout);}
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
