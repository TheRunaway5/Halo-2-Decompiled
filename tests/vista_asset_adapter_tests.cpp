#include "halo2/cache.hpp"
#include "halo2/model.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
const Tag& tag(const Cache& cache,const std::string& group,const std::string& name) {
    const auto it=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const Tag& t){return t.group==group && t.name==name;});
    check(it!=cache.tags().end(),"Required source tag is missing");return *it;
}
void same(Texture a,Texture b) {
    check(a.width==b.width && a.height==b.height && a.format==b.format &&
          a.depth==b.depth && a.layers==b.layers && a.pixels==b.pixels &&
          a.mips.size()==b.mips.size(),"Vista binding changed Xbox bitmap pixels/description");
    for(std::size_t i=0;i<a.mips.size();++i) {
        const auto& x=a.mips[i];const auto& y=b.mips[i];
        check(x.width==y.width && x.height==y.height && x.depth==y.depth &&
              x.offset==y.offset && x.size==y.size,"Vista binding changed Xbox mip layout");
    }
}
}
int main(int argc,char** argv) {
    const auto root=std::filesystem::temp_directory_path()/
        ("halo2-vista-assets-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        check(argc==3,"Supply Vista and Xbox maps directories");
        const auto vista_maps=std::filesystem::absolute(argv[1]),xbox_maps=std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(root);
        std::filesystem::create_directory_symlink(vista_maps,root/"maps");
        std::ofstream(root/"asset-donors.tsv")<<"H2_VISTA_DONORS_V1\nxbox\t"<<xbox_maps.parent_path().string()<<'\n';
        const Cache vista(root/"maps/lockout.map"),xbox(xbox_maps/"lockout.map");
        for(const auto* name:{"objects\\weapons\\rifle\\battle_rifle\\bitmaps\\battle_rifle",
                              "objects\\weapons\\rifle\\battle_rifle\\bitmaps\\battle_rifle_bump"}) {
            const auto v=tag(vista,"bitm",name).datum,x=tag(xbox,"bitm",name).datum;
            const auto texture=vista.load_bitmap(v,0);
            check(texture.width==256 && texture.height==256,"Original BR bitmap dimensions changed");
            same(texture,xbox.load_bitmap(x,0));
            const auto a=vista.load_bitmap_resource(v,0),b=xbox.load_bitmap_resource(x,0);
            check(a.width==b.width && a.height==b.height && a.format==b.format &&
                  a.type==b.type && a.depth==b.depth && a.levels==b.levels,"Texture residency did not use Xbox bytes");
            const auto low_a=vista.load_bitmap_low_detail_resource(v,0),low_b=xbox.load_bitmap_low_detail_resource(x,0);
            check(bool(low_a)==bool(low_b),"Low-detail donor presence differs");
            if(low_a)check(low_a->pixels==low_b->pixels && low_a->width==low_b->width &&
                           low_a->height==low_b->height && low_a->format==low_b->format,"Low-detail donor differs");
        }
        const auto name="objects\\characters\\masterchief\\fp\\weapons\\rifle\\fp_smg\\fp_smg";
        const auto v=tag(vista,"jmad",name).datum,x=tag(xbox,"jmad",name).datum;
        bool raw_graph_rejected=false;
        try {(void)vista.load_source_unit_animation_graph_metadata(v);}catch(const FormatError&) {raw_graph_rejected=true;}
        check(raw_graph_rejected,"Vista clips were admitted to the Xbox unit-animation raw layout");
        unsigned decoded=0;
        for(unsigned i=0;i<33;++i) {
            AnimationClip a,b;std::string vista_error,xbox_error;
            try {a=vista.load_animation(v,i);}catch(const FormatError& e){vista_error=e.what();}
            try {b=xbox.load_animation(x,i);}catch(const FormatError& e){xbox_error=e.what();}
            if(!vista_error.empty() || !xbox_error.empty()) {
                throw FormatError("SMG animation decode failed: Vista="+vista_error+" Xbox="+xbox_error);
            }
            ++decoded;
            check(a.frame_count==b.frame_count && a.loop_frame==b.loop_frame &&
                  a.authored_flags==b.authored_flags && a.parents==b.parents &&
                  a.components==b.components && a.frames.size()==b.frames.size() &&
                  a.channels.size()==b.channels.size() && a.nodes.size()==b.nodes.size() &&
                  a.frame_events.size()==b.frame_events.size(),"SMG animation metadata differs");
            for(std::size_t j=0;j<a.nodes.size();++j)
                check(vista.string_id(a.nodes[j])==xbox.string_id(b.nodes[j]),"Animation node name differs");
            for(std::size_t j=0;j<a.frame_events.size();++j)
                check(a.frame_events[j].type==b.frame_events[j].type && a.frame_events[j].frame==b.frame_events[j].frame,"Animation event differs");
            for(std::size_t j=0;j<a.frames.size();++j) {
                const auto& p=a.frames[j];const auto& q=b.frames[j];
                check(p.rotation==q.rotation && p.scale==q.scale && p.translation.x==q.translation.x &&
                      p.translation.y==q.translation.y && p.translation.z==q.translation.z,"Decoded SMG pose differs");
            }
            for(std::size_t j=0;j<a.channels.size();++j)for(unsigned k=0;k<3;++k)
                check(a.channels[j][k].times==b.channels[j][k].times &&
                      a.channels[j][k].keys==b.channels[j][k].keys,"Retained SMG animation channels differ");
        }
        check(decoded==33,"SMG codec coverage changed");
        std::filesystem::remove_all(root);
        std::cout<<"Original256x256BR texture/mips/residency and33 SMG animation poses/channels/events match Xbox, including codec8\n";
        return 0;
    } catch(const std::exception& error) {
        std::filesystem::remove_all(root);std::cerr<<error.what()<<'\n';return 1;
    }
}
