#include "halo2/source_transparent_queue.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
void require(bool v,const char *m){if(!v)throw std::runtime_error(m);}
SourceTransparentCommand seed_record(unsigned seed,unsigned index) {
    std::array<std::uint32_t,28> words;
    for(unsigned j=0;j<words.size();++j)words[j]=seed+index*0x1020304u+j*0x1234567u;
    words[0]=index%4;words[1]=std::bit_cast<std::uint32_t>(float(int(index%19)-9));
    words[2]=index%7;words[3]&=0xffffff00u;words[8]=0;
    return std::bit_cast<SourceTransparentCommand>(words);
}
Words evaluate(const Words &input) {
    unsigned at=0;const auto word=[&]{return input.at(at++);};
    const auto real=[&]{return std::bit_cast<float>(word());};
    const auto vec=[&]{return Vec3{real(),real(),real()};};
    const auto seed=word(),n=word();SourceTransparentQueue queue(seed&1);
    for(unsigned i=0;i<queue.capacity;++i)*queue.allocate()=seed_record(seed,i);
    queue.reset_world();Words output;
    const auto record=[&](const SourceTransparentCommand &c){
        const auto w=std::bit_cast<std::array<std::uint32_t,28>>(c);
        output.insert(output.end(),w.begin(),w.end());};
    const auto allocated=[&](const SourceTransparentCommand *c){
        output.push_back(c?unsigned(c-queue.storage().data()):0xffffffffu);
        if(c)record(*c);};
    for(unsigned i=0;i<n;++i){const auto op=word();
        if(op==0)queue.reset_world();
        else if(op==1)allocated(queue.allocate());
        else if(op==2)queue.begin_first_person_suffix();
        else if(op==3){const auto order=queue.ordered_ordinals(vec());output.push_back(unsigned(order.size()));
            for(auto index:order)output.push_back(index);
            unsigned count=0;for(auto index:order)if(queue.commands()[index].callback)++count;
            output.push_back(count);for(auto index:order)if(queue.commands()[index].callback)output.push_back(index);
        }else if(op==4)queue.finish_first_person_suffix();
        else if(op==5){SourceTransparentPartInput p;p.selected_template_layers=word();p.part_type=std::uint16_t(word());
            p.material_present=word();p.view_type=word();p.grouped_context=word();p.shader_flags=std::uint16_t(word());
            p.selected_shader=word();p.draw_token=word();p.center=vec();p.camera_position=vec();p.camera_forward=vec();
            p.depth_bias=real();p.plane_present=word();for(auto &v:p.plane)v=real();allocated(queue.append_part(p));
        }else if(op==6){std::array<std::uint32_t,28> w;for(auto &v:w)v=word();
            auto *c=queue.allocate();if(c)*c=std::bit_cast<SourceTransparentCommand>(w);allocated(c);
        }else if(op==7){const auto center=vec(),eye=vec(),forward=vec();const auto callback=word(),size=word();
            Words payload((size+3)/4);for(auto &v:payload)v=word();
            allocated(queue.append_callback(center,eye,forward,callback,
                {reinterpret_cast<const std::uint8_t *>(payload.data()),size}));
        }else throw std::runtime_error("unknown transparent queue fixture operation");
        output.push_back(queue.count());output.push_back(queue.first());output.push_back(queue.overflow_notice_pending());
    }
    for(unsigned index:{0u,1u,2u,7u,31u,511u,1022u,1023u})record(queue.storage()[index]);
    require(at==input.size(),"trailing transparent queue input");return output;
}
void first_person_trace(const char *path) {
    std::ifstream f(path);require(bool(f),"original FP suffix fixture missing");std::string line;unsigned count=0;
    while(std::getline(f,line)){if(line.empty() || line[0]=='#')continue;
        std::istringstream s(line);unsigned prefix,type,drain,admit,view,special,want_count,want_first,draws,callbacks;
        require(bool(s>>prefix>>type>>drain>>admit>>view>>special>>want_count>>want_first>>draws>>callbacks),"FP fixture truncated");
        SourceTransparentQueue q;for(unsigned i=0;i<prefix;++i)require(q.allocate(),"invalid FP prefix");
        SourceTransparentPartInput p;p.selected_template_layers=0x8000;p.part_type=std::uint16_t(type);p.selected_shader=3;
        //Original3FDF0's selector derives a requested mask; it passes view0 to
        //41040 for every input view, including the independent specialized bank.
        for(unsigned selector=0;selector<2;++selector){q.begin_first_person_suffix();q.append_part(p);}
        const auto order=drain?q.ordered_ordinals({}):std::vector<std::uint16_t>{};
        require(order.size()==callbacks,"original FP drain callback count differs");
        for(auto ordinal:order){unsigned expected;require(bool(s>>expected),"FP ordinal truncated");require(ordinal==expected,"original FP suffix ordinal differs");}
        require(unsigned(order.size())*admit==draws,"original FP pass admission differs");
        if(drain)q.finish_first_person_suffix();
        require(q.count()==want_count && q.first()==want_first,"original FP suffix lifecycle differs");++count;
    }
    require(count==111,"original FP trace corpus changed");std::cout<<count<<" original FP caller/producer/drain traces matched\n";
}
}
int main(int argc,char **argv){try {
    require(argc>=2,"transparent queue fixture required");const bool probe=std::string(argv[1])=="--probe";
    std::ifstream file;if(!probe){file.open(argv[1]);require(bool(file),"transparent queue fixture missing");}
    auto &stream=probe?std::cin:file;std::string line;unsigned count=0;
    while(std::getline(stream,line)){if(line.empty() || line[0]=='#')continue;std::istringstream s(line);s>>std::hex;
        unsigned n;require(bool(s>>n),"queue fixture count missing");Words row(n);for(auto &w:row)require(bool(s>>w),"queue fixture input truncated");
        const auto output=evaluate(row);if(probe){for(auto w:output)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
        else {for(unsigned j=0;j<output.size();++j){unsigned expected;require(bool(s>>expected),"queue fixture output truncated");
            if(output[j]!=expected)throw std::runtime_error("original queue mismatch case "+std::to_string(count)+" word "+std::to_string(j));}
            std::string extra;require(!(s>>extra),"trailing queue fixture output");}++count;
    }
    require(count,"empty queue fixture");if(!probe){std::cout<<count<<" original persistent queue operation streams matched\n";
        require(argc==3,"original FP trace fixture required");first_person_trace(argv[2]);}
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
