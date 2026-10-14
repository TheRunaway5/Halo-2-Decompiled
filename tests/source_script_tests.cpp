#include "halo2/source_script.hpp"
#include "halo2/scripts.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace halo2;
namespace {
struct ProgramBuilder {
    ScriptProgram program;
    std::uint32_t add(ScriptNode node) {
        const auto index=std::uint32_t(program.nodes.size());
        node.salt=std::uint16_t(0xe000+index);
        program.nodes.push_back(node);
        return (std::uint32_t(node.salt)<<16)|index;
    }
    std::uint32_t integer(int value) {
        return add({0,8,8,9,null_datum,0,std::uint32_t(value)});
    }
    std::uint32_t call(const std::string &name,
                       std::vector<std::uint32_t> children,std::uint16_t type=4) {
        const auto offset=std::uint32_t(program.strings.size());
        program.strings+=name;program.strings.push_back('\0');
        for(std::size_t i=0;i<children.size();++i)
            program.nodes[children[i]&65535].next=i+1<children.size()?children[i+1]:null_datum;
        const auto function=add({0,0,2,9,children.empty()?null_datum:children[0],offset,0});
        return add({0,0,type,8,null_datum,offset,function});
    }
};
void begin_random_dispatch() {
    ProgramBuilder builder;
    std::vector<std::uint32_t> children;
    for(unsigned child=0;child<4;++child) {
        const auto begin=builder.call("begin",{
            builder.call("record",{builder.integer(int(child))}),
            builder.call("sleep",{builder.integer(2)}),
            builder.call("random_range",{builder.integer(0),builder.integer(100)},7)},7);
        children.push_back(begin);
    }
    const auto randomized=builder.call("begin_random",std::move(children),8);
    builder.program.scripts.push_back({"source_random",0,4,
                                      builder.call("result",{randomized})});
    std::uint32_t state=0;unsigned tick=0,choices=0,inner=0,result=0;
    std::vector<std::array<unsigned,3>> records;
    ScriptVm vm(builder.program,[&](const std::string &name,const auto &args)->ScriptValue {
        if(name=="record")records.push_back({tick,args.at(0).bits,state});
        else if(name=="random_range") {
            ++inner;
            return {7,source_hs_random_range(state,std::int16_t(args.at(0).bits),
                                           std::int16_t(args.at(1).bits)),{}};
        } else if(name=="result")result=args.at(0).bits;
        else throw std::runtime_error("unexpected fixture service");
        return {};
    });
    vm.set_random_choice([&](unsigned count,std::uint32_t visited) {
        ++choices;return source_hs_begin_random(state,count,visited);
    });
    vm.start_level();
    for(tick=0;tick<10;++tick)vm.tick(tick);
    const std::vector<std::array<unsigned,3>> expected{
        {0,0,0x3c6ef35f},{2,3,0xd1ccf6e9},{4,1,0x6252e503},{6,2,0x57fe6c2d}};
    if(!vm.diagnostics().empty() || records!=expected || choices!=5 || inner!=4 ||
       result!=64 || state!=0x81fdbee7)
        throw std::runtime_error("begin_random must retain visited bits across sleeps and interleave primary draws");
}
void object_list_conversions() {
    std::array<std::uint32_t,640> names;names.fill(null_datum);
    names[0]=0xa1000000u;names[639]=0xa2000280u;
    unsigned allocated=0,appended=0;std::uint32_t seen_list{},seen_object{};
    SourceHsObjectListServices lists{
        [&]{++allocated;return 0xb1000000u;},
        [&](auto list,auto object){++appended;seen_list=list;seen_object=object;}};
    for(const auto name:{-32768,-1,640,32767})
        if(source_hs_named_object_list(std::int16_t(name),names,{})!=null_datum)
            throw std::runtime_error("out-of-range source name produced an object list");
    if(source_hs_named_object_list(1,names,{})!=null_datum ||
       source_hs_object_list(null_datum,{})!=null_datum || allocated || appended)
        throw std::runtime_error("absent object allocated a source list");
    for(const auto name:{0,639}) {
        if(source_hs_named_object_list(std::int16_t(name),names,lists)!=0xb1000000u ||
           seen_list!=0xb1000000u || seen_object!=names[unsigned(name)])
            throw std::runtime_error("source named list lost the full current object handle");
    }
    bool rejected=false;
    try{source_hs_object_list(names[0],{});}catch(const std::invalid_argument&){rejected=true;}
    if(!rejected)throw std::runtime_error("live object conversion silently supplied a list pool");
    SourceHsObjectListServices exhausted{[]{return null_datum;},
        [&](auto list,auto object){seen_list=list;seen_object=object;}};
    if(source_hs_object_list(names[0],exhausted)!=null_datum ||
       seen_list!=null_datum || seen_object!=names[0])
        throw std::runtime_error("original allocation failure must still reach the append service");

    //Actual compiled name/object operand types converted before their host
    //call. The callback supplies explicit fixture storage, not production GC.
    for(unsigned type=50;type<=61;++type) {
        ProgramBuilder b;
        const auto operand=b.add({0,std::uint16_t(type),31,9,null_datum,0,
                                  type>=56?639u:names[639]});
        b.program.scripts.push_back({"source_list",0,4,b.call("take_list",{operand})});
        unsigned calls=0,resolves=0,allocation_count=0;
        ScriptVm vm(b.program,[&](const auto &name,const auto &args)->ScriptValue {
            if(name!="take_list" || args.size()!=1 || args[0].type!=31 || args[0].bits!=0xb2000007u)
                throw std::runtime_error("VM did not supply a typed source object list");
            ++calls;return {};
        },[&](int name,unsigned target) {
            if(name!=639 || target!=50)throw std::runtime_error("VM list resolved a different name/type");
            ++resolves;return names[639];
        },{},SourceHsObjectListServices{
            [&]{++allocation_count;return 0xb2000007u;},
            [&](auto list,auto object){
                if(list!=0xb2000007u || object!=names[639])
                    throw std::runtime_error("VM changed the source list append inputs");
            }});
        vm.start_level();vm.tick(0);
        if(!vm.diagnostics().empty() || calls!=1 || allocation_count!=1 || resolves!=(type>=56))
            throw std::runtime_error("VM object-list scalar conversion failed");
    }
}
void freed_syntax_records() {
    ProgramBuilder builder;const auto root=builder.call("record",{});
    builder.program.scripts.push_back({"retained_pool",0,4,root});
    builder.program.nodes.push_back({0,0,1,0,0xe3740001u,0,0xdeadbeefu});
    unsigned calls=0;
    ScriptVm vm(builder.program,[&](const auto &,const auto &)->ScriptValue{++calls;return {};});
    if(vm.audit().active!=2)throw std::runtime_error("free syntax payload treated as allocated");
    vm.start_level();vm.tick(0);
    if(calls!=1 || !vm.diagnostics().empty())throw std::runtime_error("freed syntax record prevented live root dispatch");
    builder.program.scripts[0].root=2;
    bool rejected=false;try{ScriptVm stale(builder.program,{});}catch(const FormatError&){rejected=true;}
    if(!rejected)throw std::runtime_error("zero-salt syntax reference was accepted");
}
void nonpositive_sleep() {
    ProgramBuilder builder;
    for(const auto delay:{-32768,-7,-2,-1,0,65535}) {
        const auto id=unsigned(builder.program.scripts.size());
        const auto root=builder.call("begin",{
            builder.call("sleep",{builder.integer(delay)}),
            builder.call("record",{builder.integer(int(id))}),
            builder.call("sleep",{builder.integer(2)}),
            builder.call("record",{builder.integer(int(id+100))})});
        builder.program.scripts.push_back({"source_sleep_"+std::to_string(id),0,4,root});
    }
    unsigned tick=0;std::vector<std::array<unsigned,2>> records;
    ScriptVm vm(builder.program,[&](const auto &name,const auto &args)->ScriptValue {
        if(name!="record")throw std::runtime_error("unexpected sleep fixture service");
        records.push_back({tick,args.at(0).bits});return {};
    });
    vm.start_level();
    for(tick=0;tick<4;++tick)vm.tick(tick);
    std::vector<std::array<unsigned,2>> expected;
    for(unsigned id=0;id<6;++id)expected.push_back({0,id});
    for(unsigned id=0;id<6;++id)expected.push_back({2,id+100});
    if(!vm.diagnostics().empty() || records!=expected || vm.running())
        throw std::runtime_error("Original sleep ignores nonpositive short delays and continues dispatch");

    // Nonpositive external sleeps leave the target's existing wake intact.
    ProgramBuilder target;
    target.program.scripts.push_back({"sleeping_target",0,4,target.call("begin",{
        target.call("sleep",{target.integer(100)}),target.call("record",{target.integer(9)})})});
    const auto script=target.add({0,21,21,9,null_datum,0,0});
    target.program.scripts.push_back({"caller",1,4,target.call("begin",{
        target.call("sleep",{target.integer(-1),script}),
        target.call("sleep",{target.integer(0),target.add({0,21,21,9,null_datum,0,0})}),
        target.call("record",{target.integer(1)})})});
    records.clear();
    ScriptVm other(target.program,[&](const auto &,const auto &args)->ScriptValue {
        records.push_back({tick,args.at(0).bits});return {};
    });
    tick=0;other.start_level();other.tick(tick);
    other.start("caller");tick=1;other.tick(tick);
    tick=99;other.tick(tick);tick=100;other.tick(tick);
    if(!other.diagnostics().empty() || records!=std::vector<std::array<unsigned,2>>{{1,1},{100,9}})
        throw std::runtime_error("Nonpositive external sleep changed the target's wake");
}
void host_failure_context() {
    ProgramBuilder builder;
    builder.program.scripts.push_back({"campaign_start",0,4,builder.call("device_set_position_immediate",{})});
    ScriptVm vm(builder.program,[](const auto &,const auto &)->ScriptValue {
        throw std::out_of_range("missing device owner");
    });
    vm.start_level();vm.tick(0);
    if(vm.diagnostics().size()!=1 || vm.diagnostics().front()!=
        "campaign_start: builtin 'device_set_position_immediate': missing device owner" ||
        vm.command_actor() || vm.running())
        throw std::runtime_error("host failure lost its campaign builtin context or kept dispatch alive");
    vm.tick(1);
    if(vm.diagnostics().size()!=1)throw std::runtime_error("faulted host call ran again");
}
}
int main(int argc,char **argv) {
    if(argc==2 && std::string(argv[1])=="--object-list-probe") {
        unsigned named{};std::uint32_t value{},object{},allocation{};
        while(std::cin>>named>>value>>object>>allocation) {
            unsigned allocations=0,appends=0;std::uint32_t list_arg=null_datum,object_arg=null_datum;
            SourceHsObjectListServices services{[&]{++allocations;return allocation;},
                [&](auto list,auto handle){++appends;list_arg=list;object_arg=handle;}};
            std::array<std::uint32_t,640> names;names.fill(null_datum);
            const auto name=std::int16_t(value);
            if(name>=0 && name<640)names[unsigned(name)]=object;
            const auto result=named?source_hs_named_object_list(name,names,services):source_hs_object_list(value,services);
            std::cout<<result<<' '<<allocations<<' '<<appends<<' '<<list_arg<<' '<<object_arg<<'\n';
        }
        return 0;
    }
    if(argc==2 && std::string(argv[1])=="--random-probe") {
        std::uint32_t state{};int low{},high{};
        while(std::cin>>state>>low>>high) {
            if(low<-32768 || low>32767 || high<-32768 || high>32767)return 2;
            const auto value=source_hs_random_range(state,std::int16_t(low),std::int16_t(high));
            std::cout<<state<<' '<<value<<'\n';
        }
        return 0;
    }
    if(argc==2 && std::string(argv[1])=="--begin-random-probe") {
        std::uint32_t state{},visited{};unsigned count{};
        while(std::cin>>state>>count>>visited) {
            const auto value=source_hs_begin_random(state,count,visited);
            std::cout<<state<<' '<<(value?int(*value):-1)<<'\n';
        }
        return 0;
    }
    std::uint32_t state=0;
    if(source_hs_random_range(state,60,360)!=130 || state!=0x3c6ef35f)
        throw std::runtime_error("retail random_range first recorded state/result");
    if(source_hs_random_range(state,0,2)!=0 || state!=0x47502932)
        throw std::runtime_error("retail random_range branch state/result");
    const auto prior=state;
    if(source_hs_random_range(state,-123,-123)!=std::uint16_t(-123) || state==prior)
        throw std::runtime_error("equal bounds still consume source RNG");
    state=0;
    if(source_hs_begin_random(state,0,0) || state!=0x3c6ef35f)
        throw std::runtime_error("empty begin_random still consumes a draw");
    if(source_hs_begin_random(state,32,0xffffffffu) || state!=0x47502932)
        throw std::runtime_error("complete begin_random still consumes a draw");
    if(source_hs_begin_random(state,32,0xfffffffeu)!=0)
        throw std::runtime_error("begin_random cyclic search wraps to only unvisited child");
    bool rejected=false;
    try {source_hs_begin_random(state,33,0);}catch(const std::invalid_argument&){rejected=true;}
    if(!rejected)throw std::runtime_error("unsupported original frame size rejected");
    begin_random_dispatch();
    object_list_conversions();
    freed_syntax_records();
    host_failure_context();
    nonpositive_sleep();
    std::cout<<"source HS primary RNG, resumable begin_random and explicit object-list fixtures passed\n";
}
