#include "halo2/native_campaign_ai_command_lifecycle_tu15.hpp"
#include "halo2/native_campaign_ai_query_tu15.hpp"
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <bit>
#include <vector>
namespace halo2 {
namespace {
using U=std::uint32_t;using S=std::int32_t;constexpr U none=0xffffffffu;
S sign16(U n){return std::bit_cast<std::int16_t>(std::uint16_t(n));}
struct Lifecycle {
    SourcePhysicsContactMemory &m;SourcePhysicsWorldRuntimeAbi &a;
    const NativeCampaignAiCommandLifecycleServicesTu15 &s;const SourcePhysicsContactRecordObserver &o;
    U r(U p,unsigned width=4)const {std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));U v{};
        for(unsigned n=0;n<width;++n)v|=std::to_integer<U>(b[n])<<(n*8u);return v;}
    void w(U pc,U p,U v,unsigned width=4)const {
        const bool observed=native_tu15_observes_retained_store(m,o,pc,p,width);
        SourcePhysicsContactRecordStore event{pc,p,{},std::vector<std::byte>(width)};
        if(observed){event.before.resize(width);m.read(p,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(v>>(n*8u));m.write(p,event.written);if(observed)o(event);
    }
    U row(U global,U id,U stride)const{return r(r(global)+0x44u)+(id&0xffffu)*stride;}
    U actor(U id)const{return row(0x5020c8u,id,0x888u);}U command(U id)const{return row(0x50eee0u,id,0xd4u);}
    void push(U &sp,U v){sp-=4u;std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8u));m.write(sp,b);}
    U call(U pc,U entry,std::array<U,8> gp,std::initializer_list<U> args={},unsigned bytes=5) {
        const U sp=gp[4];for(auto p=args.end();p!=args.begin();)push(gp[4],*--p);push(gp[4],pc+bytes);a.gp=gp;
        if(entry==0x16e6a0u)native_tu15_data_array_allocate_abi(m,a,o);
        else if(entry==0x16e8a0u)native_tu15_data_array_delete_abi(m,a,o);
        else if(owns(entry))native_campaign_ai_command_lifecycle_call_tu15(m,entry,a,s,o);
        else {if(!s.callee)throw NativeCampaignAiPerformRequiredOwnerTu15(pc,entry);s.callee(pc,entry,a);}
        if(a.gp[4]!=sp)throw std::runtime_error("Native command lifecycle child returned another frame");return a.gp[0];
    }
    static bool owns(U e){switch(e){case 0x2801d0u:case 0x280400u:case 0x261be0u:case 0x261a90u:case 0x261d50u:
        case 0x262250u:case 0x262190u:case 0x263b80u:case 0x262870u:case 0x2629d0u:
        case 0x2117e0u:case 0x211a80u:case 0x211cb0u:case 0x211840u:case 0x262590u:case 0x262710u:return true;default:return false;}}
    void reset(U id) {
        const auto c=command(id);if(r(c+0xacu,1)||r(c+0xb0u)!=none){
            w(0x263baeu,c+0xacu,0,1);w(0x263bb4u,c+0xb0u,none);w(0x263bbau,c+0xd1u,0,1);
            w(0x263bc0u,c+0x52u,0,1);w(0x263bc3u,c+0x46u,0,1);}
    }
    void associated(U actor_id,U id) {
        if(id!=none && s.associated){const auto c=command(id);s.associated(actor_id,id,r(c+0x38u),std::int16_t(r(c+0x34u)));}
    }
    void retire_thread(U pc,U thread,std::array<U,8> gp) {
        gp[0]=thread;gp[2]=r(0x505e5cu);call(pc,0x16e8a0u,gp);if(s.thread_retired)s.thread_retired(thread);
    }
    void run(U e);
};
void Lifecycle::run(U e) {
    const auto in=a.gp;const U sp=in[4];auto gp=in;
    const auto finish=[&](U value,unsigned pop=0){a.gp=in;a.gp[0]=value;a.gp[4]=sp+4u+pop;};
    if(e==0x263b80u){reset(in[0]);finish(command(in[0]));return;}
    if(e==0x2629d0u){const auto p=r(sp+12u);constexpr std::array<U,9> sites{0x2629d8,0x2629da,0x2629dd,0x2629e0,0x2629e3,0x2629e6,0x2629e9,0x2629ec,0x2629ef};
        for(U n=0;n<9u;++n)w(sites[n],p+n*4u,0);
        w(0x2629f2u,p+4u,0,2);finish(in[0]&0xffff0000u,16);return;}
    if(e==0x211840u){
        const auto thread=in[0],p=row(0x505e5cu,thread,0x418u);if(r(p+8u)==0xfffffffdu)w(0x21186bu,p+8u,0);
        const S wake=std::bit_cast<S>(r(p+8u));if(wake<0||wake>std::bit_cast<S>(r(r(0x51df54u)+8u))){finish(2);return;}
        gp[4]=sp-8u;gp[7]=thread;call(0x211884u,0x211b10u,gp);const auto after=r(p+8u);
        finish(after==none?0u:after==0xfffffffdu?1u:2u);return;
    }
    if(e==0x262590u){
        const auto actor_id=r(sp+4u),id=r(sp+8u);gp[4]=sp-20u;
        for(;;){const auto c=command(id),body=actor(actor_id),thread=r(c+0x38u);
            if(thread==none){w(0x2625dbu,c+0x30u,4,2);finish(3,8);return;}
            if(r(c+0x44u,1)){w(0x2626a0u,c+0x30u,1,2);finish(2,8);return;}
            w(0x262603u,0x50ef00u,(actor_id&0xffffu)|0x80000000u);const auto squad=r(body+0x30u);
            w(squad==none?0x26261cu:0x262615u,0x50ef04u,squad==none?0xc3e703e7u:squad&0xffffu);
            w(0x26262eu,0x50eee4u,actor_id);w(0x262633u,c+4u,none,2);w(0x262637u,c+0x28u,none);w(0x26263au,c+0x2cu,none);
            gp[0]=r(c+0x38u);w(0x262640u,0x50eee8u,id);const auto value=call(0x262646u,0x211840u,gp);
            w(0x26264bu,0x50ef00u,none);const auto busy=r(c+0x44u,1);w(0x262656u,0x50eee4u,none);w(0x26265cu,0x50eee8u,none);
            if(busy){w(0x2626a0u,c+0x30u,1,2);finish(2,8);return;}
            const auto kind=r(c+4u,2);if(kind==0x16u){
                if(r(command(id)+0x38u)==none){w(0x26268du,command(id)+0x30u,4,2);finish(3,8);return;}continue;
            }
            if(kind==0x17u){w(0x2626bau,c+0x30u,2,2);finish(0,8);return;}
            if(std::uint16_t(value)==2u){w(0x2626d0u,c+0x30u,1,2);finish(value,8);return;}
            if(!std::uint16_t(value)){w(0x2626e3u,c+0x30u,2,2);finish(value,8);return;}
            if(std::uint16_t(value)==1u)w(0x2626fbu,c+0x30u,0,2);finish(value,8);return;
        }
    }
    if(e==0x262710u){
        const auto c=command(in[7]),status=r(sp+4u);gp[4]=sp-8u;
        const auto cancelled=r(c+0x7eu,1);w(0x262734u,0x50eed8u,status);w(0x26273au,0x50eed4u,in[2]);
        if(cancelled)w(0x262742u,c+0x30u,2,2);
        const auto invoke=[&](U pc,U callback){return call(pc,0x262870u,gp,{in[3],callback,in[7]});};
        for(;;){const auto state=r(c+0x30u,2);
            if(state==0u){const auto value=invoke(0x262760u,0x263140u);
                if(std::uint16_t(value)==1u){invoke(0x262771u,0x263aa0u);w(0x262776u,c+0x30u,1,2);continue;}
                if(std::uint16_t(value)==2u){w(0x262786u,c+0x30u,1,2);continue;}finish(value,4);return;}
            if(state==3u){const auto value=invoke(0x262799u,0x262a00u);w(0x2627a8u,c+0x30u,std::uint16_t(value)==2u?1u:0u,2);continue;}
            if(state==1u){const auto value=call(0x2627b5u,0x262590u,gp,{in[3],in[7]});
                if(std::uint16_t(value)==2u){w(0x262802u,status,2,2);finish(0,4);return;}
                if(!std::uint16_t(value)){finish(1,4);return;}
                if(std::uint16_t(value)==1u){if(std::uint16_t(invoke(0x2627d1u,0x262a00u))==2u)w(0x2627e0u,c+0x30u,1,2);continue;}
                if(std::uint16_t(value)==3u){finish(2,4);return;}continue;}
            finish(state==2u?1u:2u,4);return;
        }
    }
    if(e==0x211a80u){
        const U script=in[0],kind=r(sp+4u,1);gp[4]=sp-4u;gp[2]=r(0x505e5cu);
        const U id=call(0x211a89u,0x16e6a0u,gp);if(id!=none){const auto p=row(0x505e5cu,id,0x418u);
            w(0x211aaau,p+0x10u,p+0x18u);w(0x211aadu,r(p+0x10u),0);w(0x211ab6u,r(p+0x10u)+0xcu,0,2);
            w(0x211abfu,r(p+0x10u)+4u,none);w(0x211acau,p+2u,kind,1);w(0x211acdu,p+4u,script);w(0x211ad0u,p+3u,0,1);
            const bool dormant=script!=none && r(r(r(0x4ecc20u)+0x1bcu)+script*0x28u+0x20u,2)==1u;
            w(dormant?0x211aedu:0x211af8u,p+8u,dormant?0xfffffffeu:0u);}
        finish(id,4);return;
    }
    if(e==0x211cb0u){
        const U expression=r(sp+4u),thread=row(0x505e5cu,in[2],0x418u),syntax=row(0x505e6cu,expression,20u);
        const auto flags=r(syntax+6u,1);gp[4]=sp-12u;
        if(flags&1u){
            if(flags&4u){const U global=r(syntax+0x10u,2);U source;
                if(global&0x8000u)source=r(r(0x47fb88u+(global&0x7fffu)*4u),2);
                else source=r(r(r(0x4ecc20u)+0x1c4u)+(global&0x7fffu)*0x28u+0x20u,2);
                gp[6]=global;gp[5]=(expression&0xffff0000u)|source;call(0x211d30u,0x211e80u,gp);
                gp=a.gp;gp[1]=(gp[1]&0xffff0000u)|r(syntax+4u,2);gp[2]=(expression&0xffff0000u)|source;
                const auto value=call(0x211d3bu,0x212560u,gp);w(0x211d42u,in[3],value);
            }else{gp[0]=r(syntax+0x10u);gp[1]=(gp[1]&0xffff0000u)|r(syntax+4u,2);gp[2]=(gp[2]&0xffff0000u)|r(syntax+2u,2);
                const auto value=call(0x211d53u,0x212560u,gp);w(0x211d5au,in[3],value);}
        }else{
            auto frame=r(thread+0x10u);w(0x211d63u,frame+8u,in[3]);
            const auto refreshed=row(0x505e5cu,in[2],0x418u);frame=r(refreshed+0x10u);
            const auto next=frame+U(sign16(r(frame+0xcu,2)))+0x10u;w(0x211d76u,next,frame);
            w(0x211d78u,refreshed+0x10u,next);w(0x211d7bu,next+0xcu,0,2);
            const auto flag=r(thread+3u,1),top=r(thread+0x10u);w(0x211d8bu,thread+3u,flag|1u,1);w(0x211d8fu,top+4u,expression);
        }finish(a.gp[0],4);return;
    }
    if(e==0x2117e0u){
        const U script=U(sign16(in[0])),definition=r(r(0x4ecc20u)+0x1bcu)+script*0x28u;
        if(r(definition+0x20u,2)!=5u){finish(none);return;}
        gp[0]=script;gp[4]=sp-8u;const auto id=call(0x211803u,0x211a80u,gp,{4u});
        if(id!=none){gp=a.gp;gp[4]=sp-12u;gp[3]=row(0x505e5cu,id,0x418u)+0x14u;gp[2]=id;gp[0]=r(definition+0x24u);
            call(0x21182eu,0x211cb0u,gp,{gp[0]});}finish(id);return;
    }
    if(e==0x261a90u){
        const U script=r(sp+4u),thread=in[3];gp[4]=sp-8u;gp[2]=r(0x50eee0u);const auto id=call(0x261a9cu,0x16e6a0u,gp);
        if(id!=none){const auto c=command(id);gp=a.gp;gp[4]=sp-12u;w(0x261ac4u,c+0x34u,script);
            if(thread!=none)w(0x261ac9u,c+0x38u,thread);
            else {gp[0]=script;w(0x261ad5u,c+0x38u,call(0x261ad0u,0x2117e0u,gp));}
            for(const auto [pc,off]:std::array<std::array<U,2>,23>{{{0x261ae0,0x45},{0x261ae3,0x46},{0x261ae6,0x51},{0x261ae9,0x52},
                {0x261aec,0x5c},{0x261aef,0x64},{0x261af2,0x6c},{0x261af5,0x74},{0x261af8,0x75},{0x261afb,0x7e},
                {0x261afe,0x79},{0x261b01,0x7a},{0x261b04,0x7f},{0x261b07,0x80},{0x261b0d,0x81},{0x261b13,0x82},
                {0x261b19,0x83},{0x261b26,0x86},{0x261b2c,0x8c},{0x261b32,0x99},{0x261b38,0x77},{0x261b3b,0x78},{0,0}}}){
                if(!pc)continue;if(pc==0x261b26)w(0x261b1fu,c+0x84u,0,2);w(pc,c+off,0,1);}
            w(0x261b40u,c+0x30u,1,2);w(0x261b46u,c+0x40u,none);w(0x261b49u,c+0x3cu,none);
            w(0x261b4cu,c+0x60u,0);w(0x261b51u,c+0x7cu,10,2);w(0x261b57u,c+0x90u,none);
            w(0x261b5du,c+0x94u,none);w(0x261b63u,c+0x9au,none,2);reset(id);a.xmm[0]={};}
        finish(id,4);return;
    }
    if(e==0x261d50u){
        const U actor_id=in[0],body=actor(actor_id),script=U(sign16(in[2]));gp[3]=r(sp+4u);gp[4]=sp-8u;
        const auto id=call(0x261d72u,0x261a90u,gp,{script});
        if(id!=none){auto link=body+0x858u;while(r(link)!=none)link=command(r(link))+0x3cu;
            w(0x261dbbu,link,id);if(r(body+0x858u)==id)w(0x261dc2u,body+0x85cu,id);associated(actor_id,id);}
        finish(id,4);return;
    }
    if(e==0x262190u){
        const U joint_id=r(sp+4u),actor_id=r(sp+8u),joint=row(0x50eedcu,joint_id,0x8cu);bool remaining=false;
        for(U i=0;sign16(i)<sign16(r(joint+0xau,2));++i){const auto item=joint+U(sign16(i))*12u,id=r(item+0x14u);if(id==none)continue;
            const auto c=command(id),owner=r(item+0x10u);
            if(owner==actor_id){w(0x2621efu,c+0x40u,none);w(0x2621f6u,item+0x10u,none);w(0x2621fdu,item+0x14u,none);}
            else if(owner!=none){remaining=true;w(0x262210u,c+0x7eu,1,1);}}
        if(!remaining){gp[4]=sp-8u;const U thread=r(joint+0xcu);if(thread!=none)retire_thread(0x262234u,thread,gp);
            gp[0]=joint_id;gp[2]=r(0x50eedcu);call(0x262243u,0x16e8a0u,gp);}finish(a.gp[0],8);return;
    }
    if(e==0x262250u){
        const U actor_id=in[1],id=r(sp+4u),body=actor(actor_id),c=command(id);gp[4]=sp-16u;
        const auto joint=r(c+0x40u);if(joint!=none)call(0x262293u,0x262190u,gp,{joint,actor_id});
        else if(const auto thread=r(c+0x38u);thread!=none)retire_thread(0x2622a8u,thread,gp);
        auto link=body+0x858u;while(r(link)!=none){if(r(link)==id){w(0x2622e2u,link,r(c+0x3cu));break;}link=command(r(link))+0x3cu;}
        if(r(body+0x85cu)==id)w(0x2622ecu,body+0x85cu,none);gp[0]=id;gp[2]=r(0x50eee0u);
        call(0x2622fau,0x16e8a0u,gp);finish(a.gp[0],4);return;
    }
    if(e==0x261be0u){
        const U actor_id=r(sp+4u),script=r(sp+8u),thread=in[0],body=actor(actor_id),current=r(0x50ef00u);gp[4]=sp-16u;
        if(current!=none && native_campaign_ai_actor_tu15(m,current)==actor_id){
            for(auto id=r(body+0x858u);id!=none;){const auto c=command(id);w(0x261c40u,c+0x7eu,1,1);id=r(c+0x3cu);}
            gp[0]=actor_id;gp[2]=script;const auto id=call(0x261c51u,0x261d50u,gp,{thread});finish(id,8);return;
        }
        while(r(body+0x858u)!=none){gp[1]=actor_id;call(0x261c6eu,0x262250u,gp,{r(body+0x858u)});}
        gp[3]=thread;const auto id=call(0x261c81u,0x261a90u,gp,{U(sign16(script))});
        w(0x261c8bu,body+0x858u,id);w(0x261c91u,body+0x85cu,id);
        if(id!=none){call(0x261ca0u,0x262870u,gp,{actor_id,0x2629d0u,id});associated(actor_id,id);}
        finish(id,8);return;
    }
    if(e==0x262870u){
        const U actor_id=r(sp+4u),callback=r(sp+8u),context=r(sp+12u),body=actor(actor_id);gp[4]=sp-16u;
        if(!r(body+7u,1)){const auto value=call(0x2629b0u,callback,gp,{actor_id,r(body+0x18u),body+0x864u,context},4);
            w(0x2629b4u,body+0x868u,value,2);finish(value,12);return;}
        const auto perception=r(body+0x1cu);if(perception==none){finish(2,12);return;}
        auto object=r(row(0x510fa8u,perception,0x34u)+0x18u);U total{},ones{},twos{};gp[4]=sp-24u;
        while(object!=none){const auto p=r(row(0x4ecbd0u,object,12u)+8u),old=object,flags=r(p+0x134u);
            const auto ext=p+U(sign16(r(p+0x13au,2)));object=!flags&&ext?r(ext+0xcu):none;if(flags||!ext)continue;
            const auto value=call(0x262929u,callback,gp,{actor_id,old,ext+0x28u,context},4);w(0x262931u,ext+0x2cu,value,2);
            if(std::uint16_t(value)==2u)++twos;else if(std::uint16_t(value)==1u)++ones;++total;
        }
        // Both operands are signed16 counts; all real pools are far below the
        // exact binary32 integer range. The source scalar is retained DATA.
        U host{},csr=a.floating_point?a.floating_point->mxcsr:0x1f80u;float sum{},limit{},factor=std::bit_cast<float>(r(0x456910u));
        const S sum_i=sign16(ones)+sign16(twos),total_i=sign16(total);unsigned char below{};
        asm volatile("stmxcsr %[host]\n\tldmxcsr %[csr]\n\tcvtsi2ssl %[sum_i],%[sum]\n\tcvtsi2ssl %[total_i],%[limit]\n\tmulss %[factor],%[limit]\n\tcomiss %[limit],%[sum]\n\tsetb %[below]\n\tstmxcsr %[csr]\n\tldmxcsr %[host]"
            :[host]"=m"(host),[csr]"+m"(csr),[sum]"=&x"(sum),[limit]"=&x"(limit),[below]"=qm"(below)
            :[sum_i]"r"(sum_i),[total_i]"r"(total_i),[factor]"x"(factor):"memory","cc");
        if(a.floating_point)a.floating_point->mxcsr=csr;finish(below?0u:U(sign16(twos)>sign16(ones))+1u,12);return;
    }
    if(e==0x2801d0u){
        NativeCampaignScriptBank bank(m);auto next=native_campaign_script_tu15::ai_reference_actor_datums(bank,in[1]);gp[4]=sp-0x38u;
        for(auto actor=next();actor.row;actor=next()){
            gp[0]=none;call(0x2801efu,0x261be0u,gp,{actor.datum,in[7]});}
        finish(a.gp[0]);return;
    }
    if(e==0x280400u){
        const U script=r(sp+4u);NativeCampaignScriptBank bank(m);
        auto next=native_campaign_script_tu15::ai_reference_actors(bank,in[1]);
        for(auto body=next();body;body=next()){
            const U id=r(body+0x85cu);if(id!=none && r(command(id)+0x34u)==script){finish(1,4);return;}}
        finish(0,4);return;
    }
    throw std::invalid_argument("Unknown native command lifecycle entry");
}
}
void native_campaign_ai_command_lifecycle_call_tu15(SourcePhysicsContactMemory &m,U entry,SourcePhysicsWorldRuntimeAbi &a,
    const NativeCampaignAiCommandLifecycleServicesTu15 &s,const SourcePhysicsContactRecordObserver &o){Lifecycle{m,a,s,o}.run(entry);}
void native_campaign_ai_command_reset_tu15(SourcePhysicsContactMemory &m,U id,const SourcePhysicsContactRecordObserver &o){
    SourcePhysicsWorldRuntimeAbi a{};Lifecycle{m,a,{},o}.reset(id);
}
}
