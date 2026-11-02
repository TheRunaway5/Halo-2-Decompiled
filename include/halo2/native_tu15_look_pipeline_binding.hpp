#pragma once
#include "halo2/native_tu15_look_controller.hpp"
#include "halo2/native_tu15_camera_position.hpp"
#include "halo2/native_tu15_camera_query_binding.hpp"
#include "halo2/native_tu15_camera_query_input_observer.hpp"
#include "halo2/native_tu15_look_vector_angle.hpp"
#include "halo2/native_campaign_object_marker_request_tu15.hpp"
#include <stdexcept>
namespace halo2 {
struct NativeTu15LookPipelineScratch {
    NativeTu15LookControllerScratch controller;
    NativeTu15CameraPositionScratch camera;
    NativeTu15CameraQueryBindingScratch query;
};
struct NativeTu15LookPipelineFpFields {
    std::uint16_t& control_word;
    std::uint16_t& status;
    std::uint32_t& mxcsr;
    unsigned preserved_x87_depth;
};
struct NativeTu15LookPipelineActivity {
    std::uint64_t attempts{},completed{},refusals{},marker_calls{},query_refusals{},marker_observer_refusals{};
    NativeTu15LookControllerStats controller;
    NativeTu15CameraPositionStats camera;
    NativeTu15LookVectorAngleStats angles;
    NativeTu15CameraQueryBindingActivity scene_queries;
    std::uint64_t unsupported_fp_domains{};
};
// Native-only service composition. Only actual FP fields and caller-owned
// DATA enter the boundary. Unknown geometric queries refuse before any CPU
// owner is invoked; no service consumes original outcomes or register images.
inline void native_tu15_update_bound_look_controller(SourcePhysicsContactMemory&memory,
    std::uint32_t index,std::uint32_t yaw_bits,std::uint32_t pitch_bits,
    NativeTu15LookPipelineScratch scratch,NativeTu15LookPipelineFpFields fields,
    bool observer_read_only,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15LookPipelineActivity*activity=nullptr){
    NativeTu15LookPipelineActivity local;auto&out=activity?*activity:local;++out.attempts;
    NativeTu15PhysicsFrameFp fp{fields.control_word,fields.status,fields.mxcsr};
    struct Publish {
        NativeTu15LookPipelineFpFields fields;const NativeTu15PhysicsFrameFp&fp;
        ~Publish(){fields.control_word=fp.control_word;fields.status=fp.status;fields.mxcsr=fp.mxcsr;}
    }publish{fields,fp};
    try{
        if(!observer_read_only)throw std::runtime_error("Native look pipeline requires read-only store observation");
        if(fields.preserved_x87_depth)throw std::runtime_error("Native look pipeline requires its qualified empty-x87 Host domain");
        if(fields.control_word!=0x27fu){++out.unsupported_fp_domains;
            throw std::runtime_error("Native look pipeline requires its qualified default Host FP domain");}
        NativeTu15CameraPositionServices camera;
        camera.native_child=[&](std::uint32_t owner,bool entering){
            if(entering&&owner==0xbad60u&&observe){
                ++out.marker_observer_refusals;
                throw std::runtime_error("Native look pipeline requires BAD60 semantic store-observer owner");
            }
        };
        camera.segment=[&](const auto&call,auto&current)->bool{
            (void)native_tu15_observe_camera_query_input(memory,{0x16c620u,call.mask,
                call.origin,call.displacement,call.ignore_first,call.ignore_second,
                call.result,0u,current,fields.preserved_x87_depth});
            try{return native_tu15_query_bound_camera_scene(memory,
                {call.mask,call.origin,call.displacement,call.ignore_first,call.ignore_second,call.result},
                scratch.query,current,fields.preserved_x87_depth,observe,&out.scene_queries);
            }catch(...){++out.query_refusals;throw;}
        };
        NativeTu15LookControllerServices services;
        services.context.camera_position=[&](auto object,auto destination,auto&state){
            native_tu15_compute_unit_camera_position(memory,object,destination,scratch.camera,
                state,fields.preserved_x87_depth,camera,observe,&out.camera);
        };
        services.marker=[&](auto object,auto marker,auto output,auto&state){
            ++out.marker_calls;
            if(observe){
                ++out.marker_observer_refusals;
                throw std::runtime_error("Native look pipeline requires BAD60 semantic store-observer owner");
            }
            (void)native_campaign_object_marker_request_tu15(memory,object,false,marker,1,output,state);
        };
        services.vector_angle=[&](auto first,auto second,auto&state,unsigned depth){
            return native_tu15_look_vector_angle(memory,first,second,state,depth,&out.angles);
        };
        native_tu15_update_look_controller(memory,index,yaw_bits,pitch_bits,scratch.controller,
            fp,fields.preserved_x87_depth,services,observe,&out.controller);++out.completed;
    }catch(...){++out.refusals;throw;}
}
}
