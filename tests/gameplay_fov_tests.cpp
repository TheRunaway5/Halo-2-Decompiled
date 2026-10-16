#include "halo2/gameplay_fov.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>

int main(int argc,char **argv)try {
    if(argc!=3)throw std::runtime_error("MCC world and first-person FOV fixtures required");
    std::ifstream input(argv[1]);
    if(!input)throw std::runtime_error("MCC FOV fixture unavailable");
    std::string line;unsigned cases{};
    while(std::getline(input,line)) {
        if(line.empty() || line.front()=='#')continue;
        float horizontal{},expected{};unsigned width{},height{};
        std::istringstream row(line);
        if(!(row>>horizontal>>width>>height>>expected))throw std::runtime_error("invalid MCC FOV fixture");
        const auto result=halo2::gameplay_camera_angles(horizontal,horizontal,float(width)/height);
        if(std::abs(result.projection_angle-expected)>2e-7f ||
            result.lighting_distance_scale!=.8949999809265137f)
            throw std::runtime_error("MCC camera conversion differs case "+std::to_string(cases));
        ++cases;
    }
    if(cases!=315)throw std::runtime_error("incomplete MCC FOV fixture");
    std::ifstream foreground_input(argv[2]);
    if(!foreground_input)throw std::runtime_error("MCC first-person fixture unavailable");
    unsigned foreground_cases{};
    while(std::getline(foreground_input,line)) {
        if(line.empty() || line.front()=='#')continue;
        float world{},expected{},near{},far{};unsigned width{},height{};
        std::istringstream row(line);
        if(!(row>>world>>width>>height>>expected>>near>>far) ||
            expected!=halo2::gameplay_first_person_projection_angle)
            throw std::runtime_error("MCC first-person lens override differs");
        ++foreground_cases;
    }
    if(foreground_cases!=20)throw std::runtime_error("incomplete MCC first-person fixture");
    const float wide=halo2::gameplay_field_of_view(120),normal=halo2::gameplay_field_of_view(78);
    const auto a=halo2::gameplay_camera_angles(wide,wide,16.f/9.f);
    const auto b=halo2::gameplay_camera_angles(wide,wide,32.f/9.f);
    if(a.projection_angle!=b.projection_angle ||
        !(a.projection_angle>halo2::gameplay_camera_angles(normal,normal,16.f/9.f).projection_angle))
        throw std::runtime_error("FOV expansion or ultrawide vertical framing differs");
    const auto zoom=halo2::gameplay_camera_angles(wide/4,wide,16.f/9.f);
    if(std::abs(zoom.lighting_distance_scale-.895f/4)>1e-7f)
        throw std::runtime_error("zoom lighting baseline did not follow the selected FOV");
    const float authored=70.f*.01745329238474369f;
    if(halo2::DisplaySettings{}.vehicle_field_of_view!=70)
        throw std::runtime_error("Vehicle camera default does not select TU1.5");
    for(float current:{.7f,1.2217304706573486f,1.57f}) {
        if(halo2::vehicle_observer_field_of_view(current,authored,70)!=current)
            throw std::runtime_error("Retail vehicle setting replaced the original observer lens");
        for(unsigned players=1;players<=4;++players)for(unsigned view=0;view<players;++view)
            for(bool wide:{false,true}) {
                const auto layout=halo2::source_view_grid(players,view,wide);
                const auto actual=halo2::vehicle_camera_angles(current,authored,70,layout.grid,layout.span);
                const auto expected=halo2::source_camera_angles(current,1.3333333730697632f,
                    layout.grid,layout.span,authored);
                if(actual.projection_angle!=expected.projection_angle ||
                    actual.lighting_distance_scale!=expected.lighting_distance_scale)
                    throw std::runtime_error("Vehicle projection lost original split-screen factors");
            }
        const auto custom=halo2::vehicle_observer_field_of_view(current,authored,120);
        if(std::abs(std::tan(double(custom)*.5)/std::tan(double(wide)*.5)-
            std::tan(double(current)*.5)/std::tan(double(authored)*.5))>3e-7)
            throw std::runtime_error("Vehicle FOV override discarded the authored lens transition");
    }
    for(float magnification:{1.f,2.f,4.f,10.f}) {
        const float original=float(2*std::atan(std::tan(double(authored)*.5)/magnification));
        const float adjusted=halo2::gameplay_observer_field_of_view(original,authored,120);
        const float expected=float(2*std::atan(std::tan(double(wide)*.5)/magnification));
        if(std::abs(adjusted-expected)>2e-7f)
            throw std::runtime_error("selected FOV changed scope magnification");
    }
    for(float aspect:{0.f,-1.f,std::numeric_limits<float>::infinity()}) {
        bool rejected{};try{halo2::gameplay_camera_angles(wide,wide,aspect);}catch(const halo2::FormatError &){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid FOV aspect was accepted");
    }
    std::cout<<cases<<" world and "<<foreground_cases<<" first-person MCC cases; expansion/zoom checks passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
