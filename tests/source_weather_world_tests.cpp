#include "halo2/source_weather_world.hpp"
#include "halo2/source_weather_initialization.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t value;std::memcpy(&value,b.data()+p,4);return value;
}
void store(std::span<std::byte> b,std::size_t p,std::uint32_t value){std::memcpy(b.data()+p,&value,4);}
std::vector<std::byte> copy(std::span<const std::byte> b){return {b.begin(),b.end()};}
const std::array<std::string,12> disc{"ascension","beavercreek","burial_mounds","coagulation","colossus",
    "cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar"};
const std::array<std::string,9> pack{"containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"};
struct TemporaryVista {
    std::filesystem::path path;
    ~TemporaryVista(){std::error_code error;std::filesystem::remove_all(path,error);}
};
SourceWeatherParticles particle(const std::filesystem::path &path) {
    Cache cache(path);const auto bsp=cache.load_source_weather_bsp();
    const auto definition=cache.load_source_weather_definition(bsp.palette.front().datum);
    require(definition.particles.size()==1,"probe expects one actual authored system");return definition.particles.front();
}
void probe(const std::filesystem::path &root) {
    const std::array source{particle(root/"assets/disc/maps/lockout.map"),
        particle(root/"assets/map-pack/maps/warlock.map"),particle(root/"assets/map-pack/maps/backwash.map")};
    for(std::string line;std::getline(std::cin,line);) {
        if(line.empty())continue;
        std::istringstream input(line);input>>std::hex;
        std::uint32_t map,count,clock,phase,next;
        require(bool(input>>map>>count>>clock>>phase>>next),"resource probe input incomplete");
        auto p=source.at(map);require(count<=p.maximum_particles,"probe count exceeds literal allocation");
        for(auto &descriptor:p.resources) {
            const auto stride=std::to_integer<unsigned>(descriptor.descriptor[6])|
                std::to_integer<unsigned>(descriptor.descriptor[7])<<8;
            store(descriptor.descriptor,8,count*stride);
        }
        SourceWeatherParticleResource resource(p);const bool ready=resource.request(std::bit_cast<std::int32_t>(clock));
        if(phase==1)resource.request(std::bit_cast<std::int32_t>(next));
        if(phase==2)resource.rebase_age();
        const auto raw=resource.raw_storage();const auto section=word(p.authored,0x48);
        require(std::ranges::equal(raw.subspan(8+section),std::span(p.resource_storage).subspan(8+section)),
            "resource publication modified a literal particle payload");
        for(unsigned at=0;at<0x8c;at+=4)std::cout<<std::hex<<word(resource.system(),at)<<' ';
        for(unsigned at=8;at<8+section;at+=4)std::cout<<std::hex<<word(raw,at)<<' ';
        std::cout<<std::hex<<std::bit_cast<std::uint32_t>(*resource.last_use())<<' '<<ready<<'\n';
    }
}
void map_check(const std::filesystem::path &path,unsigned &maps,unsigned &weather,unsigned &views) {
    Cache cache(path);const auto atmosphere=cache.load_bsp_atmosphere(0);const auto portals=cache.load_source_portal_bsp(0);
    SourceWeatherWorld world(cache,0);
    require(world.bsp().cluster_palette.size()==atmosphere.clusters.size(),"weather owner lost BSP cluster bank");
    require(!world.initialized(),"resource publication invented an auxiliary stream");
    unsigned systems=0,plates=0,particles=0;
    for(const auto &[datum,definition]:world.definitions()) {
        ++weather;plates+=definition.definition.plates.size();
        for(const auto &resource:definition.particles) {
            require(resource.published() && resource.last_use()==0,"loaded native resource was not published");
            require(word(resource.system(),0x64)==1 && word(resource.system(),0x68)==resource.raw_token()+8,
                "published system does not address its owned raw header");
            ++systems;
        }
        for(const auto &p:definition.definition.particles)particles+=p.maximum_particles;
    }
    std::uint32_t auxiliary=0x87654321,expected=auxiliary;
    world.initialize(auxiliary);
    // All actual supplied palettes have one reference. The shared stream must
    // consume the original cold constructors'36/11 draws in the real owner.
    for(unsigned i=0;i<plates*36+particles*11;++i)expected=expected*0x19660du+0x3c6ef35fu;
    require(auxiliary==expected && world.initialized(),"shared weather constructor draw chronology differs");
    for(const auto &[datum,entry]:world.definitions()) {
        auto &definition=world.definition(datum);
        for(auto &resource:definition.particles) {
            require(resource.system()[0x86]==std::byte{1},"published resource was not initialized");
            for(std::size_t at=28;at<resource.state_rows().size();at+=32)
                require(word(resource.state_rows(),at)==0,"cold weather age was not zero");
        }
    }
    std::array<std::uint32_t,16> open;open.fill(UINT32_MAX);
    SourceWeatherState intensity;
    const auto count=std::min<std::size_t>(cache.spawns().size(),4);
    world.begin_frame();
    for(unsigned slot=0;slot<count;++slot) {
        auto eye=cache.spawns()[slot].position;eye.z+=.65f;
        const auto camera=source_frustum_camera({eye,{1,0,0},{0,0,1},.015625f,1024.f,{-1,1,-.75f,.75f}});
        const auto cluster=atmosphere.cluster_at(eye);
        const auto visibility=source_portal_visibility(portals,std::span{&camera,1},cluster,open);
        const auto admitted=world.admit_view(slot,atmosphere,cluster,visibility,eye,intensity);
        std::vector<unsigned> clipped;for(const auto &c:visibility.clusters)clipped.push_back(unsigned(c.source_cluster));
        const auto reference=source_weather_admission(world.bsp(),atmosphere,cluster,clipped,eye);
        require(admitted.size()==reference.size(),"live portal bank lost a weather palette");
        for(unsigned i=0;i<admitted.size();++i)require(admitted[i].visible==reference[i].visible &&
            std::bit_cast<std::uint32_t>(admitted[i].portal_distance)==std::bit_cast<std::uint32_t>(reference[i].portal_distance),
            "actual clipped-cluster admission differs");
        ++views;
    }
    require(auxiliary==expected,"visibility consumed a private auxiliary stream");
    intensity.stopped=1;
    SourcePortalVisibilitySet empty;
    for(const auto &entry:world.admit_view(0,atmosphere,-1,empty,{},intensity))require(!entry.visible,"stopped weather remained admitted");
    world.begin_frame();for(unsigned slot=0;slot<4;++slot)require(world.admission(slot).empty(),"new frame retained an old view");
    SourceWeatherWorld replacement(cache,0);
    require(!replacement.initialized() && replacement.cache_clock()==0,"new map inherited live weather state");
    ++maps;(void)systems;
}
void guards(const SourceWeatherParticles &source) {
    auto p=source;p.resources.front().descriptor[6]=std::byte{};
    SourceWeatherParticleResource malformed(p);const auto before=copy(malformed.system()),raw=copy(malformed.raw_storage());
    bool failed=false;try{malformed.request(9);}catch(const FormatError &){failed=true;}
    require(failed && std::ranges::equal(before,malformed.system()) && std::ranges::equal(raw,malformed.raw_storage()) &&
        !malformed.last_use(),"rejected publication partially mutated its owner");
    SourceWeatherParticleResource resource(source);failed=false;
    try{resource.render_rows();}catch(const FormatError &){failed=true;}
    require(failed,"unpublished resource exposed decoded-copy arrays");
    resource.request(7);auto rows=resource.render_rows();const auto address=rows.data();rows.front()=std::byte{0x5a};
    resource.request(20);
    require(resource.last_use()==20 && resource.render_rows().data()==address && rows.front()==std::byte{0x5a},
        "linked resource request replaced live simulation storage");
    // Submission must observe current owned rows, not decoded cache copies.
    for(std::size_t at=28;at<resource.state_rows().size();at+=32)store(resource.state_rows(),at,0);
    for(unsigned at=0;at<12;at+=4)store(resource.state_rows(),at,0);
    store(resource.state_rows(),28,std::bit_cast<std::uint32_t>(1.f));
    store(resource.render_rows(),0,std::bit_cast<std::uint32_t>(42.f));
    const auto first=resource.submission(1.f/30.f,1,0);
    store(resource.render_rows(),0,std::bit_cast<std::uint32_t>(43.f));
    const auto second=resource.submission(1.f/30.f,1,0);
    const unsigned vertices=source.type==2?2:1;
    require(first.vertices.size()==vertices && second.vertices.size()==vertices && first.lines==(source.type==2) &&
        first.vertices.back().position[0]==42.f && second.vertices.back().position[0]==43.f &&
        resource.submission(1.f/30.f,4,1).vertices.empty(),
        "weather submission did not consume its live owned view partition");
}
}
int main(int argc,char **argv){try {
    if(argc==3 && std::string_view(argv[1])=="--probe"){probe(argv[2]);return 0;}
    require(argc==2,"workspace root required");const std::filesystem::path root=argv[1];
    guards(particle(root/"assets/disc/maps/lockout.map"));
    guards(particle(root/"assets/map-pack/maps/warlock.map"));
    guards(particle(root/"assets/map-pack/maps/backwash.map"));
    unsigned maps=0,weather=0,views=0;
    for(const auto &name:disc)map_check(root/"assets/disc/maps"/(name+".map"),maps,weather,views);
    for(const auto &name:pack)map_check(root/"assets/map-pack/maps"/(name+".map"),maps,weather,views);
    require(maps==21 && weather==3,"weather ownership map inventory differs");
    std::cout<<maps<<" Xbox maps, "<<weather<<" weather owners, "<<views
        <<" actual portal-clipped local views, publication/lifetimes and explicit shared-stream initialization passed\n";
    // Vista-only assets need the same live entry path, even with no weather.
    TemporaryVista vista{std::filesystem::temp_directory_path()/
        ("halo2-weather-vista-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    std::filesystem::create_directories(vista.path);
    std::filesystem::create_directory_symlink(root/"Halo 2 Vista installer/maps",vista.path/"maps");
    std::ofstream(vista.path/"asset-donors.tsv")<<"H2_VISTA_DONORS_V1\nxbox\t"<<(root/"assets/disc").string()<<'\n';
    for(const auto *name:{"needle","street_sweeper"}) {
        map_check(vista.path/"maps"/(std::string(name)+".map"),maps,weather,views);
    }
    require(maps==23 && weather==3,"supplied weather ownership map inventory differs");
    std::cout<<maps<<" supplied maps and "<<views<<" portal-clipped local views accepted by native weather ownership\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
