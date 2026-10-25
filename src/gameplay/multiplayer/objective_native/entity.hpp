#pragma once

namespace halo2::source_objective_native_tu15_detail {
template<class State,class Dispatch>bool entity_initialize(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24c680u,s.gp[6]);s.gp[6]=read(s,0x24c681u,s.gp[4]+0xcu);s.gp[2]=s.gp[1];
    push(s,0x24c687u,s.gp[7]);s.gp[0]=0;s.flags.logic(0,4);s.gp[1]=0x21u;s.gp[7]=s.gp[6];
    for(;s.gp[1];--s.gp[1]) {
        store(s,0x24c691u,s.gp[7],4,0);s.gp[7]+=(s.other_flags&1024u)?0xfffffffcu:4u;
    }
    s.gp[0]=read(s,0x24c693u,s.gp[2]);push(s,0x24c695u,s.gp[6]);s.gp[1]=s.gp[2];
    const auto shared=read(s,0x24c698u,s.gp[0]+0xa4u);call(s,dispatch,shared,0x24c698u,0x24c69eu);
    s.gp[1]=0xffffffffu;s.flags.logic(s.gp[1],4);call(s,dispatch,0x164930u,0x24c6a1u,0x24c6a6u);
    store(s,0x24c6a6u,s.gp[6]+0x24u,4,s.gp[0]);s.gp[1]=0xffffffffu;s.gp[0]=0;s.flags.logic(0,4);
    store(s,0x24c6aeu,s.gp[6]+0x28u,4,0);s.gp[2]=s.gp[6]+0x2cu;
    constexpr std::array<std::uint32_t,4> owner_stores{0x24c6b4u,0x24c6b6u,0x24c6b9u,0x24c6bcu};
    for(unsigned n=0;n<4;++n)store(s,owner_stores[n],s.gp[2]+n*4u,4,s.gp[1]);
    store(s,0x24c6bfu,s.gp[2]+0x10u,2,s.gp[1]);s.gp[1]=0xffffffffu;s.flags.logic(s.gp[1],4);s.gp[2]=s.gp[6]+0x5cu;
    constexpr std::array<std::uint32_t,9> datum_stores{
        0x24c6c9u,0x24c6cbu,0x24c6ceu,0x24c6d1u,0x24c6d4u,0x24c6d7u,0x24c6dau,0x24c6ddu,0x24c6e0u};
    for(unsigned n=0;n<9;++n)store(s,datum_stores[n],s.gp[2]+n*4u,4,s.gp[1]);
    cursor(s,0x24c6e3u);s.gp[7]=s.pop();store(s,0x24c6e4u,s.gp[6]+0x80u,1,0);
    cursor(s,0x24c6eau);s.gp[6]=s.pop();cursor(s,0x24c6ebu);s.pop();s.gp[4]+=8u;
    ++source_objective_native_tu15_stats.owners[19];return true;
}
template<class State,class Dispatch>bool entity_difference(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24c6f0u,s.gp[1]);push(s,0x24c6f1u,s.gp[3]);s.gp[3]=read(s,0x24c6f2u,s.gp[4]+0xcu);
    s.gp[2]=read(s,0x24c6f6u,s.gp[3]);push(s,0x24c6f8u,s.gp[6]);s.gp[6]=read(s,0x24c6f9u,s.gp[4]+0x18u);
    s.gp[0]=0;s.gp[2]&=31u;s.flags.logic(s.gp[2],4);push(s,0x24c702u,s.gp[7]);store(s,0x24c703u,s.gp[4]+0xcu,4,0);
    if(s.gp[2]) {
        s.gp[0]=read(s,0x24c709u,s.gp[1]);push(s,0x24c70bu,s.gp[6]);s.gp[7]=s.gp[4]+0x10u;
        push(s,0x24c710u,s.gp[7]);push(s,0x24c711u,s.gp[2]);const auto shared=read(s,0x24c712u,s.gp[0]+0xa8u);
        call(s,dispatch,shared,0x24c712u,0x24c718u);s.gp[0]=read(s,0x24c718u,s.gp[4]+0xcu);
    }
    const auto side_mask=read(s,0x24c71cu,s.gp[3],1);s.flags.logic(side_mask&0x20u,1);s.gp[1]=read(s,0x24c71fu,0x52d710u);
    if(side_mask&0x20u) {
        s.gp[2]=read(s,0x24c727u,s.gp[1]+0x1f4u);const auto previous=read(s,0x24c72du,s.gp[6]+0x24u);s.flags.minus(previous,s.gp[2],4);
        if(previous!=s.gp[2]) {
            s.gp[0]|=0x20u;s.flags.logic(s.gp[0],4);store(s,0x24c735u,s.gp[6]+0x24u,4,s.gp[2]);
            store(s,0x24c738u,s.gp[4]+0xcu,4,s.gp[0]);
        }
    }
    // The three nine-team groups preserve original scalar widths and publish
    // their mask after every changed field, including aliased caller storage.
    const auto word_group=[&](std::uint32_t source,std::uint32_t destination,std::uint32_t mask,std::uint32_t pc) {
        for(unsigned team=0;team<9;++team) {
            const auto instruction=pc+team*0x1au;
            s.gp[2]=(s.gp[2]&0xffff0000u)|read(s,instruction,s.gp[1]+source+team*2u,2);
            const auto previous=read(s,instruction+7u,s.gp[6]+destination+team*2u,2);s.flags.minus(previous,s.gp[2]&65535u,2);
            if(previous!=std::uint16_t(s.gp[2])) {
                s.gp[0]|=mask;s.flags.logic(s.gp[0],4);store(s,instruction+18u,s.gp[6]+destination+team*2u,2,s.gp[2]);
                store(s,instruction+22u,s.gp[4]+0xcu,4,s.gp[0]);
            }
        }
    };
    const auto owner_mask=read(s,0x24c73cu,s.gp[3],1);s.flags.minus(owner_mask,0,1);
    if(owner_mask&0x80u)word_group(0x114u,0x2cu,0x80u,0x24c745u);
    s.gp[2]=read(s,0x24c82fu,s.gp[3]);s.flags.logic((s.gp[2]>>8u)&1u,1);
    if(s.gp[2]&0x100u)word_group(0x1a0u,0x3eu,0x100u,0x24c83au);
    s.gp[2]=read(s,0x24c924u,s.gp[3]);s.flags.logic((s.gp[2]>>8u)&2u,1);
    if(s.gp[2]&0x200u) {
        for(unsigned team=0;team<9;++team) {
            const auto pc=0x24c92fu+team*23u;s.gp[2]=(s.gp[2]&0xffffff00u)|read(s,pc,s.gp[1]+0x170u+team,1);
            const auto previous=read(s,pc+6u,s.gp[6]+0x50u+team,1);s.flags.minus(previous,s.gp[2]&255u,1);
            if(previous!=std::uint8_t(s.gp[2])) {
                s.gp[0]|=0x200u;s.flags.logic(s.gp[0],4);store(s,pc+16u,s.gp[6]+0x50u+team,1,s.gp[2]);
                store(s,pc+19u,s.gp[4]+0xcu,4,s.gp[0]);
            }
        }
    }
    s.gp[2]=read(s,0x24c9feu,s.gp[3]);s.flags.logic((s.gp[2]>>8u)&4u,1);
    if(s.gp[2]&0x400u) {
        s.gp[2]=s.gp[6]+0x60u;s.gp[6]=s.gp[1]+0x180u;s.gp[7]=3;
        constexpr std::array<std::uint32_t,3> load_pc{0x24ca13u,0x24ca36u,0x24ca56u};
        constexpr std::array<std::uint32_t,3> compare_pc{0x24ca25u,0x24ca47u,0x24ca68u};
        constexpr std::array<std::uint32_t,3> store_pc{0x24ca2fu,0x24ca50u,0x24ca72u};
        constexpr std::array<std::uint32_t,3> mask_pc{0x24ca32u,0x24ca52u,0x24ca75u};
        for(unsigned row=0;row<3;++row) {
            for(unsigned field=0;field<3;++field) {
                const auto offset=field*4u-4u;s.gp[1]=read(s,load_pc[field],s.gp[6]+offset);
                s.gp[1]=source_objective_entity_datum(s.gp[1]);s.flags.logic(s.gp[1],4);
                const auto previous=read(s,compare_pc[field],s.gp[2]+offset);s.flags.minus(previous,s.gp[1],4);
                if(previous!=s.gp[1]) {
                    s.gp[0]|=0x400u;s.flags.logic(s.gp[0],4);store(s,store_pc[field],s.gp[2]+offset,4,s.gp[1]);
                    store(s,mask_pc[field],s.gp[4]+0xcu,4,s.gp[0]);
                }
            }
            s.gp[6]+=12u;s.gp[2]=s.flags.plus(s.gp[2],12,4);s.gp[7]=s.flags.increment(s.gp[7],4,true);
        }
    }
    cursor(s,0x24ca82u);s.gp[7]=s.pop();cursor(s,0x24ca83u);s.gp[6]=s.pop();store(s,0x24ca84u,s.gp[3],4,s.gp[0]);
    cursor(s,0x24ca86u);s.gp[3]=s.pop();cursor(s,0x24ca87u);s.gp[1]=s.pop();cursor(s,0x24ca88u);s.pop();s.gp[4]+=12u;
    ++source_objective_native_tu15_stats.owners[20];return true;
}
template<class State,class Dispatch>bool entity_apply(State& s,Dispatch&& dispatch) {
    if(!admitted(s))return false;
    push(s,0x24ca90u,s.gp[1]);push(s,0x24ca91u,s.gp[3]);s.gp[3]=read(s,0x24ca92u,s.gp[4]+0xcu);
    s.gp[0]=s.gp[3]&31u;s.flags.logic(s.gp[0],4);push(s,0x24ca9bu,s.gp[6]);s.gp[6]=read(s,0x24ca9cu,s.gp[4]+0x18u);
    store(s,0x24caa0u,s.gp[4]+0xbu,1,1);
    if(s.gp[0]) {
        s.gp[2]=read(s,0x24caa7u,s.gp[1]);push(s,0x24caa9u,s.gp[6]);push(s,0x24caaau,s.gp[0]);
        const auto shared=read(s,0x24caabu,s.gp[2]+0xacu);call(s,dispatch,shared,0x24caabu,0x24cab1u);
        s.flags.logic(s.gp[0]&255u,1);store(s,0x24cab3u,s.gp[4]+0xbu,1,(s.gp[0]&255u)!=0);
    }
    s.flags.logic(s.gp[3]&0x20u,1);s.gp[0]=read(s,0x24cabbu,0x52d710u);
    if(s.gp[3]&0x20u) {
        s.gp[1]=read(s,0x24cac2u,s.gp[6]+0x24u);const auto previous=read(s,0x24cac5u,s.gp[0]+0x1f4u);
        s.flags.minus(previous,s.gp[1],4);
        if(previous!=s.gp[1]) {
            push(s,0x24cacdu,s.gp[1]);call(s,dispatch,0x24c5d0u,0x24caceu,0x24cad3u);s.gp[0]=read(s,0x24cad3u,0x52d710u);
        }
    }
    const auto words=[&](std::uint32_t from,std::uint32_t to,std::uint32_t first,unsigned first_register) {
        for(unsigned team=0;team<9;++team) {
            const auto reg=(team&1u)?3u-first_register:first_register;const auto pc=first+team*11u;
            s.gp[reg]=(s.gp[reg]&0xffff0000u)|read(s,pc,s.gp[6]+from+team*2u,2);
            store(s,pc+4u,s.gp[0]+to+team*2u,2,s.gp[reg]);
        }
    };
    s.flags.logic(s.gp[3]&255u,1);
    if(s.gp[3]&0x80u)words(0x2cu,0x114u,0x24cadcu,1);
    s.flags.logic((s.gp[3]>>8u)&1u,1);
    if(s.gp[3]&0x100u)words(0x3eu,0x1a0u,0x24cb44u,2);
    s.flags.logic((s.gp[3]>>8u)&2u,1);
    if(s.gp[3]&0x200u) {
        for(unsigned team=0;team<9;++team) {
            const auto reg=(team&1u)?2u:1u;const auto pc=0x24cbacu+team*9u;
            s.gp[reg]=(s.gp[reg]&0xffffff00u)|read(s,pc,s.gp[6]+0x50u+team,1);
            store(s,pc+3u,s.gp[0]+0x170u+team,1,s.gp[reg]);
        }
    }
    s.flags.logic((s.gp[3]>>8u)&4u,1);
    if(s.gp[3]&0x400u) {
        s.gp[2]=read(s,0x24cc06u,0x4f54f4u);push(s,0x24cc0cu,s.gp[5]);push(s,0x24cc0du,s.gp[7]);
        s.gp[3]=s.gp[0]+0x180u;s.gp[7]=s.gp[6]+0x60u;s.gp[5]=3;
        // Resolve snapshot indices against the actual live player pool. A
        // nonzero salt is required; absent, negative and out-of-range rows
        // retain NONE. No recorded handle or admission result is supplied.
        const auto player=[&](unsigned field) {
            constexpr std::array<std::uint32_t,3> load{0x24cc20u,0x24cc67u,0x24ccacu};
            constexpr std::array<std::uint32_t,3> capacity{0x24cc2eu,0x24cc74u,0x24ccbau};
            constexpr std::array<std::uint32_t,3> stride{0x24cc33u,0x24cc79u,0x24ccbfu};
            constexpr std::array<std::uint32_t,3> rows{0x24cc39u,0x24cc7fu,0x24ccc5u};
            constexpr std::array<std::uint32_t,3> alive{0x24cc3cu,0x24cc82u,0x24ccc8u};
            constexpr std::array<std::uint32_t,3> final_stride{0x24cc52u,0x24cc98u,0x24ccdeu};
            constexpr std::array<std::uint32_t,3> final_rows{0x24cc55u,0x24cc9bu,0x24cce1u};
            constexpr std::array<std::uint32_t,3> salt{0x24cc5bu,0x24cca1u,0x24cce7u};
            constexpr std::array<std::uint32_t,3> publish{0x24cc64u,0x24ccaau,0x24ccf0u};
            const auto input=field?0u:1u,output=field?1u:0u,offset=field*4u-4u;
            s.gp[input]=read(s,load[field],s.gp[7]+offset);s.gp[6]=0;s.flags.minus(s.gp[input],0xffffffffu,4);
            const auto index=std::bit_cast<std::int32_t>(s.gp[input]);
            if(index!=-1) {
                s.flags.logic(s.gp[input],4);
                if(index>=0) {
                    const auto count=read(s,capacity[field],s.gp[2]+0x38u);s.flags.minus(s.gp[input],count,4);
                    if(index<std::bit_cast<std::int32_t>(count)) {
                        s.gp[output]=read(s,stride[field],s.gp[2]+0x24u)*s.gp[input];
                        s.gp[output]=s.flags.plus(s.gp[output],read(s,rows[field],s.gp[2]+0x44u),4);
                        const auto generation=read(s,alive[field],s.gp[output],2);s.flags.minus(generation,0,2);
                        if(generation)s.gp[6]=s.gp[output];
                    }
                }
            }
            s.gp[output]=0xffffffffu;s.flags.logic(s.gp[6],4);
            if(s.gp[6]) {
                s.flags.minus(s.gp[input],0xffffffffu,4);
                s.gp[output]=read(s,final_stride[field],s.gp[2]+0x24u);s.gp[6]=read(s,final_rows[field],s.gp[2]+0x44u);
                s.gp[output]*=s.gp[input];
                const auto generation=signed_half(read(s,salt[field],s.gp[output]+s.gp[6],2));
                s.gp[output]=source_objective_player_datum(s.gp[input],std::int16_t(generation));s.flags.logic(s.gp[output],4);
            }
            store(s,publish[field],s.gp[3]+offset,4,s.gp[output]);
        };
        for(unsigned row=0;row<3;++row) {
            for(unsigned field=0;field<3;++field)player(field);
            s.gp[7]+=12u;s.gp[3]=s.flags.plus(s.gp[3],12,4);s.gp[5]=s.flags.increment(s.gp[5],4,true);
        }
        cursor(s,0x24cd00u);s.gp[7]=s.pop();cursor(s,0x24cd01u);s.gp[5]=s.pop();
    }
    s.gp[0]=(s.gp[0]&0xffffff00u)|read(s,0x24cd02u,s.gp[4]+0xbu,1);
    cursor(s,0x24cd06u);s.gp[6]=s.pop();cursor(s,0x24cd07u);s.gp[3]=s.pop();cursor(s,0x24cd08u);s.gp[1]=s.pop();
    cursor(s,0x24cd09u);s.pop();s.gp[4]+=12u;++source_objective_native_tu15_stats.owners[21];return true;
}
} // namespace halo2::source_objective_native_tu15_detail
