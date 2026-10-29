include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/CampaignNativeAiPerceptionGroupInitialize.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/NativeTu15DataArrayIteration.cmake")
# Real274800 and its registered invalidation callbacks already belong to
# halo2_systems. Keep the composed pressure owner in that same native library.
target_sources(halo2_systems PRIVATE
    src/campaign/ai/native_campaign_ai_perception_group_pressure_tu15.cpp)
target_link_libraries(halo2_systems PRIVATE halo2_native_tu15_data_array_delete)
