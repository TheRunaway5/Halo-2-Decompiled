#include <limits>
#include <type_traits>
// Independent ownership/definedness/refusal models for the native-local DATA
// view. This test header is never part of a production runtime.
namespace {
struct PrivateModelMemory final:SourcePhysicsContactMemory {
    std::array<std::byte,4096> bytes{};
    mutable unsigned reads{};unsigned writes{};
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        check(p>=0x10000u&&std::uint64_t(p-0x10000u)+out.size()<=bytes.size(),"Private model read escaped");
        ++reads;std::copy_n(bytes.begin()+p-0x10000u,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>in)override{
        check(p>=0x10000u&&std::uint64_t(p-0x10000u)+in.size()<=bytes.size(),"Private model write escaped");
        ++writes;std::copy(in.begin(),in.end(),bytes.begin()+p-0x10000u);
    }
};
unsigned check_private_models(){
    unsigned controls{};
    const auto refuses=[&](const auto&operation){bool rejected=false;try{operation();}catch(const std::runtime_error&){rejected=true;}check(rejected,"Private negative control was admitted");++controls;};
    PrivateModelMemory outer;std::fill(outer.bytes.begin(),outer.bytes.end(),std::byte{0x5a});
    const std::array<NativeTu15ContactPrivateRange,2>owned{{{0x20000u,128u},{0x20100u,64u}}};
    const std::array<NativeTu15ContactPrivateRange,1>retained{{{0x10000u,4096u}}};
    NativeTu15ContactCandidatePrivateData memory(outer,owned,retained);
    SourcePhysicsContactRecordObserver observer=[](const auto&){};
    check(native_tu15_observes_retained_store(outer,observer,0x2f3d6bu,0x10000u,4),"Default retained observer suppressed");++controls;
    check(!native_tu15_observes_retained_store(memory,observer,0x3090b9u,0x20000u,16),"Owned local preimage exposed");++controls;
    std::array<std::byte,16>data{};
    refuses([&]{memory.read(0x20000u,data);});check(!outer.reads&&!outer.writes,"Unprovided read touched outer memory");
    const std::array<std::byte,2>two{std::byte{0xab},std::byte{0xcd}};memory.write(0x20000u,two);
    check(memory.data_is_defined(0x20000u,2)&&!memory.data_is_defined(0x20000u,4),"Partial definition was widened");++controls;
    refuses([&]{memory.read(0x20000u,std::span(data).first(4));});
    refuses([&]{memory.read(0x2007fu,std::span(data).first(2));});
    refuses([&]{memory.write(0x1ffffu,two);});
    refuses([&]{(void)memory.observes_retained_store(0x2f3d6bu,0x20000u,4);});
    refuses([&]{(void)memory.observes_retained_store(0x23328fu,0x20000u,4);});
    refuses([&]{(void)memory.observes_retained_store(0x2f3ce7u,0x20000u,4);});
    refuses([&]{memory.require_retained_extent(0x20000u,4);});
    check(memory.copy_defined_data(0x303ba0u,0x20010u,0x20000u,4),"Private transfer not handled");++controls;
    check(memory.data_is_defined(0x20010u,2)&&!memory.data_is_defined(0x20010u,4),"Partial transfer fabricated data");++controls;
    memory.read(0x20010u,std::span(data).first(2));check(data[0]==two[0]&&data[1]==two[1],"Defined bytes not copied");++controls;
    refuses([&]{memory.copy_defined_data(0x303ba0u,0x10000u,0x20010u,4,observer);});
    check(!outer.reads&&!outer.writes,"Undefined escape touched retained data");++controls;
    memory.write(0x20012u,two);check(memory.data_is_defined(0x20010u,4),"Completed source remains undefined");++controls;
    unsigned events{};bool callback_threw=false;
    try{memory.copy_defined_data(0x303ba0u,0x10000u,0x20010u,4,[&](const auto&e){
        ++events;check(e.instruction==0x303ba0u&&e.before==std::vector<std::byte>(4,std::byte{0x5a})&&e.written==std::vector<std::byte>({two[0],two[1],two[0],two[1]}),"Defined transfer event differs");throw std::runtime_error("declared observer failure");});}
    catch(const std::runtime_error&e){callback_threw=std::string(e.what())=="declared observer failure";}
    check(callback_threw&&events==1&&outer.reads==1&&outer.writes==1&&outer.bytes[0]==two[0],"Poststore transfer exception lost publication");++controls;
    check(memory.copy_defined_data(0x303ba0u,0x20020u,0x10000u,4),"Retained input copy not handled");++controls;
    check(memory.data_is_defined(0x20020u,4)&&outer.reads==2&&outer.writes==1,"Retained input was not one real read");++controls;
    const auto before_reads=outer.reads,before_writes=outer.writes;
    check(!memory.copy_defined_data(0x303ba0u,0x10010u,0x10000u,4)&&outer.reads==before_reads&&outer.writes==before_writes,"Default copy path changed");++controls;
    check(memory.copy_defined_data(0x303ba0u,0x20020u,0x20060u,4)&&!memory.data_is_defined(0x20020u,4),"Unknown copy retained stale definition");++controls;
    refuses([&]{memory.read(0x20020u,std::span(data).first(4));});
    refuses([&]{memory.copy_defined_data(0x2f3d6bu,0x20030u,0x20010u,4);});
    refuses([&]{memory.copy_defined_data(0x303ba0u,0x10000u,0x20010u,3);});
    const std::array<std::byte,4>four{std::byte{1},std::byte{2},std::byte{3},std::byte{4}};
    memory.write(0x20040u,four);memory.copy_defined_data(0x303ba0u,0x20041u,0x20040u,4);std::array<std::byte,5>five{};memory.read(0x20040u,five);
    check(five==std::array<std::byte,5>{four[0],four[0],four[1],four[2],four[3]},"Overlapping transfer did not snapshot source operand");++controls;
    NativeTu15MoppAabbPrivateCache nested(memory,0x20100u);
    check(!nested.observes_retained_store(0x3090b9u,0x20000u,16),"Nested domain did not delegate");++controls;
    refuses([&]{nested.require_retained_extent(0x20000u,4);});
    refuses([&]{nested.observes_retained_store(0x3090b9u,0x20100u,4);});
    check(nested.copy_defined_data(0x303ba0u,0x20050u,0x20010u,4)&&nested.data_is_defined(0x20050u,4),"Nested defined transfer did not delegate");++controls;
    refuses([&]{nested.copy_defined_data(0x303ba0u,0x20100u,0x20010u,4);});
    const std::array<NativeTu15ContactPrivateRange,1>alias{{{0x10000u,16u}}};refuses([&]{NativeTu15ContactCandidatePrivateData invalid(outer,alias,retained);});
    const std::array<NativeTu15ContactPrivateRange,2>overlap{{{0x20000u,16u},{0x20008u,16u}}};refuses([&]{NativeTu15ContactCandidatePrivateData invalid(outer,overlap,retained);});
    // The native-only scope is installed before depth/provider admission and
    // restored on every refusal, including a genuine nested enclosing scope.
    NativeTu15ContactCandidateBindingProviders providers;providers.owner_memory=&outer;
    NativeTu15PairDeletionNativeScope scope;NativeTu15ContactCandidateBindingActivity stats;
    NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1fa0};const auto reads=outer.reads,writes=outer.writes;
    refuses([&]{native_tu15_update_bound_contact_candidates(outer,{},{},{},{},{},{},fp,1,providers,scope,{},&stats);});
    check(!scope.active()&&stats.unsupported_depths==1&&stats.refusals==1&&outer.reads==reads&&outer.writes==writes,"Depth refusal touched source or leaked scope");++controls;
    {NativeTu15PairDeletionNativeScopeGuard enclosing(scope);providers.owner_memory=&memory;
     refuses([&]{native_tu15_update_bound_contact_candidates(outer,{},{},{},{},{},{},fp,0,providers,scope);});
     check(scope.active()&&outer.reads==reads&&outer.writes==writes,"Nested owner refusal changed enclosing scope/data");++controls;}
    check(!scope.active(),"Native scope remained active");++controls;
    static_assert(!std::is_copy_constructible_v<NativeTu15ContactCandidatePrivateData>);
    static_assert(!std::is_copy_assignable_v<NativeTu15ContactCandidatePrivateData>);
    static_assert(!std::is_move_constructible_v<NativeTu15ContactCandidatePrivateData>);
    static_assert(!std::is_move_assignable_v<NativeTu15ContactCandidatePrivateData>);
    controls+=4;
    // Lifetime invalidation removes only definition metadata. It never reads
    // the outer bank or fabricates a numerical value for the invalidated word.
    const auto old_reads=outer.reads,old_writes=outer.writes;
    const std::array<std::byte,8>eight{std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte{6},std::byte{7},std::byte{8}};
    memory.write(0x20000u,eight);
    check(memory.withhold_defined_data(0x20002u,2)&&memory.data_is_defined(0x20000u,2)&&
        !memory.data_is_defined(0x20000u,4)&&memory.data_is_defined(0x20004u,4),"Withhold widened or retained definition");++controls;
    refuses([&]{memory.read(0x20000u,std::span(data).first(4));});
    check(memory.copy_defined_data(0x303ba0u,0x20030u,0x20000u,8)&&
        memory.data_is_defined(0x20030u,2)&&!memory.data_is_defined(0x20030u,4)&&
        memory.data_is_defined(0x20034u,4),"Withheld mask was not propagated");++controls;
    refuses([&]{memory.copy_defined_data(0x303ba0u,0x10020u,0x20030u,8,observer);});
    check(memory.withhold_defined_data(0xffffffffu,0)&&outer.reads==old_reads&&outer.writes==old_writes,"Empty withhold touched DATA");++controls;
    // Validate the shared extent admission before any potentially huge mask
    // operation; the first check also safely discriminates a wrapping mutant.
    refuses([&]{memory.observes_retained_store(0,0x20020u,std::numeric_limits<std::size_t>::max()-15u);});
    refuses([&]{memory.withhold_defined_data(0x20020u,std::numeric_limits<std::size_t>::max()-15u);});
    refuses([&]{memory.withhold_defined_data(0x2007fu,2);});
    refuses([&]{memory.withhold_defined_data(0xffffffffu,2);});
    refuses([&]{memory.withhold_defined_data(0x10000u,4);});
    check(memory.data_is_defined(0x20004u,4)&&outer.reads==old_reads&&outer.writes==old_writes,"Refused invalidation changed state or accessed retained DATA");++controls;
    const std::array<NativeTu15ContactPrivateRange,1>inner_owned{{{0x30000u,16u}}};
    NativeTu15ContactCandidatePrivateData inner(static_cast<SourcePhysicsContactMemory&>(memory),inner_owned,retained);
    inner.write(0x30000u,four);memory.write(0x20050u,four);
    check(inner.withhold_defined_data(0x20050u,4)&&!inner.data_is_defined(0x20050u,4)&&
        !memory.data_is_defined(0x20050u,4),"Nested withhold cloned the outer mask");++controls;
    check(inner.withhold_defined_data(0x30001u,2)&&inner.data_is_defined(0x30000u,1)&&
        !inner.data_is_defined(0x30000u,4)&&inner.data_is_defined(0x30003u,1),"Nested local invalidation escaped its mask");++controls;
    refuses([&]{inner.withhold_defined_data(0x10000u,4);});
    check(outer.reads==old_reads&&outer.writes==old_writes,"Nested invalidation touched retained bytes");++controls;
    return controls;
}
} // namespace
