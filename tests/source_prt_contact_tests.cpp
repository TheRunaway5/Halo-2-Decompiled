#include "halo2/source_prt_contact.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
struct Bank { std::int32_t active{};std::vector<SourcePrtContactCacheRecord> rows;std::vector<std::byte> arena; };
Words words(const std::string& line){Words out;std::istringstream in(line);std::uint32_t w;while(in>>std::hex>>w)out.push_back(w);return out;}
Words solve(const std::vector<Bank>& banks,const Words& input) {
    if(input.size()!=21 || input[0]>=banks.size())throw FormatError("PRT contact fixture dimensions");
    const auto& bank=banks[input[0]];std::array<SourcePrtContactPoint,3> points;
    for(unsigned i=0;i<3;++i){points[i].cluster=std::bit_cast<float>(input[4+i*5]);for(unsigned j=0;j<4;++j)points[i].weights[j]=std::bit_cast<float>(input[5+i*5+j]);}
    const auto sample=source_prt_contact_sample(bank.rows,bank.active,bank.arena,input[1],input[2],std::int32_t(input[3]),points,std::bit_cast<float>(input[19]),std::bit_cast<float>(input[20]));
    if(!sample)return {0,0xcdcdcdcdu,0xcdcdcdcdu,0xcdcdcdcdu,0xcdcdcdcdu,0xcdcdcdcdu,0xcdcdcdcdu};
    Words out{1};for(auto v:{sample->incident.x,sample->incident.y,sample->incident.z,sample->color.x,sample->color.y,sample->color.z})out.push_back(std::bit_cast<std::uint32_t>(v));return out;
}
template<class F>void reject(F&& f){bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}if(!rejected)throw std::runtime_error("missing source PRT contact bank accepted");}
}
int main(){try{
    const auto base=std::string(HALO2_SOURCE_DIR)+"/tests/fixtures/retail_source_prt_contact";
    std::ifstream bin(base+".bin",std::ios::binary);if(!bin)throw std::runtime_error("PRT contact cache bank missing");
    std::vector<char> chars((std::istreambuf_iterator<char>(bin)),{});std::vector<std::byte> storage(chars.size());for(std::size_t i=0;i<chars.size();++i)storage[i]=std::byte(chars[i]);const Bytes b(storage);
    if(b.size()<12 || std::string(chars.data(),8)!="H2PRTCO1")throw FormatError("PRT contact bank header");
    std::size_t at=8;const auto take=[&](){const auto v=b.u32(at);at+=4;return v;};const auto n=take();if(n>16)throw FormatError("PRT contact bank count");std::vector<Bank> banks;
    for(unsigned i=0;i<n;++i){Bank bank;bank.active=std::int32_t(take());const auto rows=take(),bytes=take();if(rows>32||bytes>0x27000)throw FormatError("PRT contact bank storage");bank.rows.resize(rows);for(auto& row:bank.rows)for(auto& w:row.words)w=take();const auto slice=b.slice(at,bytes);bank.arena.assign(slice.begin(),slice.end());at+=bytes;banks.push_back(std::move(bank));}
    if(at!=b.size())throw FormatError("PRT contact trailing bank bytes");
    std::ifstream fixture(base+".tsv");if(!fixture)throw std::runtime_error("PRT contact fixture missing");unsigned cases=0;std::size_t checked=0;std::string line;
    while(std::getline(fixture,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');if(split==std::string::npos)throw FormatError("PRT contact separator");const auto output=solve(banks,words(line.substr(0,split))),expected=words(line.substr(split+1));if(output!=expected)throw std::runtime_error("PRT contact differs at case "+std::to_string(cases));checked+=output.size();++cases;}
    if(cases!=1227 || checked!=8589 || banks.front().active!=6)throw std::runtime_error("PRT contact independent coverage changed");
    std::ifstream shorts(base+"_short.tsv");if(!shorts)throw std::runtime_error("PRT contact original CPU conversion fixture missing");unsigned short_cases=0;
    while(std::getline(shorts,line)){if(line.empty()||line[0]=='#')continue;const auto split=line.find('|');const auto raw=words(line.substr(0,split)),expected=words(line.substr(split+1));if(raw.size()!=5||expected.size()!=5)throw FormatError("PRT short fixture dimensions");std::array<std::int16_t,5> input;for(unsigned i=0;i<5;++i)input[i]=std::bit_cast<std::int16_t>(std::uint16_t(raw[i]));const auto point=source_prt_contact_short_point(input);Words out{std::bit_cast<std::uint32_t>(point.cluster)};for(auto v:point.weights)out.push_back(std::bit_cast<std::uint32_t>(v));if(out!=expected)throw std::runtime_error("PRT source CPU short decode differs");++short_cases;}
    if(short_cases!=24)throw std::runtime_error("actual PRT CPU conversion coverage changed");
    std::array<SourcePrtContactPoint,3> points{};const auto query=[&](const Bank& bank,unsigned section){return source_prt_contact_sample(bank.rows,bank.active,bank.arena,0xe0010020,section,0,points,.2f,.3f);};
    auto bad=banks[1];bad.active=2;reject([&]{query(bad,0);});
    bad=banks[1];bad.rows[0].words[2]=0xffffffffu;reject([&]{query(bad,0);});
    bad=banks[1];bad.arena.clear();reject([&]{query(bad,0);});
    reject([&]{query(banks[1],16);});
    bad=banks[1];bad.active=-1;bad.rows.clear();bad.arena.clear();if(query(bad,16))throw std::runtime_error("negative prefix acquired source PRT sample");
    // CPU conversion is intentionally not a clamped SNORM GPU conversion.
    const auto shortpoint=source_prt_contact_short_point({3,-32768,-1,0,32767});
    if(shortpoint.cluster!=3 || std::bit_cast<std::uint32_t>(shortpoint.weights[0])!=0xbf800000u || std::bit_cast<std::uint32_t>(shortpoint.weights[3])!=0x3f800000u || shortpoint.weights[1]>=0 || shortpoint.weights[2]<=0)throw std::runtime_error("source CPU PRT midpoint conversion changed");
    std::cout<<cases<<" unchanged241C0 cases, "<<checked<<" exact output words, current cache and CPU quantizer controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
