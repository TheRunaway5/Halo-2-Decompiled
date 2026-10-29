include_guard(GLOBAL)

# Packet layouts can differ between active, source-qualified cohorts even
# while their feature version is equal. Keep those peers out of one match.
set(multiplayer_wire_inputs
    src/network/multiplayer_network.cpp
    src/runtime/source_shadow_transport.cpp
    include/halo2/source_shadow_transport.hpp
    include/halo2/online_packet_role.hpp
    src/network/snapshot_stream.cpp
    src/platform/native_request_protocol.cpp
    include/halo2/multiplayer_datagrams.hpp
    include/halo2/snapshot_stream.hpp
    include/halo2/native_request_protocol.hpp
    include/halo2/system_link_state.hpp
    include/halo2/multiplayer.hpp
    include/halo2/cache.hpp
    include/halo2/multiplayer_authority.hpp
    include/halo2/native_first_person_registry_tu15.hpp
    include/halo2/source_first_person_context_tu15.hpp
    src/gameplay/animation/source_first_person_context_tu15.cpp
    src/gameplay/multiplayer/native_first_person_registry_tu15.cpp
    include/halo2/source_runtime_resources.hpp
    include/halo2/transform.hpp
    include/halo2/source_world_pose.hpp
    include/halo2/source_world_pose_wire.hpp
    src/rendering/models/source_world_pose_wire.cpp
    include/halo2/source_decal_frame_wire_tu15.hpp
    include/halo2/source_object_loop_emitter_wire_tu15.hpp
    include/halo2/native_grenade_wire.hpp
    include/halo2/source_gameplay_fade_tu15.hpp
    src/gameplay/multiplayer/source_gameplay_fade_tu15.cpp
    src/gameplay/multiplayer/source_gameplay_fade_journal_tu15.cpp
    src/runtime/source_authority_archive.hpp
    rust/online_protocol/src/native_requests.rs
    rust/online_protocol/src/netcode/stream.rs
    rust/online_protocol/src/netcode/generic.rs
    rust/online_protocol/src/netcode/pose.rs
    rust/online_protocol/src/netcode/range.rs
    rust/online_protocol/src/netcode/mod.rs
    rust/online_protocol_bridge/src/netcode.rs
    include/halo2/source_first_person_packet_tu15.hpp
    src/gameplay/animation/source_first_person_packet_tu15.cpp)
set(multiplayer_wire_identity "Halo2NativeWireSource-v1\n")
foreach(relative IN LISTS multiplayer_wire_inputs)
    set(path "${CMAKE_CURRENT_SOURCE_DIR}/${relative}")
    if(EXISTS "${path}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
        file(READ "${path}" contents)
        string(REPLACE "\r\n" "\n" contents "${contents}")
        string(SHA256 digest "${contents}")
    else()
        # A qualified cohort may exclude a concurrently developed feature.
        # Its absent serializer is part of that cohort's distinct identity.
        set(digest "absent")
    endif()
    string(APPEND multiplayer_wire_identity "${relative}:${digest}\n")
endforeach()
string(SHA256 multiplayer_wire_sha256 "${multiplayer_wire_identity}")
string(SUBSTRING "${multiplayer_wire_sha256}" 0 16 multiplayer_wire_id)
set(multiplayer_wire_source "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-wire-schema.cpp")
file(CONFIGURE OUTPUT "${multiplayer_wire_source}" CONTENT
    "#include <cstdint>\nnamespace halo2 {std::uint64_t native_multiplayer_wire_schema(){return 0x${multiplayer_wire_id}ULL;}}\n"
    @ONLY)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/multiplayer-wire-schema.json" CONTENT
    "{\"schema\":\"Halo2NativeWireSource-v1\",\"id\":\"${multiplayer_wire_id}\",\"sha256\":\"${multiplayer_wire_sha256}\"}\n"
    @ONLY)
target_sources(halo2_systems PRIVATE "${multiplayer_wire_source}")
