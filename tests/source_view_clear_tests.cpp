#include "halo2/source_view_clear.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc,char **argv) {
    using namespace halo2;
    if(argc!=2)throw FormatError("Expected unchanged-original clear event fixture");
    std::ifstream input(argv[1]);if(!input)throw FormatError("Missing original clear event fixture");
    std::string type,id;unsigned count=0;
    while(input>>type>>id) {
        const auto word=[&](){std::uint32_t v{};if(!(input>>std::hex>>v))throw FormatError("Truncated original clear fixture");return v;};
        SourceClearViewport viewport;std::vector<SourceClearEvent> actual;
        if(type=="V") {
            std::array<std::byte,0x298> header{};
            const auto mode=word(),temporary=word(),clear=word(),transition=word(),control=word();
            const auto put=[&](unsigned at,std::uint32_t v,unsigned size){for(unsigned i=0;i<size;++i)header[at+i]=std::byte((v>>(i*8))&255);};
            put(2,mode,2);put(7,temporary,1);put(8,clear,2);
            for(unsigned i=0;i<3;++i)put(12+i*4,word(),4);
            for(auto &v:viewport)v=word();
            actual=source_view_initialization_plan(header,std::uint8_t(transition),std::uint16_t(control),viewport);
        } else if(type=="C") {
            const auto flags=word(),color=word(),depth=word(),stencil=word(),temporary=word(),target=word();
            for(auto &v:viewport)v=word();
            actual=source_clear_plan(flags,color,depth,std::uint8_t(stencil),std::uint8_t(temporary),std::bit_cast<std::int32_t>(target),viewport);
        } else throw FormatError("Unknown original clear event fixture row");
        const auto size=word();std::vector<SourceClearEvent> expected;
        for(unsigned i=0;i<size;++i) {
            SourceClearEvent e;e.kind=SourceClearEvent::Kind(word());
            for(auto &v:e.words)v=word();expected.push_back(e);
        }
        if(actual!=expected)throw FormatError("Original clear event order or operands differ: "+id);
        ++count;
    }
    if(!input.eof() || !count)throw FormatError("Incomplete original clear fixture");
    std::cout<<count<<" unchanged-original view/clear event cases passed\n";
}
