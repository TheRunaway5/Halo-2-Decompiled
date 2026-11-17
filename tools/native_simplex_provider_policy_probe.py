"""Owned compile-time negative control for the two simplex parent entry guards."""
import hashlib
import subprocess

GUARD='    if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;\n'

def check_guard(root,scratch,base,owner):
    header='physics/runtime/native_tu15_simplex_'+owner+'_source_owner.hpp'
    original=(root/'src'/header).read_text()
    if original.count(GUARD)!=1:raise ValueError('Exact required-owner guard mutation seam drift: '+owner)
    probe=scratch/('policy-'+owner+'.cpp')
    probe.write_text('''#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "'''+header+'''"
#include <iostream>
using namespace halo2;
struct Memory:SourcePhysicsContactMemory {
    mutable unsigned accesses{};
    void read(std::uint32_t,std::span<std::byte>)const override{++accesses;throw std::runtime_error("Unexpected guard read");}
    void write(std::uint32_t,std::span<const std::byte>)override{++accesses;throw std::runtime_error("Unexpected guard write");}
};
int main(){
    asm volatile("fninit");_mm_setcsr(0x1f80u);
    Memory memory;SourcePhysicsContactRecordObserver observer;SourcePhysicsWorldRuntimeServices services;
    physics_world_runtime_detail::WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    if(!native_simplex_'''+owner+'''_owner_detail::admitted(state))return 40;
    unsigned calls{};services.required_owner=[&](auto&,const auto&,auto&,const auto&){++calls;throw std::runtime_error("Uninvoked unknown owner");};
    const auto accepted=native_simplex_'''+owner+'''_owner_detail::admitted(state);
    if(memory.accesses||calls)return 42;
    if(accepted){std::cerr<<"uncertified owner admitted";return 41;}
    return 0;
}
''')
    overlay=scratch/('policy-mutant-'+owner);mutant=overlay/header;mutant.parent.mkdir(parents=True);mutant.write_text(original.replace(GUARD,''))
    results={}
    for name,includes,expected in [('baseline',[],0),('omitted-provider-guard',['-I'+str(overlay)],41)]:
        binary=scratch/('policy-'+owner+'-'+name)
        command=[base[0],*includes,*base[1:],str(probe),'-o',str(binary)]
        built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Required-owner guard probe compile failed: '+built.stderr[-5000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True)
        if result.returncode!=expected or(name!='baseline'and result.stderr!='uncertified owner admitted'):
            raise ValueError('Required-owner guard negative control failed: '+name+': '+result.stderr[-2000:])
        results[name]={'returncode':result.returncode,'diagnostic':result.stderr,'command':command}
    return dict(parent=owner,scope='Pure whole-entry admission: empty provider accepted, unknown provider refused before any provider call or DATA access. The compiled omitted-guard mutant must admit it and be rejected.',probe_sha256=hashlib.sha256(probe.read_bytes()).hexdigest(),mutant_header_sha256=hashlib.sha256(mutant.read_bytes()).hexdigest(),results=results)
