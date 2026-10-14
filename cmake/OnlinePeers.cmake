# Pinned native ICE/DTLS/SCTP stack. Only unordered, zero-retransmit UDP data
# channels are used. Media and WebSocket implementations are not bundled.
include(FetchContent)
function(halo2_load_peer_stack)
set(BUILD_SHARED_LIBS OFF)
set(NO_MEDIA ON)
set(NO_WEBSOCKET ON)
set(NO_EXAMPLES ON)
set(NO_TESTS ON)
set(BUILD_SHARED_DEPS_LIBS OFF)
FetchContent_Declare(halo2_datachannel
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
    GIT_TAG 6b1e2e620f1e37f0eafeee702eaea0043cb305fd
    GIT_SUBMODULES deps/libjuice deps/plog deps/usrsctp
    GIT_SHALLOW FALSE
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(halo2_datachannel)
endfunction()
halo2_load_peer_stack()
add_library(halo2_online_peers src/online_peer_transport.cpp)
target_include_directories(halo2_online_peers PUBLIC include)
target_link_libraries(halo2_online_peers PRIVATE LibDataChannel::LibDataChannelStatic Threads::Threads)
target_compile_options(halo2_online_peers PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
add_executable(online_peer_transport_tests tests/online_peer_transport_tests.cpp)
target_link_libraries(online_peer_transport_tests PRIVATE halo2_online_peers)
add_test(NAME online_peer_direct_udp COMMAND online_peer_transport_tests)
if(UNIX)
    add_executable(online_peer_wan_probe tools/online_peer_wan_probe.cpp)
    target_link_libraries(online_peer_wan_probe PRIVATE halo2_online_peers nlohmann_json::nlohmann_json)
endif()
