#include "halo2/mcc_campaign_vanilla.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
#include <stdexcept>

namespace {
using J=nlohmann::json;
using namespace halo2;
void require(bool value,const std::string& why) {if(!value)throw std::runtime_error(why);}
std::vector<std::byte> read_file(const std::filesystem::path& path) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);require(bool(stream),"Missing fixture "+path.string());
    const auto size=stream.tellg();stream.seekg(0);std::vector<std::byte> out(std::size_t(size),std::byte{});
    stream.read(reinterpret_cast<char*>(out.data()),out.size());require(bool(stream),"Short fixture read");return out;
}
std::string sha(std::span<const std::byte> bytes) {
    unsigned size=0;std::array<unsigned char,32> out{};
    require(EVP_Digest(bytes.data(),bytes.size(),out.data(),&size,EVP_sha256(),nullptr)==1 && size==32,"Digest failed");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(auto byte:out){result+=hex[byte>>4];result+=hex[byte&15];}return result;
}
struct Fixture {
    J profile;
    std::uint32_t base{};
    std::vector<std::byte> bytes;
    std::vector<bool> owned;
    std::map<std::uint32_t,std::string> strings;
    std::map<std::uint32_t,MccCampaignTagIdentity> tags;
    MccCampaignVanillaSource source;
    std::span<const std::byte> read(std::uint32_t address,std::size_t size) const {
        require(address>=base && std::uint64_t(address-base)+size<=bytes.size(),"Unowned source extent");
        for(std::size_t i=address-base;i<std::size_t(address-base)+size;++i)require(owned[i],"Source namespace crosses a hole");
        return std::span(bytes).subspan(address-base,size);
    }
    unsigned word(std::uint32_t address,unsigned width=4) const {
        const auto input=read(address,width);unsigned value=0;
        for(unsigned i=0;i<width;++i)value|=std::to_integer<unsigned>(input[i])<<(i*8);
        return value;
    }
    Fixture(const J& p,const std::filesystem::path& folder):profile(p) {
        std::uint64_t first=0xffffffffu,last=0;
        for(const auto& bank:p["banks"]){const auto at=bank["token"].get<std::uint32_t>();first=std::min(first,std::uint64_t(at));last=std::max(last,std::uint64_t(at)+bank["size"].get<unsigned>());}
        require(last-first<64*1024*1024,"Excessive test arena");base=first;bytes.resize(last-first);owned.resize(bytes.size());
        for(const auto& bank:p["banks"]) {
            auto data=read_file(folder/p["map"].get<std::string>()/"mcc"/(bank["name"].get<std::string>()+".bin"));
            require(data.size()==bank["size"].get<unsigned>() && sha(data)==bank["sha256"].get<std::string>(),"Fixture fingerprint differs");
            const auto at=bank["token"].get<unsigned>()-base;
            for(std::size_t i=0;i<data.size();++i){if(owned[at+i])require(bytes[at+i]==data[i],"Conflicting aliased source banks");bytes[at+i]=data[i];owned[at+i]=true;}
        }
        const auto scenario=p["scenario_address"].get<unsigned>();
        const auto sid=[&](unsigned id,const std::string& name){const auto found=strings.find(id);require(found==strings.end() || found->second==name,"Conflicting StringID namespace");strings[id]=name;};
        for(const auto& literal:p["literal_bindings"]) {
            const auto& b=literal["binding"];const auto value=literal["value"].get<unsigned>();
            if(b.contains("string_id"))sid(value,b["string_id"]);
            if(b.contains("trigger_volume"))sid(word(word(scenario+0x10c)+(value&65535)*0x44),b["trigger_volume"]);
            if(b.contains("ai_starting_location")) {
                const auto squad=word(scenario+0x164)+((value>>16)&0x3fff)*0x74;
                const auto id=word(word(squad+0x4c)+(value&65535)*0x64);
                const auto name=b["ai_starting_location"].get<std::string>();sid(id,name.substr(name.find('/')+1));
            }
            if(b.contains("structure_bsp"))tags[word(word(scenario+0x214)+(value&65535)*0x44+0x14)]={b["group"],b["path"]};
            else if(b.contains("group"))tags[value]={b["group"],b["path"]};
        }
        for(const auto& owner:p["navpoint_namespaces"])for(const auto& key:{"hud","bitmap","shader"}) {
            const auto& tag=owner[key];tags[tag["datum"].get<unsigned>()]={tag["group"],tag["name"]};
        }
        source.is_mcc=true;source.scenario_path=profile["scenario_path"].get_ref<const std::string&>();source.scenario_address=scenario;
        source.read=[this](auto at,auto size){return read(at,size);};
        source.resolve_tag=[this](auto id)->std::optional<MccCampaignTagIdentity>{const auto found=tags.find(id);if(found==tags.end())return {};return found->second;};
        source.resolve_string_id=[this](auto id){const auto found=strings.find(id);require(found!=strings.end(),"Unknown fixture StringID");return found->second;};
    }
    void commit(const MccCampaignVanillaPlan& plan) {
        for(const auto& write:plan.writes)require(std::ranges::equal(read(write.address,write.before.size()),write.before),"Stale planned before image");
        std::vector<std::pair<std::uint32_t,std::size_t>> ranges;
        for(const auto& write:plan.writes){require(write.before.size()==write.after.size(),"Resized metadata write");ranges.emplace_back(write.address,write.before.size());}
        std::sort(ranges.begin(),ranges.end());for(std::size_t i=1;i<ranges.size();++i)require(std::uint64_t(ranges[i-1].first)+ranges[i-1].second<=ranges[i].first,"Overlapping planned writes");
        for(const auto& write:plan.writes)std::copy(write.after.begin(),write.after.end(),bytes.begin()+write.address-base);
    }
};
void rejected(Fixture& fixture,const std::string& description) {
    const auto before=fixture.bytes;const auto plan=plan_mcc_campaign_vanilla(fixture.source);
    require(!plan.matched && !plan.already_equivalent && plan.writes.empty() && !plan.reason.empty(),description+" admitted a custom or incomplete context");
    require(fixture.bytes==before,description+" wrote live source bytes");
}
}
int main(int argc,char** argv)try {
    if(argc==1 || (argc==3 && (std::string_view(argv[1]).empty() || std::string_view(argv[2]).empty()))) {
        std::cout<<"Owned MCC campaign audit fixtures were not selected\n";return 77;
    }
    require(argc==3,"usage: mcc_campaign_vanilla_tests RAW_MCC_AUDIT_FIXTURES PROFILE_MANIFEST");
    std::ifstream input(argv[2]);const auto manifest=J::parse(input);require(manifest["profiles"].size()==14,"Expected all fourteen retail profiles");
    static_assert(0x37f==895); // Verified movie opcode word; this is not a call count.
    unsigned profiles=0,retired=0,removed=0,wakes=0,assignments=0,movies=0,remaining_movies=0,calls=0,navigation_calls=0,aliases=0,words=0,slots=0,wrappers=0,renames=0,navpoints=0,durations=0,rewritten_rows=0,allocations=0,negative=0;
    for(const auto& p:manifest["profiles"]) {
        Fixture f(p,argv[1]);const auto before=f.bytes;const auto plan=plan_mcc_campaign_vanilla(f.source);
        require(plan.matched && !plan.already_equivalent && !plan.writes.empty(),"Retail profile did not plan verified conversion: "+p["map"].get<std::string>()+" / "+plan.reason);
        require(f.bytes==before,"Planning changed the donor arena");
        require(plan.removed_scripts==p["removed_scripts"].size() && plan.retired_nodes==p["retired_nodes"].size() && plan.wake_patches==p["wake_patches"].size(),"Retail authoring counts differ");
        require(plan.script_ordinal_words==p["proof"]["script_ordinal_words"] && plan.scenario_ordinal_words==p["proof"]["scenario_ordinal_words"],"Ordinal census differs");
        f.commit(plan);
        for(const auto& bank:p["banks"])require(sha(f.read(bank["token"],bank["size"]))==bank["after_sha256"].get<std::string>(),"Converted bank differs from retained independent projection");
        const auto again=plan_mcc_campaign_vanilla(f.source);require(again.matched && again.already_equivalent && again.writes.empty(),"Exact converted input is not idempotent");
        // Surviving source string storage is repaired by original2AA6D0 at HS
        // startup. The authoring planner must preserve both relative+C and +10.
        const auto nodes=std::find_if(p["banks"].begin(),p["banks"].end(),[](const auto& b){return b["name"]=="block_568";});
        const auto at=(*nodes)["token"].template get<unsigned>(),size=(*nodes)["size"].template get<unsigned>();
        std::set<unsigned> static_opcodes;
        for(const auto& reference:p["script_references"])if(reference["kind"]==0) {
            static_opcodes.insert(reference["datum"].get<unsigned>()&65535u);
            static_opcodes.insert(reference["function_datum"].get<unsigned>()&65535u);
        }
        std::set<unsigned> original_opcode_restorations;
        for(const auto& word:p["node_words"])if(word["field_offset"]==2 && word["width"]==2)original_opcode_restorations.insert(word["datum"].get<unsigned>()&65535u);
        for(const auto& rewrite:p["syntax_row_rewrites"])original_opcode_restorations.insert(rewrite["index"].get<unsigned>());
        for(unsigned i=0;i<size;i+=20)if(f.word(at+i,2) && !static_opcodes.contains(i/20) && !original_opcode_restorations.contains(i/20))
            require(std::equal(before.begin()+at-f.base+i+2,before.begin()+at-f.base+i+4,f.bytes.begin()+at-f.base+i+2),"Retained MCC builtin/literal opcode was rewritten");
        for(unsigned i=0;i<size;i+=20)if(f.word(at+i,2) && f.word(at+i+2,2)==9 && f.word(at+i+6,2)&1)
            require(std::equal(before.begin()+at-f.base+i+12,before.begin()+at-f.base+i+20,f.bytes.begin()+at-f.base+i+12),"Surviving string storage was canonicalized");
        std::set<unsigned> retired_handles;for(const auto& datum:p["retired_nodes"])retired_handles.insert(datum.get<unsigned>());
        std::map<unsigned,unsigned> removed_successors;
        for(const auto& key:{"wake_patches","assignment_patches","movie_patches","call_patches"})for(const auto& patch:p[key])
            require(removed_successors.emplace(patch["removed"].get<unsigned>(),patch["next"].get<unsigned>()).second,"Duplicate test splice owner");
        const auto final_successor=[&](unsigned next){std::set<unsigned> seen;while(removed_successors.contains(next)){require(seen.insert(next).second,"Cyclic test splice");next=removed_successors.at(next);}return next;};
        profiles++;retired+=plan.retired_nodes;removed+=plan.removed_scripts;wakes+=plan.wake_patches;
        for(const auto& patch:p["assignment_patches"]) {
            assignments++;
            const auto predecessor=at+(patch["predecessor"].get<unsigned>()&65535u)*20;
            require(f.word(predecessor+8)==patch["next"].get<unsigned>(),"Added breadcrumb assignment remains linked");
            for(const auto& datum:patch["rows"]) {
                const auto row=at+(datum.get<unsigned>()&65535u)*20;
                require(f.word(row,2)==0,"Added assignment retained its salt");
                for(unsigned byte=2;byte<20;++byte)require(f.word(row+byte,1)==0xba,"Added assignment free-row bytes differ");
            }
            for(const auto& key:{"global_datum","literal_datum","predecessor"}) {
                Fixture bad(p,argv[1]);const auto token=at+(patch[key].get<unsigned>()&65535u)*20;
                bad.bytes[token-bad.base+16]^=std::byte{1};
                rejected(bad,std::string("Modified added-assignment ")+key);negative++;
            }
        }
        for(const auto& patch:p["movie_patches"]) {
            movies++;
            if(!retired_handles.contains(patch["predecessor"].get<unsigned>()))require(f.word(at+(patch["predecessor"].get<unsigned>()&65535u)*20+8)==final_successor(patch["next"].get<unsigned>()),"Added movie remains linked");
            for(const auto& datum:patch["rows"]) {
                const auto row=at+(datum.get<unsigned>()&65535u)*20;
                require(f.word(row,2)==0,"Added movie retained its salt");
                for(unsigned byte=2;byte<20;++byte)require(f.word(row+byte,1)==0xba,"Added movie free-row bytes differ");
            }
        }
        for(unsigned i=0;i<size;i+=20)if(f.word(at+i,2) && f.word(at+i+2,2)==0x37f && !(f.word(at+i+6,2)&1))remaining_movies++;
        if(!p["movie_patches"].empty()) {
            Fixture bad(p,argv[1]);const auto& patch=p["movie_patches"].front();const auto literal=at+(patch["string_datum"].get<unsigned>()&65535u)*20;
            bad.bytes[literal-bad.base+2]^=std::byte{1};rejected(bad,"Modified movie string literal type");negative++;
        }
        for(const auto& label:p.value("movie_proof",J::object()).value("extra_guard_banks",J::array())) {
            Fixture bad(p,argv[1]);const auto bank=std::find_if(p["banks"].begin(),p["banks"].end(),[&](const auto& b){return b["name"]==label;});
            require(bank!=p["banks"].end(),"Missing complete movie holder namespace guard");const auto token=(*bank)["token"].get<unsigned>(),extent=(*bank)["size"].get<unsigned>();
            const auto original=bad.source.read;bad.source.read=[original,token,extent](auto address,auto bytes){return address==token && bytes==extent?std::span<const std::byte>{}:original(address,bytes);};
            rejected(bad,"Missing movie-holder typed namespace");negative++;
        }
        for(const auto& patch:p["call_patches"]) {
            calls++;const auto function=patch["function"].get<std::string>();navigation_calls+=function.starts_with("breadcrumbs_") || function=="ai_overcomes_oversteer";
            if(!retired_handles.contains(patch["predecessor"].get<unsigned>()))require(f.word(at+(patch["predecessor"].get<unsigned>()&65535u)*20+8)==final_successor(patch["next"].get<unsigned>()),"Added authoring call chain remains linked");
            for(const auto& datum:patch["rows"]) {
                const auto token=at+(datum.get<unsigned>()&65535u)*20;
                require(f.word(token,2)==0,"Added call retained its salt");
                for(unsigned byte=2;byte<20;++byte)require(f.word(token+byte,1)==0xba,"Added call free-row bytes differ");
            }
            for(const auto& key:{"predecessor","removed","function_datum"}) {
                Fixture bad(p,argv[1]);const auto token=at+(patch[key].get<unsigned>()&65535u)*20;
                bad.bytes[token-bad.base+8]^=std::byte{1};rejected(bad,std::string("Modified added-call ")+key);negative++;
            }
            Fixture bad(p,argv[1]);const auto token=at+(patch["function_datum"].get<unsigned>()&65535u)*20;
            bad.bytes[token-bad.base]^=std::byte{1};rejected(bad,"Stale added-call full-salt owner");negative++;
        }
        for(const auto& literal:p["literal_bindings"])if(literal["binding"].contains("authored_label")) {
            aliases++;
            const auto value=literal["value"].get<unsigned>();
            require(p["map"]=="05b_deltatowers" && (value==153 || value==154),"Unexpected authored AI alias");
            const auto owner=f.word(p["scenario_address"].get<unsigned>()+0x164)+(value&65535u)*0x74;
            require(std::string(reinterpret_cast<const char*>(f.read(owner,32).data()))==literal["binding"]["ai_squad"].get<std::string>(),"Physical AI alias identity differs");
            Fixture bad(p,argv[1]);bad.bytes[owner-bad.base+18]^=std::byte{0x20};rejected(bad,"Changed explicit AI alias case identity");negative++;
            Fixture wrong_label(p,argv[1]);const auto token=at+(literal["datum"].get<unsigned>()&65535u)*20;
            wrong_label.bytes[token-wrong_label.base+12]^=std::byte{1};rejected(wrong_label,"Changed authored AI alias label");negative++;
        }
        for(const auto& label:p["v4_proof"]["extra_guard_banks"]) {
            Fixture bad(p,argv[1]);const auto bank=std::find_if(p["banks"].begin(),p["banks"].end(),[&](const auto& b){return b["name"]==label;});
            require(bank!=p["banks"].end(),"Missing new holder namespace guard");const auto token=(*bank)["token"].get<unsigned>(),extent=(*bank)["size"].get<unsigned>();
            const auto original=bad.source.read;bad.source.read=[original,token,extent](auto address,auto bytes){return address==token && bytes==extent?std::span<const std::byte>{}:original(address,bytes);};
            rejected(bad,"Missing added-call retained typed namespace");negative++;
        }
        for(const auto& word:p["node_words"]) {
            words++;
            const auto token=at+(word["datum"].get<unsigned>()&65535u)*20+word["field_offset"].get<unsigned>();
            require(f.word(token,word["width"])==word["after"].get<unsigned>(),"Original typed fade word was not restored");
            Fixture bad(p,argv[1]);bad.bytes[token-bad.base]^=std::byte{1};rejected(bad,"Custom original-fade operand/order/type");negative++;
        }
        for(const auto& slot:p["string_slots"]) {
            slots++;
            const auto strings=f.word(p["scenario_address"].get<unsigned>()+0x1b4),offset=slot["offset"].get<unsigned>(),extent=slot["size"].get<unsigned>();
            const auto expected=slot["after"].get<std::string>();
            for(unsigned i=0;i<extent;++i)require(f.word(strings+offset+i,1)==(i<expected.size()?unsigned(static_cast<unsigned char>(expected[i])):0),"Restored original function-name slot differs");
            for(const auto& debug:slot["expression_debug_offsets"]) {
                const auto token=at+(debug["datum"].get<unsigned>()&65535u)*20+12;
                require(std::equal(before.begin()+token-f.base,before.begin()+token-f.base+4,f.bytes.begin()+token-f.base),"Overlapping expression source offset changed");
                require(f.word(token)==debug["offset"].get<unsigned>(),"Overlapping original source/debug offset differs");
                Fixture bad(p,argv[1]);bad.bytes[token-bad.base]^=std::byte{1};rejected(bad,"Changed overlapping expression source offset");negative++;
            }
            for(unsigned i:{0u,extent-1}) {
                Fixture bad(p,argv[1]);bad.bytes[strings+offset-bad.base+i]^=std::byte{1};rejected(bad,"Custom owned function-name slot bytes/capacity");negative++;
            }
            Fixture bad(p,argv[1]);const auto declarations=f.word(p["scenario_address"].get<unsigned>()+0x1bc);
            bad.bytes[declarations+slot["static_declaration"].get<unsigned>()*40+32-bad.base]^=std::byte{1};rejected(bad,"Changed original static declaration signature");negative++;
        }
        for(const auto& wrapper:p["wrapper_promotions"]) {
            wrappers++;
            require(f.word(at+(wrapper["predecessor"].get<unsigned>()&65535u)*20+8)==wrapper["child"].get<unsigned>(),"Original nav call was not promoted");
            require(f.word(at+(wrapper["child"].get<unsigned>()&65535u)*20+8)==wrapper["next"].get<unsigned>(),"Promoted original nav call retained wrapper siblings");
            for(const auto& key:{"root","function"})require(!f.word(at+(wrapper[key].get<unsigned>()&65535u)*20,2),"Added wrapper retained its salt");
            for(const auto& key:{"root","function","child","predecessor"}) {
                Fixture bad(p,argv[1]);const auto token=at+(wrapper[key].get<unsigned>()&65535u)*20;
                bad.bytes[token-bad.base]^=std::byte{1};rejected(bad,"Custom promoted nav wrapper/child full-salt owner");negative++;
            }
        }
        for(const auto& rename:p["global_renames"]) {
            renames++;const auto declarations=f.word(p["scenario_address"].get<unsigned>()+0x1c4),index=rename["index"].get<unsigned>();
            const auto text=std::string(reinterpret_cast<const char*>(f.read(declarations+index*40,32).data()));
            require(text==rename["after"].get<std::string>() && f.word(p["scenario_address"].get<unsigned>()+0x1c0)==p["banks"][std::distance(p["banks"].begin(),std::find_if(p["banks"].begin(),p["banks"].end(),[](const auto& b){return b["name"]=="block_448";}))]["size"].get<unsigned>()/40,"Original clock global rename changed ordinal/count");
            require(f.word(at+(rename["root"].get<unsigned>()&65535u)*20+16)==0xffff001e && !f.word(at+(rename["function"].get<unsigned>()&65535u)*20,2),"Original SHORT30 initializer did not release its sole function child");
            unsigned references=0;for(unsigned i=0;i<size;i+=20)if(f.word(at+i,2) && f.word(at+i+6,2)!=0xbaba && f.word(at+i+6,2)&4 && f.word(at+i+16)==index)references++;
            require(references==rename["reference_count_after"].get<unsigned>(),"Original clock global reference census differs");
            Fixture bad(p,argv[1]);bad.bytes[declarations+index*40-bad.base]^=std::byte{1};rejected(bad,"Custom original clock global name");negative++;
        }
        for(const auto& owner:p["navpoint_namespaces"]) {
            navpoints++;
            for(const auto& key:{"hud","bitmap","shader"}) {
                Fixture bad(p,argv[1]);const auto datum=owner[key]["datum"].get<unsigned>();bad.tags[datum].path+="_custom";rejected(bad,"Custom navpoint full-salt asset identity");negative++;
            }
            for(const auto& label:{"navpoint_hud_header","navpoint_bitmap_header","navpoint_palette","navpoint_bitmap_sequences"}) {
                const auto bank=std::find_if(p["banks"].begin(),p["banks"].end(),[&](const auto& b){return b["name"]==label;});require(bank!=p["banks"].end(),"Missing complete navpoint palette owner guard");
                Fixture bad(p,argv[1]);const auto token=(*bank)["token"].get<unsigned>(),extent=(*bank)["size"].get<unsigned>();const auto original=bad.source.read;
                bad.source.read=[original,token,extent](auto address,auto bytes){return address==token && bytes==extent?std::span<const std::byte>{}:original(address,bytes);};rejected(bad,"Missing complete navpoint palette/sequence owner");negative++;
                Fixture custom(p,argv[1]);custom.bytes[token-custom.base+extent-1]^=std::byte{1};rejected(custom,"Custom navpoint palette/sequence bytes");negative++;
            }
        }
        for(const auto& duration:p["sound_duration_promotions"]) {
            durations++;const auto predecessor=at+(duration["predecessor"].get<unsigned>()&65535u)*20,child=at+(duration["child"].get<unsigned>()&65535u)*20;
            require(f.word(predecessor+8)==duration["child"].get<unsigned>() && f.word(child+4,2)==7 && f.word(child+8)==duration["next"].get<unsigned>(),"Original sound-duration call/context was not promoted");
            for(const auto& datum:duration["retired_rows"])require(f.word(at+(datum.get<unsigned>()&65535u)*20,2)==0,"Added duration wrapper/getter retained its salt");
            for(const auto& key:{"root","scalar","child","child_function","predecessor"}) {
                Fixture bad(p,argv[1]);const auto token=at+(duration[key].get<unsigned>()&65535u)*20;
                bad.bytes[token-bad.base]^=std::byte{1};rejected(bad,"Custom duration expression full-salt/owner");negative++;
            }
        }
        for(const auto& rewrite:p["syntax_row_rewrites"]) {
            rewritten_rows++;const auto token=at+rewrite["index"].get<unsigned>()*20;const auto& expected=rewrite["after"];
            for(unsigned i=0;i<7;++i)require(f.word(token+(i<4?i*2:8+(i-4)*4),i<4?2:4)==expected[i].get<unsigned>(),"Original reconstructed typed syntax row differs");
            for(unsigned byte:{0u,6u,16u}){Fixture bad(p,argv[1]);bad.bytes[token-bad.base+byte]^=std::byte{1};rejected(bad,"Custom reconstructed syntax row/free-slot/type/value");negative++;}
        }
        for(const auto& allocated:p["syntax_allocations"]) {
            allocations++;const auto token=at+allocated["index"].get<unsigned>()*20;
            require(!((unsigned(std::to_integer<unsigned>(before[token-f.base+1]))<<8)|std::to_integer<unsigned>(before[token-f.base])),"Original reconstructed slot was occupied before allocation");
            require(f.word(token,2)==allocated["salt"].get<unsigned>(),"Original reconstructed allocation salt differs");
            Fixture bad(p,argv[1]);bad.bytes[token-bad.base]=std::byte{1};rejected(bad,"Occupied original allocation slot");negative++;
        }
        if(p["map"]=="01a_tutorial") {
            const auto declarations=f.word(p["scenario_address"].get<unsigned>()+0x1bc);
            require(std::equal(before.begin()+declarations-f.base,before.begin()+declarations-f.base+77*40,f.bytes.begin()+declarations-f.base),"Tutorial declaration order/prototypes changed");
            std::set<unsigned> edited;
            for(const auto& word:p["node_words"]){const auto start=at+(word["datum"].get<unsigned>()&65535u)*20+word["field_offset"].get<unsigned>();for(unsigned i=0;i<word["width"].get<unsigned>();++i)edited.insert(start+i);}
            for(const auto& slot:p["string_slots"]){const auto start=f.word(p["scenario_address"].get<unsigned>()+0x1b4)+slot["offset"].get<unsigned>();for(unsigned i=0;i<slot["size"].get<unsigned>();++i)edited.insert(start+i);}
            for(const auto& rename:p["global_renames"]){const auto start=f.word(p["scenario_address"].get<unsigned>()+0x1c4)+rename["index"].get<unsigned>()*40;for(unsigned i=0;i<32;++i)edited.insert(start+i);}
            for(const auto& datum:p["retired_nodes"]){const auto start=at+(datum.get<unsigned>()&65535u)*20;for(unsigned i=0;i<20;++i)edited.insert(start+i);}
            for(unsigned i=0;i<f.bytes.size();++i)if(before[i]!=f.bytes[i])require(edited.contains(f.base+i),"Tutorial changed bytes outside the proved source words/string slot");
        }
        {Fixture bad(p,argv[1]);bad.source.is_mcc=false;rejected(bad,"Original reference context");negative++;}
        {Fixture bad(p,argv[1]);bad.source.scenario_path="custom\\same_script_names";rejected(bad,"Unknown authored scenario");negative++;}
        {Fixture bad(p,argv[1]);bad.source.scenario_address+=4;rejected(bad,"Shifted source owner");negative++;}
        {Fixture bad(p,argv[1]);bad.bytes[at-bad.base+16]^=std::byte{1};rejected(bad,"Modified donor syntax");negative++;}
        {Fixture bad(p,argv[1]);const auto original=bad.source.read;bad.source.read=[original,at,size](auto address,auto bytes){auto result=original(address,bytes);return address==at && bytes==size?result.first(result.size()-1):result;};rejected(bad,"Truncated native source view");negative++;}
        {Fixture bad(p,argv[1]);const auto& first=plan.writes.front();
            if(plan.writes.size()>1)std::copy(first.after.begin(),first.after.end(),bad.bytes.begin()+first.address-bad.base);
            else {
                const auto changed=std::mismatch(first.before.begin(),first.before.end(),first.after.begin()).first-first.before.begin();
                require(changed<first.before.size(),"Single-write profile lacks a real difference");
                bad.bytes[first.address-bad.base+changed]=first.after[changed];
            }
            rejected(bad,"Partially committed profile");negative++;}
        if(!f.strings.empty()){Fixture bad(p,argv[1]);bad.source.resolve_string_id=[](auto){return std::string("custom_string_identity");};rejected(bad,"Wrong StringID namespace");negative++;}
        if(!f.tags.empty()){Fixture bad(p,argv[1]);bad.source.resolve_tag=[](auto)->std::optional<MccCampaignTagIdentity>{return MccCampaignTagIdentity{"weap","custom\\wrong_full_handle"};};rejected(bad,"Wrong full-salt tag group/path");negative++;}
        if(p["map"]=="04a_gasgiant") {
            const auto group=std::find_if(p["banks"].begin(),p["banks"].end(),[](const auto& b){return b["name"]=="block_344";});
            require(group!=p["banks"].end(),"Missing complete Gasgiant AI squad-group guard");
            Fixture bad(p,argv[1]);const auto token=(*group)["token"].get<unsigned>(),extent=(*group)["size"].get<unsigned>();
            const auto original=bad.source.read;bad.source.read=[original,token,extent](auto address,auto bytes){return address==token && bytes==extent?std::span<const std::byte>{}:original(address,bytes);};
            rejected(bad,"Missing Gasgiant AI squad-group namespace");negative++;
        }
        // Already-equivalent admission must still check external namespaces.
        if(!f.tags.empty()){f.source.resolve_tag=[](auto)->std::optional<MccCampaignTagIdentity>{return {};};rejected(f,"Changed converted tag namespace");negative++;}
    }
    require(profiles==14 && removed==22 && wakes==14 && assignments==3 && movies==38 && remaining_movies==0 && calls==39 && navigation_calls==27 && aliases==2 && words==139 && slots==2 && rewritten_rows==14 && allocations==8 && wrappers==2 && renames==2 && navpoints==1 && durations==38 && retired==6246,"Fourteen-profile bounded authoring aggregate differs");
    std::cout<<J({{"profiles",profiles},{"original_typed_word_restorations",words},{"original_function_name_slots",slots},{"original_reconstructed_syntax_rows",rewritten_rows},{"original_allocated_syntax_rows",allocations},{"navigation_oversteer_calls_removed",navigation_calls},{"total_added_authoring_calls_removed",calls},{"original_nav_wrapper_promotions",wrappers},{"original_clock_globals_restored",renames},{"owned_navpoint_palettes",navpoints},{"original_sound_duration_promotions",durations},{"explicit_AI_label_aliases",aliases},{"removed_scripts",removed},{"wake_patches",wakes},{"assignment_patches",assignments},{"movie_calls_removed",movies},{"movie_opcode_hex","0x37f"},{"movie_opcode_decimal",895},{"excluded_movie_calls_preserved",remaining_movies},{"retired_nodes",retired},{"custom_guards",negative},{"constructor_independent",true},{"source_writes_during_planning",0}}).dump(2)<<'\n';
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
