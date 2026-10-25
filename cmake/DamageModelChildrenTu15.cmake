target_sources(halo2_systems PRIVATE src/gameplay/objects/source_damage_model_children_tu15.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_damage_model_children_tu15.cpp
        PROPERTIES COMPILE_OPTIONS "/fp:strict")
else()
    set_source_files_properties(src/gameplay/objects/source_damage_model_children_tu15.cpp
        PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
