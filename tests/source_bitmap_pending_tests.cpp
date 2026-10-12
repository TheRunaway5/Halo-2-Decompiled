#include "halo2/source_bitmap_pending.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace halo2;
namespace {
constexpr std::uint32_t none=0xffffffffu,async_address=0x4e0364,loading_address=0x4e6484;
constexpr std::size_t async_bytes=9616;
struct Region {std::uint32_t address{};std::vector<std::byte> bytes;};
using Table=std::map<std::string,Region>;
struct Patch {std::string name;std::size_t offset{};std::vector<std::byte> bytes;};
using InputWords=std::array<std::uint32_t,20>;
struct Seed {Table table;InputWords words;};

std::vector<std::byte> decode(const std::string &hex) {
    if(hex.empty() || hex.size()%2)throw std::invalid_argument("pending fixture hex byte length malformed");
    const auto nibble=[](char c)->unsigned {
        if(c>='0' && c<='9')return unsigned(c-'0');
        if(c>='a' && c<='f')return unsigned(c-'a'+10);
        if(c>='A' && c<='F')return unsigned(c-'A'+10);
        throw std::invalid_argument("pending fixture nonhex byte");
    };
    std::vector<std::byte> out;out.reserve(hex.size()/2);
    for(std::size_t i=0;i<hex.size();i+=2)out.push_back(std::byte(nibble(hex[i])*16+nibble(hex[i+1])));
    return out;
}
std::string encode(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);
    for(const auto byte:bytes) {const auto value=std::to_integer<unsigned>(byte);out+=digits[value>>4];out+=digits[value&15];}
    return out;
}
std::uint32_t get(std::span<const std::byte> bytes,std::size_t at) {
    if(at>bytes.size() || bytes.size()-at<4)throw std::invalid_argument("pending raw bank word missing");
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned char>(bytes[at+i]))<<(i*8);
    return value;
}
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value) {
    if(at>bytes.size() || bytes.size()-at<4)throw std::invalid_argument("pending raw output word missing");
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte((value>>(i*8))&255u);
}
void put_word(std::span<std::byte> bytes,std::size_t at,std::uint16_t value) {
    if(at>bytes.size() || bytes.size()-at<2)throw std::invalid_argument("pending raw output short missing");
    bytes[at]=std::byte(value&255u);bytes[at+1]=std::byte(value>>8);
}
Region &addressed(Table &table,std::uint32_t address) {
    Region *found=nullptr;
    for(auto &[name,region]:table)if(region.address==address) {
        if(found)throw std::invalid_argument("pending fixture duplicate bank address");
        found=&region;
    }
    if(!found)throw std::invalid_argument("pending fixture literal raw bank absent");
    return *found;
}
std::uint32_t node_index(std::uint32_t pointer) {
    if(!pointer)return none;
    constexpr auto nodes=async_address+4;
    if(pointer<nodes || pointer-nodes>=150u*64u || (pointer-nodes)%64u)
        throw std::invalid_argument("pending fixture async link is not an original node pointer");
    return (pointer-nodes)/64u;
}
std::uint32_t node_pointer(std::uint32_t index) {
    if(index==none)return 0;
    if(index>=150)throw std::invalid_argument("pending native async link outside original150 nodes");
    return async_address+4+index*64;
}
SourceAsyncQueue queue_from_raw(const Region &region) {
    if(region.address!=async_address || region.bytes.size()!=async_bytes)
        throw std::invalid_argument("pending fixture literal async bank shape differs");
    SourceAsyncQueue queue;queue.counter=get(region.bytes,0);
    for(unsigned i=0;i<150;++i) {
        auto &node=queue.nodes[i];const auto at=4+i*64;
        node.priority=std::bit_cast<std::int32_t>(get(region.bytes,at));node.job=get(region.bytes,at+4);
        for(unsigned word=0;word<10;++word)node.task.words[word]=get(region.bytes,at+8+word*4);
        node.category=get(region.bytes,at+48);node.callback=get(region.bytes,at+52);node.done=get(region.bytes,at+56);
        node.next=node_index(get(region.bytes,at+60));
    }
    queue.free_head=node_index(get(region.bytes,9604));queue.work_head=node_index(get(region.bytes,9608));
    queue.requests_added=get(region.bytes,9612);return queue;
}
std::vector<std::byte> queue_to_raw(const SourceAsyncQueue &queue) {
    // TypedQueue has nodes first and its counter near the end. Reencode every
    // original field independently; no typed-object memcpy/bitcast is valid.
    std::vector<std::byte> bytes(async_bytes);put(bytes,0,queue.counter);
    for(unsigned i=0;i<150;++i) {
        const auto &node=queue.nodes[i];const auto at=4+i*64;
        put(bytes,at,std::bit_cast<std::uint32_t>(node.priority));put(bytes,at+4,node.job);
        for(unsigned word=0;word<10;++word)put(bytes,at+8+word*4,node.task.words[word]);
        put(bytes,at+48,node.category);put(bytes,at+52,node.callback);put(bytes,at+56,node.done);
        put(bytes,at+60,node_pointer(node.next));
    }
    put(bytes,9604,node_pointer(queue.free_head));put(bytes,9608,node_pointer(queue.work_head));
    put(bytes,9612,queue.requests_added);return bytes;
}
std::array<SourceBitmapManagerRegion,7> core(const SourceBitmapManagerBanks &bank) {
    return {bank.manager,bank.image_pool,bank.physical_pool,bank.image_rows,
        bank.physical_rows,bank.image_bits,bank.physical_bits};
}
SourceBitmapManagerBanks banks(Table &table,std::vector<SourceBitmapManagerRegion> &owners) {
    const auto region=[&](const std::string &name) {auto &r=table.at(name);return SourceBitmapManagerRegion{r.address,r.bytes};};
    for(auto &[name,r]:table)if(name.starts_with("owner_"))owners.push_back({r.address,r.bytes});
    return {region("manager"),region("image_pool"),region("physical_pool"),region("image_rows"),
        region("physical_rows"),region("image_bits"),region("physical_bits"),owners};
}
SourceBitmapPendingInput request(const InputWords &words) {
    if(words[2]>255 || words[3]>255)throw std::invalid_argument("pending fixture wait/force byte malformed");
    return {words[0],words[1],std::uint8_t(words[2]),std::uint8_t(words[3]),std::bit_cast<std::int32_t>(words[4])};
}
SourceBitmapPendingContext context(const InputWords &words) {
    if(words[5]>255 || words[16]>1 || words[18]>1)throw std::invalid_argument("pending fixture context presence malformed");
    SourceBitmapPendingContext out;
    out.manager={std::uint8_t(words[5]),SourceBitmapManagerFence{words[6],words[7]}};
    out.physical_base=words[8];out.physical_storage={words[9],words[10]};
    out.files={words[11],words[12],words[13],words[14]};out.queue_address=words[15];
    if(words[16])out.free_mutex=words[17];
    if(words[18])out.work_mutex=words[19];
    return out;
}
std::array<std::uint32_t,28> result_words(const SourceBitmapPendingResult &result) {
    std::array<std::uint32_t,28> words{};
    words[0]=std::uint32_t(result.boundary);words[1]=result.handle;words[2]=result.pending_delete;
    words[3]=result.mutex.has_value();words[4]=result.mutex.value_or(0);words[5]=result.read.has_value();
    if(result.read) {
        const auto &read=*result.read;
        const std::array<std::uint32_t,7> fields{read.location,read.size,read.buffer,read.bytes_output,
            read.done,read.category,std::bit_cast<std::uint32_t>(read.priority)};
        std::copy(fields.begin(),fields.end(),words.begin()+6);
    }
    words[13]=result.request.has_value();
    if(result.request) {
        const auto &read=*result.request;std::copy(read.task.words.begin(),read.task.words.end(),words.begin()+14);
        words[24]=std::bit_cast<std::uint32_t>(read.priority);words[25]=read.category;words[26]=read.callback;words[27]=read.done;
    }
    return words;
}
std::vector<Patch> differences(const Table &before,const Table &after) {
    if(before.size()!=after.size())throw std::runtime_error("pending raw bank inventory changed");
    std::vector<Patch> out;
    for(const auto &[name,r]:before) {
        const auto &final=after.at(name);
        if(r.address!=final.address || r.bytes.size()!=final.bytes.size())throw std::runtime_error("pending raw bank shape changed");
        std::size_t at=0;
        while(at<r.bytes.size()) {
            if(r.bytes[at]==final.bytes[at]) {++at;continue;}
            const auto start=at;while(at<r.bytes.size() && r.bytes[at]!=final.bytes[at])++at;
            out.push_back({name,start,{final.bytes.begin()+std::ptrdiff_t(start),final.bytes.begin()+std::ptrdiff_t(at)}});
        }
    }
    return out;
}
void patches(std::istringstream &row,Table &table) {
    std::size_t count;if(!(row>>std::dec>>count))throw std::invalid_argument("pending fixture patch count missing");
    for(std::size_t i=0;i<count;++i) {
        std::string name,hex;std::size_t offset;
        if(!(row>>name>>std::hex>>offset>>hex))throw std::invalid_argument("pending fixture patch malformed");
        auto change=decode(hex);auto &r=table.at(name);
        if(offset>r.bytes.size() || change.size()>r.bytes.size()-offset)throw std::invalid_argument("pending fixture patch outside bank");
        std::copy(change.begin(),change.end(),r.bytes.begin()+std::ptrdiff_t(offset));
    }
}
void phase(const Table &before,Table &after,const SourceBitmapPendingResult &result,
    const InputWords &input,const SourceAsyncQueue &queue) {
    auto original=before;const auto &raw=addressed(original,async_address);
    if(addressed(after,loading_address).bytes!=addressed(original,loading_address).bytes)
        throw std::runtime_error("pending prefix changed loading before acquisition");
    auto expected=raw.bytes;
    if(result.boundary==SourceBitmapPendingBoundary::NeedsFreeMutexAcquire) {
        if(result.handle==none || result.pending_delete!=none || !result.read || !result.request ||
            !input[16] || !result.mutex || *result.mutex!=input[17])
            throw std::runtime_error("pending free-acquisition boundary fields differ");
        put(expected,9612,get(expected,9612)+1u);
        const auto &images=after.at("image_rows");const auto offset=std::size_t(result.handle&65535u)*40;
        if(offset>images.bytes.size() || images.bytes.size()-offset<40 ||
            images.bytes[offset+3]!=std::byte{} || (std::to_integer<unsigned char>(images.bytes[offset+2])&1u) ||
            get(images.bytes,offset+8) || get(images.bytes,offset+12))
            throw std::runtime_error("pending prefix supplied ready/job/final priority before acquisition");
        const auto done=images.address+std::uint32_t(offset)+3;
        if(result.read->done!=done || result.request->done!=done || result.read->bytes_output ||
            result.read->category!=3 || result.request->category!=3 ||
            result.request->callback!=source_async_read_callback)
            throw std::runtime_error("pending boundary actual done/read metadata differs");
    } else {
        if(result.handle!=none || result.read || result.request)
            throw std::runtime_error("pending failure/work boundary manufactured an acquired image request");
        if(result.boundary==SourceBitmapPendingBoundary::CompletedSourceFailure) {
            if(result.pending_delete!=none || result.mutex)
                throw std::runtime_error("pending source failure was relabeled as a mutex boundary");
        } else if(result.boundary==SourceBitmapPendingBoundary::NeedsWorkMutexAcquire) {
            if(result.pending_delete==none || !input[18] || !result.mutex || *result.mutex!=input[19])
                throw std::runtime_error("pending work-acquisition boundary fields differ");
        } else throw std::runtime_error("pending unknown phase admitted");
    }
    if(queue_to_raw(queue)!=expected)throw std::runtime_error("pending prefix altered async nodes/IDs/heads beyond its actual request counter");
}
std::vector<std::tuple<std::uint32_t,const std::byte*,std::size_t>> views(const SourceBitmapManagerBanks &bank) {
    std::vector<std::tuple<std::uint32_t,const std::byte*,std::size_t>> out;
    for(const auto &r:core(bank))out.emplace_back(r.address,r.bytes.data(),r.bytes.size());
    for(const auto &r:bank.owners)out.emplace_back(r.address,r.bytes.data(),r.bytes.size());
    return out;
}
SourceBitmapManagerRegion &selected(SourceBitmapManagerBanks &bank,const SourceBitmapPendingInput &input) {
    for(auto &owner:bank.owners)if(owner.address==input.owner_address)return owner;
    throw std::runtime_error("pending guard seed selected owner absent");
}
unsigned negatives(const Seed &free_seed) {
    unsigned controls=0;
    const auto reject=[&](auto configure,const Seed &seed) {
        auto table=seed.table;auto queue=queue_from_raw(addressed(table,async_address));
        std::vector<SourceBitmapManagerRegion> owners;auto bank=banks(table,owners);
        auto input=request(seed.words);auto actual_context=context(seed.words);
        configure(bank,queue,input,actual_context);
        const auto before=table;const auto queue_before=queue_to_raw(queue);const auto view_before=views(bank);
        bool failed=false;
        try {source_bitmap_begin_pending(bank,queue,input,actual_context);}catch(const std::invalid_argument &) {failed=true;}
        if(!failed || !differences(before,table).empty() || queue_to_raw(queue)!=queue_before || views(bank)!=view_before)
            throw std::runtime_error("pending invalid metadata/storage/context admitted or mutated at guard "+std::to_string(controls));
        ++controls;
    };
    const auto region=[](SourceBitmapManagerBanks &bank,unsigned i)->SourceBitmapManagerRegion& {
        switch(i) {case 0:return bank.manager;case 1:return bank.image_pool;case 2:return bank.physical_pool;
            case 3:return bank.image_rows;case 4:return bank.physical_rows;case 5:return bank.image_bits;default:return bank.physical_bits;}
    };
    constexpr std::array<std::size_t,7> required{112,76,76,512*40,512*24,64,64};
    for(unsigned i=0;i<7;++i) {
        reject([&](auto &bank,auto &,auto &,auto &) {auto &r=region(bank,i);r.bytes=r.bytes.first(required[i]-1);},free_seed);
        reject([&](auto &bank,auto &,auto &,auto &) {region(bank,i).address=0;},free_seed);
        reject([&](auto &bank,auto &,auto &,auto &) {region(bank,i).address=0xfffffff0u;},free_seed);
    }
    reject([](auto &bank,auto &,auto &,auto &) {bank.owners[0].bytes=bank.owners[0].bytes.first(115);},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {bank.owners[0].address=0;},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {bank.owners[0].address=0xfffffff0u;},free_seed);
    reject([](auto &,auto &,auto &input,auto &) {input.owner_address=0;},free_seed);
    for(const auto partition:{3u,none})reject([&](auto &,auto &,auto &input,auto &) {input.partition=partition;},free_seed);
    for(const std::uint16_t format:{std::uint16_t(0xffff),std::uint16_t(24)})
        reject([&](auto &bank,auto &,auto &input,auto &) {put_word(selected(bank,input).bytes,12,format);},free_seed);
    for(const auto size:{0u,none})reject([&](auto &bank,auto &,auto &input,auto &) {
        put(selected(bank,input).bytes,0x34+input.partition*4,size);},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {put(bank.image_pool.bytes,0x20,511);},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {put(bank.image_pool.bytes,0x24,39);},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {put(bank.image_pool.bytes,0x44,0);},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {put(bank.manager.bytes,0x30,none);},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {put(bank.manager.bytes,0x34,11);},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {put(bank.manager.bytes,0x2c,1);},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.physical_storage.address=0;},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.physical_storage.bytes=0;},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.physical_storage.address=c.physical_base+1;},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.physical_storage={0xfffffff0u,256};},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.physical_base=c.physical_storage.address-1;},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.physical_base=c.physical_storage.address+c.physical_storage.bytes;},free_seed);
    reject([](auto &bank,auto &,auto &,auto &c) {
        c.physical_storage.address=bank.manager.address;c.physical_base=bank.manager.address;},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {bank.image_bits.address=bank.physical_bits.address;},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {bank.image_bits.bytes=bank.physical_bits.bytes;},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {bank.owners[0].address=bank.owners[1].address;},free_seed);
    reject([](auto &bank,auto &,auto &,auto &) {bank.owners[0].bytes=bank.owners[1].bytes;},free_seed);
    reject([](auto &bank,auto &,auto &,auto &c) {c.queue_address=bank.manager.address;},free_seed);
    reject([](auto &bank,auto &queue,auto &,auto &) {
        bank.manager.bytes=std::as_writable_bytes(std::span<SourceAsyncQueue>(&queue,1)).first(112);},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.queue_address=0;},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.queue_address=0xfffffff0u;},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.queue_address=c.physical_storage.address;},free_seed);
    reject([](auto &,auto &,auto &,auto &c) {c.free_mutex.reset();},free_seed);
    reject([](auto &bank,auto &,auto &,auto &c) {
        const auto at=std::size_t(get(bank.manager.bytes,0x3c)&65535u)*40;
        auto row=bank.image_rows.bytes.subspan(at,40);row[2]=std::byte{};row[3]=std::byte{1};
        put(row,12,0);put(row,0x14,0x40001);put(row,0x1c,1);c.manager.fence.reset();},free_seed);
    reject([](auto &bank,auto &,auto &,auto &c) {
        const auto at=std::size_t(get(bank.manager.bytes,0x3c)&65535u)*40;
        bank.image_rows.bytes[at+2]=std::byte{2};c.manager.cache_busy_override.reset();},free_seed);
    if(controls!=56)throw std::runtime_error("pending frozen guard corpus incomplete");
    return controls;
}
} // namespace

int main(int argc,char **argv) {try {
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream file;
    if(!probe) {
        if(argc!=2)throw std::invalid_argument("pending fixture required");
        file.open(argv[1]);if(!file)throw std::invalid_argument("pending fixture missing");
    }
    auto &input=probe?std::cin:file;Table initial;std::string line;
    std::size_t cases=0,checked_bytes=0;std::array<unsigned,3> phases{};
    std::optional<Seed> free_seed;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        const auto divider=line.find('|');std::istringstream row(line.substr(0,divider));std::string tag;row>>tag;
        if(tag=="B") {
            std::string name,hex,extra;std::uint32_t address;
            if(!(row>>name>>std::hex>>address>>hex) || row>>extra ||
                !initial.emplace(name,Region{address,decode(hex)}).second)
                throw std::invalid_argument("pending fixture bank malformed/duplicate");
            continue;
        }
        if(tag!="P")throw std::invalid_argument("pending fixture kind malformed");
        InputWords source;row>>std::hex;
        for(auto &word:source)if(!(row>>word))throw std::invalid_argument("pending fixture input truncated");
        auto actual=initial;patches(row,actual);std::string extra;
        if(row>>extra)throw std::invalid_argument("pending fixture unexpected input");
        auto &async=addressed(actual,async_address);const auto &loading=addressed(actual,loading_address);
        if(loading.bytes.size()!=4)throw std::invalid_argument("pending fixture loading word shape differs");
        auto queue=queue_from_raw(async);
        if(queue_to_raw(queue)!=async.bytes)throw std::runtime_error("pending literal async mapping lost an original word");
        const auto before=actual;std::vector<SourceBitmapManagerRegion> owners;auto bank=banks(actual,owners);
        const auto result=source_bitmap_begin_pending(bank,queue,request(source),context(source));
        phase(before,actual,result,source,queue);async.bytes=queue_to_raw(queue);
        const auto words=result_words(result);
        if(probe) {
            for(unsigned i=0;i<words.size();++i)
                std::cout<<(i?" ":"")<<std::hex<<std::uppercase<<std::setfill('0')<<std::setw(8)<<words[i];
            const auto changes=differences(before,actual);std::cout<<' '<<std::dec<<changes.size();
            for(const auto &change:changes)
                std::cout<<' '<<change.name<<' '<<std::hex<<change.offset<<' '<<encode(change.bytes);
            std::cout<<'\n';continue;
        }
        if(divider==std::string::npos)throw std::invalid_argument("pending fixture separator missing");
        std::istringstream output(line.substr(divider+1));output>>std::hex;
        std::array<std::uint32_t,28> expected_words;
        for(auto &word:expected_words)if(!(output>>word))throw std::invalid_argument("pending fixture output truncated");
        auto expected=before;patches(output,expected);
        if(output>>extra || words!=expected_words)
            throw std::runtime_error("original pending result differs at case "+std::to_string(cases));
        for(const auto &[name,r]:expected) {
            if(actual.at(name).address!=r.address || actual.at(name).bytes!=r.bytes)
                throw std::runtime_error("original pending bank "+name+" differs at case "+std::to_string(cases));
            checked_bytes+=r.bytes.size();
        }
        const auto phase_index=std::uint32_t(result.boundary);if(phase_index>=phases.size())throw std::runtime_error("pending phase outside proved enum");
        ++phases[phase_index];
        if(result.boundary==SourceBitmapPendingBoundary::NeedsFreeMutexAcquire && !free_seed)free_seed=Seed{before,source};
        ++cases;
    }
    if(!input.eof())throw std::invalid_argument("pending fixture stream malformed");
    if(!probe) {
        if(cases!=248 || checked_bytes!=18635712 || !free_seed || phases!=std::array<unsigned,3>{6,0,242})
            throw std::runtime_error("pending frozen corpus incomplete: expected248 cases/18635712 bytes/phases6,0,242");
        std::cout<<cases<<" source pending cases; "<<checked_bytes<<" bank bytes exact; "
            <<negatives(*free_seed)<<" admission/phase controls\n";
    }
    return 0;
}catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
