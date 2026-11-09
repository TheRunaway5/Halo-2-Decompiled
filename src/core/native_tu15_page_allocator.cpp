#include "halo2/native_tu15_page_allocator.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include "halo2/native_tu15_page_window_kernel.hpp"
#include "halo2/native_tu15_page_retire_kernel.hpp"
#include <array>
#include <bit>
#include <stdexcept>

namespace halo2 {namespace {
using M=SourcePhysicsContactMemory;using A=SourcePhysicsContactRuntimeAbi;
using O=SourcePhysicsContactRecordObserver;using S=SourcePhysicsContactRuntimeServices;
std::uint32_t read(const M& m,std::uint32_t at,unsigned n=4){
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(n));std::uint32_t v{};
    for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(8u*k);return v;
}
void write(M& m,const O& o,std::uint32_t pc,std::uint32_t at,std::uint32_t v,unsigned n=4){
    SourcePhysicsContactRecordStore event{pc,at,std::vector<std::byte>(n),std::vector<std::byte>(n)};
    m.read(at,event.before);for(unsigned k=0;k<n;++k)event.written[k]=std::byte(v>>(8u*k));
    m.write(at,event.written);if(o)o(event);
}
void push(M& m,A& a,std::uint32_t pc,std::uint32_t v){a.gp[4]-=4u;write(m,{},pc,a.gp[4],v);}
std::uint32_t pop(const M& m,A& a){const auto v=read(m,a.gp[4]);a.gp[4]+=4u;return v;}
std::int32_t i(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
void byte(std::uint32_t& v,std::uint32_t b){v=(v&0xffffff00u)|(b&255u);}
void callback(M& m,A& a,const S& s,const O& o,std::uint32_t pc,std::uint32_t next){
    if(!s.required_owner)throw std::runtime_error("Native TU1.5 page allocator callback has no owner");
    const auto target=a.gp[0];push(m,a,pc,next);s.required_owner(m,{pc,target,false},a,o);
}
// Private complete32A660 CPU stack-probe composition. The reads touch the
// actual caller pages; no guard-page/OS success is fabricated.
void stack_probe(M& m,A& a){
    auto& g=a.gp;if(!g[0]){(void)pop(m,a);return;}
    if(g[0]<0x1000u){g[0]=0u-g[0]+g[4]+4u;(void)read(m,g[0]);std::swap(g[4],g[0]);
        g[0]=read(m,g[0]);push(m,a,0x32a677u,g[0]);(void)pop(m,a);return;}
    push(m,a,0x32a679u,g[1]);g[1]=g[4]+8u;
    do{g[1]-=0x1000u;g[0]-=0x1000u;(void)read(m,g[1]);}while(g[0]>=0x1000u);
    g[1]-=g[0];g[0]=g[4];(void)read(m,g[1]);g[4]=g[1];g[1]=read(m,g[0]);
    g[0]=read(m,g[0]+4u);push(m,a,0x32a69fu,g[0]);(void)pop(m,a);
}
}

void native_tu15_page_window_prefer_abi(M& m,A& a){
    // Keep this legacy scratch-flags API while sharing the actual DATA
    // policy with the independently qualified complete caller ABI adapter.
    struct Access {
        M &memory;A &abi;using P=NativeTu15PageWindowPhase;
        std::uint32_t read(P phase,const NativeTu15PageWindow&,std::uint32_t at){
            auto &g=abi.gp;
            if(phase==P::candidate_tick)push(memory,abi,0x13fe03u,g[3]);
            if(phase==P::best_tick){g[1]=std::min(g[2]-g[3],300u);byte(g[0],0);}
            const auto value=halo2::read(memory,at);
            if(phase==P::owner_tick||phase==P::candidate_count)g[2]=value;
            else if(phase==P::candidate_tick)g[3]=value;
            else if(phase==P::best_tick){g[2]=std::min(g[2]-value,300u);g[3]=g[1]+150u;g[3]=pop(memory,abi);}
            return value;
        }
        void prepare(P,const NativeTu15PageWindow&){abi.gp[2]+=150u;}
        void complete(P,const NativeTu15PageWindow &data){if(data.prefer)byte(abi.gp[0],1);(void)pop(memory,abi);}
    }access{m,a};
    NativeTu15PageWindow data;data.owner=a.gp[1];data.candidate=a.gp[6];data.best=a.gp[7];native_tu15_prefer_page_window(access,data);
}

void native_tu15_page_delete_abi(M& m,A& a,const S& s,const O& o){
    struct Access {
        M &memory;A &abi;const O &observer;using P=NativeTu15PageRetirePhase;
        std::uint32_t read(P phase,const NativeTu15PageRetirement&,std::uint32_t at){
            auto &g=abi.gp;if(phase==P::callback){g[0]=(g[3]&0xffffu)*3u;push(memory,abi,0x140320u,g[6]);g[6]=g[2]+g[0]*8u;}
            const auto value=halo2::read(memory,at);
            switch(phase){
            case P::pool:case P::previous_pool:case P::next_pool:g[1]=value;break;
            case P::rows:case P::previous_rows:case P::next_rows:case P::next_for_head:case P::previous_for_tail:case P::delete_pool:g[2]=value;break;
            case P::callback:case P::previous:case P::next:g[0]=value;break;
            case P::next_for_previous:case P::previous_for_next:g[1]=value;g[0]=(g[0]&0xffffu)*3u;break;
            default:break;
            }
            return value;
        }
        void write(P phase,const NativeTu15PageRetirement&,std::uint32_t at,std::uint32_t value){
            const auto site=phase==P::previous_store?0x140347u:phase==P::head_store?0x140350u:phase==P::next_store?0x14036cu:0x14037eu;
            halo2::write(memory,observer,site,at,value);
        }
        void prepare(P,const NativeTu15PageRetirement&){abi.gp[0]=abi.gp[3];abi.gp[6]=pop(memory,abi);}
    }access{m,a,o};
    struct Services {
        M &memory;A &abi;const S &services;const O &observer;
        NativeTu15PageRetireResume callback(std::uint32_t,std::uint32_t,const NativeTu15PageRetirement&){
            push(memory,abi,0x14032bu,abi.gp[3]);halo2::callback(memory,abi,services,observer,0x14032cu,0x14032eu);return{abi.gp[7],abi.gp[3],abi.gp[6]};
        }
        void erase(std::uint32_t,std::uint32_t,const NativeTu15PageRetirement&){native_tu15_data_array_delete_abi(memory,abi,observer);}
    }services{m,a,s,o};
    NativeTu15PageRetirement data;data.owner=a.gp[7];data.datum=a.gp[3];native_tu15_retire_page(access,data,services);
}

void native_tu15_page_allocate_abi(M& m,A& a,const S& s,const O& o){
    auto& g=a.gp;
    const auto local=[&](std::uint32_t offset){return read(m,g[4]+offset);};
    const auto frame=[&](std::uint32_t pc,std::uint32_t offset,std::uint32_t v,unsigned n=4){write(m,{},pc,g[4]+offset,v,n);};
    const auto finish=[&](){g[6]=pop(m,a);g[5]=pop(m,a);g[3]=pop(m,a);g[4]+=0x1054u;(void)pop(m,a);g[4]+=12u;};
    g[0]=0x1054u;push(m,a,0x13fe55u,0x13fe5au);stack_probe(m,a);
    g[0]=local(0x105cu);push(m,a,0x13fe61u,g[3]);push(m,a,0x13fe62u,g[5]);g[5]=local(0x1060u);
    g[1]=read(m,g[5]+0x34u);g[2]=1;push(m,a,0x13fe72u,g[6]);g[2]<<=(g[1]&31u);
    g[6]=std::uint32_t(i(g[0])>>(g[1]&31u));--g[2];frame(0x13fe7cu,0x18u,g[6]);
    if(g[0]&g[2]){++g[6];frame(0x13fe83u,0x18u,g[6]);}
    g[1]=local(0x106cu);g[0]=0xffffffffu;g[3]=0;frame(0x13fe95u,0x1cu,g[0]);
    if(i(g[1])<0)frame(0x13fe9bu,0x28u,g[3]);
    else{frame(0x13fea4u,0x28u,8u);if(i(g[1])<=8)frame(0x13feaeu,0x28u,g[1]);}
    g[1]=local(0x28u);
    if(i(g[6])>=i(read(m,g[5]+g[1]*4u+0x44u))&&i(g[1])>i(g[3])){finish();return;}
    g[1]=read(m,g[5]+0x30u);push(m,a,0x13fec3u,g[7]);g[7]=read(m,g[5]+0x3cu);frame(0x13fec7u,0x34u,g[0]);
    g[2]=g[0]=0;frame(0x13fed1u,0x13u,0,1);frame(0x13fed6u,0x14u,g[2]);frame(0x13fedau,0x18u,g[0]);
    frame(0x13fedeu,0x28u,g[7]);frame(0x13fee2u,0x48u,0xffffffffu);
    if(i(g[1])>0)goto scan;
failed:
    g[1]=local(0x2cu);
    if(local(0x1070u)==g[1]){g[0]=read(m,g[5]+g[1]*4u+0x44u);g[2]=local(0x1cu);
        if(i(g[0])>i(g[2]))g[0]=g[2];write(m,o,0x13ff06u,g[5]+g[1]*4u+0x44u,g[0]);}
    g[0]=local(0x20u);g[7]=pop(m,a);finish();return;
next_span:
    g[2]=local(0x14u);g[0]=local(0x18u);
scan:
    g[1]=(g[0]+1u)&0x800000ffu;
    if(i(g[1])<0){--g[1];g[1]|=0xffffff00u;++g[1];}
    if(g[1]!=g[2]){g[6]=local(0x48u);g[0]=(g[0]<<4u)+g[4]+0x64u;
        write(m,{},0x13ff45u,g[0],g[6]);g[6]=0;write(m,{},0x13ff49u,g[0]+8u,g[3]);
        write(m,{},0x13ff4cu,g[0]+4u,g[6]);write(m,{},0x13ff4fu,g[0]+12u,g[6]);frame(0x13ff52u,0x18u,g[1]);}
    if(g[7]==0xffffffffu){g[0]=read(m,g[5]+0x30u);g[1]=g[0]-g[3];frame(0x13ff62u,0x24u,0);
        frame(0x13ff6au,0x30u,g[1]);g[3]=g[0];goto windows;}
    g[1]=read(m,g[5]+0x64u);g[1]=read(m,g[1]+0x44u);g[0]=(g[7]&0xffffu)*3u;g[6]=g[1]+g[0]*8u;
    if(g[3]!=read(m,g[6]+8u)){g[6]=read(m,g[6]+8u);g[0]=g[6]-g[3];frame(0x140027u,0x24u,0);
        frame(0x14002fu,0x30u,g[0]);g[3]=g[6];goto windows;}
    g[0]=read(m,g[6]+4u);g[2]=read(m,g[6]+0x14u);frame(0x13ff97u,0x30u,g[0]);g[0]=read(m,g[5]+0x24u);frame(0x13ffa0u,0x24u,g[2]);
    if(g[0]){push(m,a,0x13ffa6u,g[7]);callback(m,a,s,o,0x13ffa7u,0x13ffa9u);byte(g[1],(g[0]&255u)?1u:0u);}
    else byte(g[1],0);
    g[0]=read(m,g[6]+0x14u);g[2]=local(0x1070u);g[3]=read(m,g[5]+0x38u);g[2]+=g[0];
    if(g[2]>=g[3])byte(g[1],1);
    else if(!(g[1]&255u)&&(local(0x34u)==0xffffffffu||g[0]<local(0x50u))){frame(0x13ffdbu,0x34u,g[7]);frame(0x13ffdfu,0x50u,g[0]);}
    g[0]=read(m,g[6]+12u);g[2]=read(m,g[5]+0x64u);g[3]=read(m,g[6]+4u);g[2]=read(m,g[2]+0x44u);
    frame(0x13ffefu,0x28u,g[0]);g[0]&=0xffffu;frame(0x13fff8u,0x48u,g[7]);g[7]=read(m,g[6]+8u);g[0]*=3u;
    g[3]+=g[7];g[7]=local(0x28u);g[0]=g[2]+g[0]*8u;
    if(g[1]&255u){g[0]=local(0x18u);frame(0x140017u,0x14u,g[0]);goto span_done;}
    g[2]=local(0x14u);
windows:
    g[0]=g[2];if(g[2]==local(0x18u))goto span_done;g[2]=local(0x24u);
window:
    g[1]=g[0]<<4u;g[6]=g[4]+g[1]+0x64u;g[1]=(g[0]+1u)&0x800000ffu;
    if(i(g[1])<0){--g[1];g[1]|=0xffffff00u;++g[1];}
    g[0]=g[1]<<4u;g[0]+=g[4]+0x64u;frame(0x14007du,0x4cu,g[1]);
    if(g[2]>read(m,g[6]+4u))write(m,{},0x140083u,g[6]+4u,g[2]);
    g[0]=local(0x30u);g[7]=read(m,g[6]+12u);g[7]+=g[0];write(m,{},0x140093u,g[6]+12u,g[7]);
    if(i(g[7])<i(local(0x1cu)))goto next_window;
    byte(g[0],local(0x13u)&255u);if(!(g[0]&255u))goto choose;
    g[0]=read(m,g[5]+0x2cu);
    if(g[0]==0){g[1]=read(m,g[6]+4u);g[2]=local(0x3cu);byte(g[0],0);
        if(g[1]<g[2]||(g[1]==g[2]&&i(g[7])<i(local(0x44u))))byte(g[0],1);}
    else if(g[0]==1){g[0]=0;g[7]=g[4]+0x38u;g[1]=g[5];push(m,a,0x1400e6u,0x1400ebu);native_tu15_page_window_prefer_abi(m,a);}
    else{--g[0];g[2]=read(m,g[5]+0x38u);g[1]=read(m,g[6]+4u);g[0]=g[2]-g[1];g[1]=g[0]?g[0]:0;
        g[0]=g[2]-local(0x3cu);g[2]=g[0]?g[0]:0;g[0]=local(0x44u);g[7]*=g[2];g[0]*=g[1];byte(g[0],i(g[0])>i(g[7]));}
    g[1]=local(0x4cu);if(!(g[0]&255u))goto advance_begin;
choose:
    g[2]=read(m,g[6]);g[0]=read(m,g[6]+4u);frame(0x140111u,0x38u,g[2]);g[2]=read(m,g[6]+8u);
    frame(0x140118u,0x3cu,g[0]);g[0]=read(m,g[6]+12u);frame(0x14011fu,0x40u,g[2]);frame(0x140123u,0x44u,g[0]);frame(0x140127u,0x13u,1,1);
advance_begin:
    g[2]=(local(0x14u)+1u)&0x800000ffu;if(i(g[2])<0){--g[2];g[2]|=0xffffff00u;++g[2];}
    frame(0x140141u,0x14u,g[2]);g[2]=local(0x24u);
next_window:
    g[0]=g[1];if(g[0]!=local(0x18u))goto window;g[7]=local(0x28u);
span_done:
    if(i(g[3])<i(read(m,g[5]+0x30u)))goto next_span;
    byte(g[0],local(0x13u)&255u);if(!(g[0]&255u))goto failed;
    g[6]=read(m,g[5]+0x64u);frame(0x140171u,0x60u,0xffffffffu);
datum_scan:
    g[2]=local(0x60u)+1u;if(i(g[2])<0)goto scan_finished;
    g[7]=read(m,g[6]+0x38u);if(i(g[2])>=i(g[7]))goto scan_finished;g[0]=read(m,g[6]+0x48u);
bitmap:
    g[1]=g[2]&31u;g[3]=1u<<(g[1]&31u);g[1]=std::uint32_t(i(g[2])>>5u);
    if(read(m,g[0]+g[1]*4u)&g[3])goto occupied;
    ++g[2];if(i(g[2])<i(g[7]))goto bitmap;
scan_finished:
    g[1]=read(m,g[5]+0x64u);g[2]=read(m,g[1]+0x3cu);
    if(g[2]!=read(m,g[1]+0x20u))goto allocate;
    g[3]=local(0x34u);if(g[3]==0xffffffffu)goto allocate;g[6]=local(0x38u);
    if(g[6]==g[3]){g[1]=read(m,g[1]+0x44u);g[0]=(g[3]&0xffffu)*3u;g[2]=read(m,g[1]+g[0]*8u+0x10u);
        frame(0x1401ddu,0x38u,g[2]);g[6]=g[2];}
    g[7]=g[5];push(m,a,0x1401e5u,0x1401eau);native_tu15_page_delete_abi(m,a,s,o);goto allocated_predecessor;
occupied:
    if(g[2]==0xffffffffu)goto scan_finished;
    g[0]=read(m,g[6]+0x24u);g[3]=read(m,g[6]+0x44u);g[0]*=g[2];
    g[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,g[0]+g[3],2)))));
    g[7]=local(0x40u);g[0]+=g[3];g[3]=local(0x1cu);g[1]=(g[1]<<16u)|g[2];frame(0x14020du,0x60u,g[2]);
    g[2]=read(m,g[0]+8u);g[3]+=g[7];if(i(g[2])>=i(g[3]))goto datum_scan;
    g[0]=read(m,g[0]+4u);g[0]+=g[2];g[2]=g[7];if(i(g[0])<=i(g[2]))goto datum_scan;
    g[3]=g[1];g[7]=g[5];push(m,a,0x140231u,0x140236u);native_tu15_page_delete_abi(m,a,s,o);goto datum_scan;
allocate:
    g[6]=local(0x38u);
allocated_predecessor:
    g[2]=read(m,g[5]+0x64u);push(m,a,0x140242u,0x140247u);native_tu15_data_array_allocate_abi(m,a,o);
    frame(0x14024au,0x20u,g[0]);if(g[0]==0xffffffffu)goto failed;
    g[0]=read(m,g[5]+0x64u);g[1]=read(m,g[0]+0x44u);g[7]=local(0x20u);g[0]=g[7]&0xffffu;g[3]=0xffffffffu;
    g[2]=g[0]*3u;g[0]=g[1]+g[2]*8u;
    if(g[6]==g[3]){
        g[2]=read(m,g[5]+0x3cu);const bool empty=g[2]==g[3];write(m,o,0x140277u,g[0]+0x10u,g[3]);
        if(empty){g[1]=read(m,g[5]+0x3cu);write(m,o,0x14027fu,g[5]+0x40u,g[7]);
            write(m,o,0x140282u,g[0]+12u,g[1]);write(m,o,0x140285u,g[5]+0x3cu,g[7]);}
        else{g[2]=(g[2]&0xffffu)*3u;g[1]+=g[2]*8u;write(m,o,0x140296u,g[1]+0x10u,g[7]);
            g[1]=read(m,g[5]+0x3cu);write(m,o,0x14029cu,g[0]+12u,g[1]);write(m,o,0x14029fu,g[5]+0x3cu,g[7]);}
    }else{
        g[6]=(g[6]&0xffffu)*24u;g[2]=read(m,g[6]+g[1]+12u);
        if(g[2]==g[3]){g[1]=read(m,g[5]+0x40u);write(m,o,0x1402bbu,g[0]+0x10u,g[1]);write(m,o,0x1402beu,g[5]+0x40u,g[7]);}
        else{g[2]=(g[2]&0xffffu)*3u;g[1]+=g[2]*8u;g[2]=read(m,g[1]+0x10u);
            write(m,o,0x140305u,g[0]+0x10u,g[2]);write(m,o,0x140308u,g[1]+0x10u,g[7]);}
        g[2]=read(m,g[5]+0x64u);g[1]=read(m,g[2]+0x44u);g[2]=read(m,g[1]+g[6]+12u);g[1]+=g[6];
        write(m,o,0x1402cdu,g[0]+12u,g[2]);write(m,o,0x1402d0u,g[1]+12u,g[7]);
    }
    g[1]=local(0x40u);g[2]=local(0x1cu);write(m,o,0x1402dbu,g[0]+8u,g[1]);write(m,o,0x1402deu,g[0]+4u,g[2]);
    g[1]=read(m,g[5]+0x38u);write(m,o,0x1402e4u,g[0]+0x14u,g[1]);g[0]=g[7];g[7]=pop(m,a);finish();
}
}
