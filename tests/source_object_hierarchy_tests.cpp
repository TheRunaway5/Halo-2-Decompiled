#include "halo2/source_object_spatial_pool.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
constexpr auto none=0xffffffffu;
void check(bool value,const char* message){if(!value)throw FormatError(message);}
template<class F>void rejects(F action,const char* message){
    try{action();}catch(const FormatError&){return;}throw FormatError(message);
}
Words parse(std::string_view line){
    std::istringstream stream{std::string(line)};Words result;std::uint32_t word;
    while(stream>>std::hex>>word)result.push_back(word);
    if(!stream.eof())throw FormatError("invalid hierarchy fixture word");return result;
}
void append(Words& out,const SourceObjectRegistrationOwner& owner){
    out.insert(out.end(),{owner.full_handle,owner.parent,owner.child_head,owner.sibling,
        owner.parent_node,owner.primitive.header_flags,owner.primitive.object_flags,
        owner.location.leaf,std::uint32_t(std::uint16_t(owner.location.cluster))|
        (std::uint32_t(std::uint16_t(owner.location.bsp))<<16),
        std::uint32_t(std::int32_t(owner.header_cluster)),owner.reference_head});
}
Words probe(const Words& input){
    std::size_t at=0;auto take=[&](){
        if(at==input.size())throw FormatError("short hierarchy fixture");return input[at++];};
    const auto count=take();check(count>0 && count<=2048,"invalid hierarchy fixture owner count");
    auto journal=std::make_unique<SourceObjectRegistrationJournal>(1);
    std::vector<std::uint32_t> handles;std::uint64_t ordinal=0;
    for(unsigned i=0;i<count;++i){
        SourceObjectRegistrationOwner owner;owner.full_handle=take();owner.pool_rank=std::uint16_t(owner.full_handle);
        owner.parent=take();owner.child_head=take();owner.sibling=take();owner.parent_node=std::uint8_t(take());
        owner.primitive.header_flags=std::uint8_t(take());owner.primitive.object_flags=take();
        owner.location.leaf=take();const auto location=take();
        owner.location.cluster=std::int16_t(location);owner.location.bsp=std::int16_t(location>>16);
        owner.header_cluster=std::int16_t(take());owner.reference_head=take();
        journal->birth(++ordinal,owner);handles.push_back(owner.full_handle);
    }
    Words output;const auto operations=take();
    for(unsigned i=0;i<operations;++i){
        const auto kind=take();
        if(kind==0){const auto parent=take(),child=take(),node=take(),bsp=take();
            journal->attach_link(++ordinal,parent,child,std::int16_t(node),std::int16_t(bsp));}
        else if(kind==1)journal->detach_link(++ordinal,take());
        else throw FormatError("unknown hierarchy fixture operation");
        for(auto handle:handles)append(output,journal->owner(handle));
    }
    check(at==input.size(),"extra hierarchy fixture words");return output;
}
void controls(){
    auto journal=std::make_unique<SourceObjectRegistrationJournal>(1);
    ObjectLightSpatialBsp bsp;bsp.clusters.resize(1);std::uint64_t ordinal=0;
    const auto birth=[&](std::uint32_t handle){SourceObjectRegistrationOwner owner;
        owner.full_handle=handle;owner.pool_rank=std::uint16_t(handle);
        owner.primitive.type=2;owner.primitive.object_flags=0x200;
        owner.primitive.header_flags=0x0a;owner.location={17,0,3};owner.header_cluster=0;
        owner.primitive.primitive_sphere={{1,2,3},4};owner.rendering_sphere={{5,6,7},8};
        owner.root_position={9,10,11};journal->birth(++ordinal,owner);};
    for(unsigned i=0;i<6;++i)birth(0xe1000000u|i);
    const std::uint32_t parent=0xe1000000,child=parent+1,second=parent+2,third=parent+3,grandchild=parent+4;
    journal->register_owner(++ordinal,child,{17,0,3},bsp);
    const auto before=journal->events().size();
    rejects([&]{journal->attach_link(ordinal+1,parent,child,20,3);},"registered child attached without actual unregister");
    check(journal->events().size()==before && journal->collidable().cluster(0).size()==1,
        "rejected attachment changed registration order");
    journal->unregister(++ordinal,child);journal->attach_link(++ordinal,parent,child,20,3);
    journal->attach_link(++ordinal,parent,second,19,2);journal->attach_link(++ordinal,parent,third,0x123,1);
    journal->attach_link(++ordinal,child,grandchild,0,-1);
    check(journal->owner(parent).child_head==third && journal->owner(third).sibling==second &&
        journal->owner(second).sibling==child && journal->owner(third).parent_node==0x23,
        "source prepend order or literal node-byte truncation differs");
    const auto& held=journal->owner(child);
    check(held.header_cluster==0 && held.location.leaf==none && held.location.cluster==-1 &&
        held.location.bsp==3 && held.root_position.x==9 && held.rendering_sphere.radius==8 &&
        held.primitive.primitive_sphere.radius==4,"topology slice manufactured location/header/bounds/transform services");
    rejects([&]{journal->attach_link(ordinal+1,grandchild,parent,0,0);},"ancestor cycle admitted");
    rejects([&]{journal->attach_link(ordinal+1,parent,child,0,0);},"already attached child relinked");
    rejects([&]{journal->free(ordinal+1,second);},"attached storage freed without detach");
    rejects([&]{journal->detach_link(ordinal+1,0xe2000001);},"stale full child identity accepted");
    const auto saved=*journal;
    check(journal->detach_link(++ordinal,second),"middle child missing from actual sibling list");
    check(journal->owner(third).sibling==child && journal->owner(second).parent==none &&
        journal->owner(second).sibling==none && journal->owner(second).parent_node==0xff &&
        !(journal->owner(second).primitive.header_flags&0x80),"source middle splice differs");
    check(journal->owner(second).location.bsp==2 && journal->owner(second).location.cluster==-1,
        "detach invented a registration location");
    journal->free(++ordinal,second);
    check(journal->detach_link(++ordinal,child),"tail child missing");
    rejects([&]{journal->free(ordinal+1,child);},"root with retained child freed");
    check(journal->detach_link(++ordinal,grandchild),"grandchild unlink missing");journal->free(++ordinal,grandchild);
    journal->free(++ordinal,child);
    *journal=saved;ordinal=journal->events().back().ordinal;
    check(journal->owner(parent).child_head==third && journal->owner(second).sibling==child &&
        journal->owner(child).child_head==grandchild && journal->collidable().cluster(0).empty(),
        "checkpoint lost exact hierarchy or unregistered spatial state");
    journal->detach_link(++ordinal,second);journal->attach_link(++ordinal,third,second,-1,4);
    check(journal->owner(third).child_head==second && journal->owner(parent).child_head==third,
        "same-tick reparent changed an unrelated sibling chain");
    for(std::size_t i=1;i<journal->events().size();++i)
        check(journal->events()[i].ordinal>journal->events()[i-1].ordinal,"hierarchy event chronology lost");
}
}
int main(int argc,char** argv){try{
    if(argc==2 && std::string_view(argv[1])=="--probe"){
        std::string line;while(std::getline(std::cin,line)){
            const auto output=probe(parse(line));for(std::size_t i=0;i<output.size();++i)
                std::cout<<(i?" ":"")<<std::hex<<output[i];std::cout<<'\n';}return 0;}
    check(argc==2,"hierarchy test requires original fixture");std::ifstream input(argv[1]);
    check(bool(input),"missing original hierarchy fixture");std::string line;unsigned cases=0;std::size_t words=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;const auto separator=line.find('|');
        check(separator!=std::string::npos,"missing hierarchy fixture separator");
        const auto expected=parse(std::string_view(line).substr(separator+1));
        const auto actual=probe(parse(std::string_view(line).substr(0,separator)));
        if(actual!=expected){std::size_t i=0;while(i<actual.size() && i<expected.size() && actual[i]==expected[i])++i;
            throw FormatError("original hierarchy differs at case"+std::to_string(cases)+" word"+std::to_string(i));}
        ++cases;words+=expected.size();
    }
    check(input.eof() && cases>=256,"incomplete original hierarchy coverage");controls();
    std::cout<<cases<<" original topology streams / "<<words<<" exact words; explicit unregister, ordered links, free and restore controls passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
