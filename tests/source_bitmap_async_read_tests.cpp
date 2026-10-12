#include "halo2/source_bitmap_async_read.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>
#ifdef __linux__
#include <unistd.h>
#endif

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::vector<std::uint32_t> route(const std::vector<std::uint32_t> &input){
    if(input.size()!=13)throw std::invalid_argument("source bitmap routing fixture length differs");
    const auto request=source_bitmap_async_request({input[0],input[1],input[2],input[3],
        input[4],input[5],std::bit_cast<std::int32_t>(input[6])},
        {input[7],input[8],input[9],input[10]});
    SourceAsyncQueue queue;queue.requests_added=input[11];queue.counter=input[12];
    const auto clear=source_async_begin_request(queue,request);
    std::vector<std::uint32_t> out(request.task.words.begin(),request.task.words.end());
    out.insert(out.end(),{std::bit_cast<std::uint32_t>(request.priority),request.category,
        request.callback,request.done,queue.requests_added,clear,0,queue.counter});return out;
}
std::uint64_t hash(std::span<const std::byte> bytes){
    std::uint64_t value=14695981039346656037ull;
    for(auto byte:bytes)value=(value^std::to_integer<unsigned>(byte))*1099511628211ull;
    return value;
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    if(!probe && argc!=3)throw std::invalid_argument("source routing fixture and immutable map directory required");
    std::ifstream fixture;if(!probe){fixture.open(argv[1]);require(bool(fixture),"missing source bitmap routing fixture");}
    auto &input=probe?std::cin:fixture;std::string line;unsigned routes=0,words=0,resources=0;
    std::size_t read_bytes=0;
    const std::filesystem::path directory=probe?std::filesystem::path{}:argv[2];
    const SourceBitmapFileBank files{76,72,64,68};
    const std::array<SourceAsyncOwnedFile,4> bindings{{
        {files.current,directory/"zanzibar.map"},{files.mainmenu,directory/"mainmenu.map"},
        {files.shared,directory/"shared.map"},{files.single_player_shared,directory/"single_player_shared.map"}}};
    std::unique_ptr<SourceAsyncOwnedFiles> owned;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto split=line.find('|');const char kind=probe?'R':line[0];
        const auto start=probe?0:2;
        std::istringstream before(line.substr(start,split==std::string::npos?split:split-start));before>>std::hex;
        std::vector<std::uint32_t> row;std::uint32_t word;while(before>>word)row.push_back(word);
        if(!before.eof())throw std::invalid_argument("source bitmap routing input malformed");
        if(kind=='R'){
            const auto actual=route(row);
            if(probe){for(auto value:actual)std::cout<<std::hex<<value<<' ';std::cout<<'\n';continue;}
            require(split!=std::string::npos,"source bitmap fixture output missing");
            std::istringstream after(line.substr(split+1));after>>std::hex;
            std::vector<std::uint32_t> expected;while(after>>word)expected.push_back(word);
            require(after.eof() && actual==expected,"complete source bitmap routing differs");
            ++routes;words+=unsigned(actual.size());
        }else if(kind=='D'){
            require(row.size()==3 && split!=std::string::npos,"source immutable resource fixture malformed");
            std::uint64_t expected_bytes,expected_hash;std::istringstream after(line.substr(split+1));
            after>>std::hex>>expected_bytes>>expected_hash;require(bool(after),"missing immutable resource hash");
            std::string extra;require(!(after>>extra),"unexpected immutable resource fixture output");
            if(!owned)owned=std::make_unique<SourceAsyncOwnedFiles>(bindings);
            std::vector<std::byte> bytes(row[1],std::byte{0xa5});
            auto request=source_bitmap_async_request({row[0],row[1],row[2],0,0x3000,3,2},files);
            SourceAsyncReadExecution result;unsigned calls=0;
            do{
                result=owned->read(request.task,262144,{row[2],bytes});
                require(++calls<8192,"native read did not reach a source callback completion");
            }while(result.completion.callback_result==0);
            const auto count=request.task.words[5];
            require(count==expected_bytes && count<=bytes.size(),"native read byte count differs from immutable file");
            require(hash(std::span(bytes).first(count))==expected_hash,"native read bytes differ from authored resource");
            require(std::all_of(bytes.begin()+count,bytes.end(),[](auto b){return b==std::byte{0xa5};}),
                "native short read changed unwritten owned bytes");
            require(request.task.words[3]==((row[0]&0x3fffffffu)+count),"source native read offset differs");
            ++resources;read_bytes+=count;
        }else throw std::invalid_argument("unknown source bitmap routing fixture kind");
    }
    if(probe)return 0;
    require(routes==1216 && words==21888 && resources==54 && read_bytes==1454108,
        "source bitmap routing/resource corpus is incomplete");
    unsigned controls=0;
    const auto rejects=[&](auto &&action){bool rejected=false;try{action();}
        catch(const std::invalid_argument&){rejected=true;}require(rejected,"unrepresented native read input admitted");++controls;};
    rejects([&]{source_bitmap_async_request({},files);});
    rejects([&]{SourceAsyncOwnedFiles duplicate(std::array<SourceAsyncOwnedFile,2>{bindings[0],bindings[0]});});
    rejects([&]{SourceAsyncOwnedFiles missing(std::array<SourceAsyncOwnedFile,1>{{{44,directory/"not-an-authored-map.map"}}});});
    std::array<std::byte,64> storage;storage.fill(std::byte{0xa5});
    auto task=source_async_read_task({files.current,0x2000,64,0,0});
    rejects([&]{owned->read(task,64,{0x2001,storage});});
    task.words[0]=123;rejects([&]{owned->read(task,64,{0x2000,storage});});task.words[0]=files.current;
    task.words[2]=65;rejects([&]{owned->read(task,65,{0x2000,storage});});
    task=source_async_read_task({files.current,0xfffffff0,64,0,0});
    rejects([&]{owned->read(task,64,{0xfffffff0,storage});});
    task.words[5]=32;rejects([&]{owned->read(task,32,{0xfffffff0,storage});});
    // Exact low-word sentinel, even if the native regular-file seek succeeds.
    task=source_async_read_task({files.current,0x2000,64,0xffffffffu,0});
    auto sentinel=owned->read(task,64,{0x2000,storage});
    require(sentinel.io.seek_low==source_async_none && sentinel.io.bytes_read==0 &&
        sentinel.completion.callback_result==1 && task.words[5]==0 &&
        std::all_of(storage.begin(),storage.end(),[](auto b){return b==std::byte{0xa5};}),
        "source low seek sentinel read or mutated bytes");++controls;
    // Actual1A0E70 supplies a nonNULL zero high word, not a signed low-only
    // displacement. These native regular-file seeks succeed beyond EOF.
    for(const auto offset:{0x80000000u,0x80000001u,0xfffffffeu}){
        task=source_async_read_task({files.current,0x2000,64,offset,0});
        const auto high_bit=owned->read(task,64,{0x2000,storage});
        require(high_bit.io.seek_low==offset && high_bit.io.bytes_read==0 &&
            high_bit.completion.callback_result==1 && task.words[5]==0 &&
            task.words[3]==offset && std::all_of(storage.begin(),storage.end(),
                [](auto b){return b==std::byte{0xa5};}),
            "source nonNULL zero-high seek became a signed negative displacement");++controls;
    }
    task=source_async_read_task({files.current,0x2000,0,0,0});
    const auto zero=owned->read(task,262144,{0x2000,{}});
    require(zero.io.seek_low==0 && zero.io.bytes_read==0 && zero.completion.callback_result==1,
        "source zero-size native seek/read differs");++controls;
#ifdef __linux__
    // Real nonseekable owned stream. No status/result is injected.
    int descriptors[2];require(pipe(descriptors)==0,"cannot create native nonseekable control");
    try{
        SourceAsyncOwnedFiles nonseekable(std::array<SourceAsyncOwnedFile,1>{{
            {55,std::filesystem::path("/proc/self/fd")/std::to_string(descriptors[0])}}});
        task=source_async_read_task({55,0x2000,64,0,0});
        const auto failed=nonseekable.read(task,64,{0x2000,storage});
        require(failed.io.seek_low==source_async_none && failed.io.bytes_read==0 &&
            failed.completion.callback_result==1 && task.words[5]==0,"actual native failed seek mapping differs");
        ++controls;
    }catch(...){close(descriptors[0]);close(descriptors[1]);throw;}
    close(descriptors[0]);close(descriptors[1]);
#endif
    std::cout<<routes<<" source routing cases; "<<words<<" words exact; "<<resources
        <<" immutable resource reads; "<<read_bytes<<" bytes; "<<controls<<" native mapping controls\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
