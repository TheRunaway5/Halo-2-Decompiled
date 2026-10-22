#include "halo2/menu_world_source_storage.hpp"
#include "halo2/menu_world_triangle_storage.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
void check(VkResult result) {if(result!=VK_SUCCESS)throw FormatError("raw storage Vulkan failure: "+std::to_string(result));}
std::atomic<unsigned> validation_errors{};
VKAPI_ATTR VkBool32 VKAPI_CALL diagnostic(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT *data,void *) {
  if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {++validation_errors;std::cerr<<data->pMessage<<'\n';}
  return VK_FALSE;
}
std::vector<std::byte> read(const std::filesystem::path &path) {
  std::ifstream stream(path,std::ios::binary|std::ios::ate);require(bool(stream),"missing original storage fixture");
  const auto size=stream.tellg();require(size>=0 && size<64*1024*1024,"invalid storage fixture extent");
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));stream.seekg(0);
  require(bool(stream.read(reinterpret_cast<char *>(bytes.data()),size)),"short original storage fixture");return bytes;
}
// Real transfer readback only: this fixture does not claim vertex fetch,
// shader execution, draw selection or a framebuffer comparison.
struct Gpu {
  VkInstance instance{};VkDebugUtilsMessengerEXT messenger{};VkPhysicalDevice physical{};
  VkDevice device{};VkQueue queue{};VkCommandPool pool{};VkCommandBuffer command{};
  struct Buffer {VkBuffer buffer{};VkDeviceMemory memory{};void *mapped{};};
  std::vector<Buffer> buffers;
  bool initialize() {
    unsigned count{};check(vkEnumerateInstanceLayerProperties(&count,nullptr));
    std::vector<VkLayerProperties> layers(count);check(vkEnumerateInstanceLayerProperties(&count,layers.data()));
    if(std::none_of(layers.begin(),layers.end(),[](const auto &p){return std::string(p.layerName)=="VK_LAYER_KHRONOS_validation";}))return false;
    const char *layer="VK_LAYER_KHRONOS_validation",*extension=VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_1;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback=diagnostic;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;
    create.enabledLayerCount=1;create.ppEnabledLayerNames=&layer;create.enabledExtensionCount=1;create.ppEnabledExtensionNames=&extension;create.pNext=&debug;
    check(vkCreateInstance(&create,nullptr,&instance));
    check(reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"))(instance,&debug,nullptr,&messenger));
    check(vkEnumeratePhysicalDevices(instance,&count,nullptr));std::vector<VkPhysicalDevice> devices(count);
    check(vkEnumeratePhysicalDevices(instance,&count,devices.data()));unsigned family{};
    for(auto candidate:devices) {
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate,&count,families.data());
      for(unsigned i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_GRAPHICS_BIT){physical=candidate;family=i;break;}
      if(physical)break;
    }
    require(physical,"raw storage fixture has no graphics device");
    const float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
    VkDeviceCreateInfo d{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};d.queueCreateInfoCount=1;d.pQueueCreateInfos=&q;
    check(vkCreateDevice(physical,&d,nullptr,&device));vkGetDeviceQueue(device,family,0,&queue);
    VkCommandPoolCreateInfo p{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};p.queueFamilyIndex=family;p.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(device,&p,nullptr,&pool));
    VkCommandBufferAllocateInfo a{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};a.commandPool=pool;a.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;a.commandBufferCount=1;
    check(vkAllocateCommandBuffers(device,&a,&command));
    VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);
    std::cout<<"Raw source storage device: "<<properties.deviceName<<'\n';return true;
  }
  ~Gpu() {
    if(device) {
      vkDeviceWaitIdle(device);
      for(auto b:buffers){if(b.mapped)vkUnmapMemory(device,b.memory);vkDestroyBuffer(device,b.buffer,nullptr);vkFreeMemory(device,b.memory,nullptr);}
      if(pool)vkDestroyCommandPool(device,pool,nullptr);vkDestroyDevice(device,nullptr);
    }
    if(messenger)reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"))(instance,messenger,nullptr);
    if(instance)vkDestroyInstance(instance,nullptr);
  }
  std::vector<std::byte> readback(MenuWorldSourceBufferSlice slice) {
    require(slice.buffer && slice.size,"raw storage returned an empty GPU slice");
    Buffer b;VkBufferCreateInfo c{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};c.size=slice.size;c.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    check(vkCreateBuffer(device,&c,nullptr,&b.buffer));VkMemoryRequirements requirements;vkGetBufferMemoryRequirements(device,b.buffer,&requirements);
    VkPhysicalDeviceMemoryProperties properties;vkGetPhysicalDeviceMemoryProperties(physical,&properties);
    unsigned type=properties.memoryTypeCount;
    for(unsigned i=0;i<properties.memoryTypeCount;++i)if((requirements.memoryTypeBits&(1u<<i)) &&
        (properties.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=i;break;}
    require(type<properties.memoryTypeCount,"no coherent storage readback memory");
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};allocation.allocationSize=requirements.size;allocation.memoryTypeIndex=type;
    check(vkAllocateMemory(device,&allocation,nullptr,&b.memory));check(vkBindBufferMemory(device,b.buffer,b.memory,0));
    check(vkMapMemory(device,b.memory,0,slice.size,0,&b.mapped));buffers.push_back(b);
    check(vkResetCommandBuffer(command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};check(vkBeginCommandBuffer(command,&begin));
    VkMemoryBarrier upload{VK_STRUCTURE_TYPE_MEMORY_BARRIER};upload.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;upload.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&upload,0,nullptr,0,nullptr);
    VkBufferCopy copy{slice.offset,0,slice.size};vkCmdCopyBuffer(command,slice.buffer,b.buffer,1,&copy);
    VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&host,0,nullptr,0,nullptr);
    check(vkEndCommandBuffer(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;
    check(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));check(vkQueueWaitIdle(queue));
    std::vector<std::byte> result(slice.size);std::memcpy(result.data(),b.mapped,result.size());return result;
  }
};
MenuWorldSourceBinding fixture(const Cache &cache,const std::filesystem::path &directory,unsigned event,
    std::shared_ptr<const MenuWorldSourceSection> section,unsigned part,MenuWorldSourceTopology topology,
    MenuWorldSourceOwner owner,std::uint32_t shader) {
  const auto raw=read(directory/("event-"+std::to_string(event)+"-constants192.bin"));
  MenuWorldSourceConstants bank;require(raw.size()==sizeof(bank),"original storage bank is not c192");std::memcpy(bank.data(),raw.data(),raw.size());
  // This storage fixture deliberately supplies the retained original palette.
  // The separate source-binding test verifies the live palette producers.
  std::array<float,12> rows;std::memcpy(rows.data(),bank[50].data(),sizeof(rows));
  return menu_world_source_binding(cache,owner,menu_world_source_draw(section,part,topology,std::span(&rows,1)),shader,bank);
}
void compare(const MenuWorldSourceStoredDraw &draw,unsigned event,const std::filesystem::path &directory,
    Gpu &gpu,std::size_t &streams,std::size_t &bytes,std::size_t &triangles) {
  const auto prefix="event-"+std::to_string(event);
  require(draw.topology==VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST && draw.index_type==VK_INDEX_TYPE_UINT16,
      "storage changed source ordered list submission contract");
  const auto &inputs=draw.source.inputs();
  require(draw.vertex_bindings.size()==draw.vertex_buffers.size() && draw.vertex_buffers.size()==draw.vertex_offsets.size() &&
      draw.vertex_attributes.size()==inputs.size(),"GPU input layout is incomplete");
  for(const auto &input:inputs) {
    const auto attr=std::find_if(draw.vertex_attributes.begin(),draw.vertex_attributes.end(),[&](const auto &a){return a.location==input.vertex_register;});
    require(attr!=draw.vertex_attributes.end() && attr->binding<draw.vertex_bindings.size(),"source input register lost its Vulkan binding");
    const auto &binding=draw.vertex_bindings[attr->binding];const auto &resource=draw.resources.at(input.stream.resource);
    const auto format=input.stream.format==MenuWorldSourceFormat::float32x3?VK_FORMAT_R32G32B32_SFLOAT:
        input.stream.format==MenuWorldSourceFormat::float32x2?VK_FORMAT_R32G32_SFLOAT:
        input.stream.format==MenuWorldSourceFormat::snorm16x3?VK_FORMAT_R16G16B16_SNORM:
        input.stream.format==MenuWorldSourceFormat::snorm16x2?VK_FORMAT_R16G16_SNORM:VK_FORMAT_R32_SINT;
    require(binding.binding==attr->binding && binding.stride==input.stream.byte_stride && binding.inputRate==VK_VERTEX_INPUT_RATE_VERTEX && attr->format==format,
        "Vulkan fetch description differs from source format/stride");
    require(draw.vertex_buffers[attr->binding]==resource.buffer &&
        draw.vertex_offsets[attr->binding]+attr->offset==resource.offset+input.stream.byte_offset,
        "GPU vertex binding points outside original input resource");
    const auto expected=read(directory/(prefix+"-vertex-"+std::to_string(input.vertex_register)+".bin"));
    require(input.stream.byte_offset+expected.size()<=resource.size,"original stream exceeds uploaded resource");
    const auto actual=gpu.readback({resource.buffer,resource.offset+input.stream.byte_offset,expected.size()});
    require(actual==expected,"GPU transfer readback differs from original captured raw vertex stream");++streams;bytes+=actual.size();
  }
  const auto raw=read(directory/(prefix+"-indices.bin"));require(raw.size()%4==0,"captured index bank is not uint32");
  std::vector<std::uint32_t> indices(raw.size()/4);std::memcpy(indices.data(),raw.data(),raw.size());
  const bool strip=draw.source.draw().topology==MenuWorldSourceTopology::triangle_strip;
  const auto count=strip?indices.size()-2:indices.size()/3;std::vector<std::uint16_t> expected;
  for(std::size_t i=0;i<count;++i) {
    const std::array<std::size_t,3> order=strip?(i&1?std::array{i,i+2,i+1}:std::array{i,i+1,i+2}):std::array{i*3,i*3+1,i*3+2};
    for(const auto at:order){require(indices[at]<=0xffff,"captured index is outside original ushort");expected.push_back(std::uint16_t(indices[at]));}
  }
  const auto actual=gpu.readback(draw.indices);
  require(draw.index_count==expected.size() && actual.size()==expected.size()*2 &&
      std::memcmp(actual.data(),expected.data(),actual.size())==0,"uploaded source triangle order differs from original captured indices");triangles+=count;
}
void compare_range(const MenuWorldSourceStoredDraw &draw,unsigned event,unsigned original_first,
    MenuWorldSourceIndexRange range,const std::filesystem::path &directory,Gpu &gpu) {
  require(draw.source_index_range==range && draw.source.draw().index_range()==range,
          "stored index metadata lost the resolved absolute source range");
  const auto raw=read(directory/("event-"+std::to_string(event)+"-indices.bin"));const Bytes original(raw);
  require(raw.size()%4==0 && range.first>=original_first &&
          range.first-original_first<=raw.size()/4 && range.count<=raw.size()/4-(range.first-original_first),
          "range control escapes independently captured source indices");
  const auto first=range.first-original_first;
  const bool strip=draw.source.draw().topology==MenuWorldSourceTopology::triangle_strip;
  const auto count=strip?(range.count>2?range.count-2:0):range.count/3;
  std::vector<std::uint16_t> expected;
  for(std::size_t i=0;i<count;++i) {
    const std::array<std::size_t,3> order=strip?(i&1?std::array{i,i+2,i+1}:std::array{i,i+1,i+2}):std::array{i*3,i*3+1,i*3+2};
    for(const auto at:order) {
      const auto index=original.u32((first+at)*4);require(index<=0xffff,"captured range index escapes ushort");
      expected.push_back(std::uint16_t(index));
    }
  }
  require(draw.index_count==expected.size(),"stored partial range lost ordered source triangles");
  if(expected.empty()) {
    require(!draw.indices.buffer && !draw.indices.offset && !draw.indices.size,
            "empty range allocated an index buffer or retained a prior packet");return;
  }
  const auto actual=gpu.readback(draw.indices);
  require(actual.size()==expected.size()*2 && std::memcmp(actual.data(),expected.data(),actual.size())==0,
          "GPU partial index range differs from original captured order/parity/degenerates");
}
}
int main(int argc,char **argv) {try {
  require(argc==3,"raw GPU storage test needs mainmenu.map and Stage3 directory");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  Gpu gpu;if(!gpu.initialize())return 77;
  {
    bool rejected=false;
    try {MenuWorldSourceStorage invalid({gpu.device,VK_NULL_HANDLE});}
    catch(const FormatError &){rejected=true;}
    require(rejected,"storage admitted an absent physical device");
    const Cache cache(argv[1]);const std::filesystem::path directory=argv[2];
    const auto bsp52=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,52));
    const auto bsp0=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,0));
    const auto carrier=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,0xe1d70062u,2));
    const auto owner58=load_menu_world_source_bsp_owner(cache,0,58),owner0=load_menu_world_source_bsp_owner(cache,0,0);
    const MenuWorldSourceOwner carrier_owner{MenuWorldSourceOwnerKind::scenery_placement,0xe1780003u,3,{}};
    std::vector<MenuWorldSourceBinding> bindings;
    bindings.push_back(fixture(cache,directory,1335,bsp52,2,MenuWorldSourceTopology::triangle_list,
        {MenuWorldSourceOwnerKind::bsp_instance,owner58.bsp_datum,58,{}},0xe1cc0057u));
    bindings.push_back(fixture(cache,directory,1448,carrier,0,MenuWorldSourceTopology::triangle_strip,carrier_owner,0xe1dd0068u));
    bindings.push_back(fixture(cache,directory,1880,carrier,0,MenuWorldSourceTopology::triangle_strip,carrier_owner,0xe1ba0045u));
    bindings.push_back(fixture(cache,directory,2409,bsp0,0,MenuWorldSourceTopology::triangle_list,
        {MenuWorldSourceOwnerKind::bsp_instance,owner0.bsp_datum,0,{}},0xe1ba0045u));
    const auto owner59=load_menu_world_source_bsp_owner(cache,0,59);
    bindings.push_back(menu_world_source_binding(cache,{MenuWorldSourceOwnerKind::bsp_instance,owner59.bsp_datum,59,{}},
        menu_world_source_draw(bsp52,2,MenuWorldSourceTopology::triangle_list,std::span(&owner59.rows,1)),0xe1cc0057u,bindings[0].constants()));
    MenuWorldSourceStorage storage({gpu.device,gpu.physical});const auto draws=storage.store(bindings);
    require(draws.size()==5,"storage lost explicit source draw bindings");
    std::size_t streams{},bytes{},triangles{};const std::array events{1335u,1448u,1880u,2409u};
    for(unsigned i=0;i<4;++i)compare(draws[i],events[i],directory,gpu,streams,bytes,triangles);
    require(draws[0].resources[0].buffer==draws[4].resources[0].buffer && draws[0].indices.buffer==draws[4].indices.buffer &&
        draws[0].source.owner()!=draws[4].source.owner() && draws[0].source.constants()!=draws[4].source.constants(),
        "raw shared section conflated distinct owner banks or duplicated immutable buffers");
    require(draws[1].indices.buffer==draws[2].indices.buffer && draws[1].resources[0].buffer==draws[2].resources[0].buffer &&
        draws[1].source.vertex_shader()!=draws[2].source.vertex_shader(),"base and fog do not share the same source geometry");
    const auto retained=draws[0].resources[0];const auto expected=gpu.readback(retained);
    const auto whole_index=draws[0].indices;
    const auto ranged=[&](const MenuWorldSourceBinding &source,MenuWorldSourceIndexRange range) {
      const auto &draw=source.draw();
      return menu_world_source_binding(cache,source.owner(),menu_world_source_draw(draw.geometry,draw.part,
          draw.topology,draw.object_rows,range),source.vertex_shader(),source.constants());
    };
    // Unchanged4DAA0 list controls retain distinct noncontiguous ranges and
    // coalesce adjacent ones. This test consumes their exact emitted bounds;
    // it does not produce a visibility mask or choose admission.
    std::vector<MenuWorldSourceBinding> partial;
    partial.push_back(ranged(bindings[0],{702,54}));
    partial.push_back(ranged(bindings[0],{756,54})); // Same count, different first.
    partial.push_back(ranged(bindings[0],{702,120})); // Same first, different count.
    partial.push_back(ranged(bindings[0],{810,12}));
    partial.push_back(ranged(bindings[4],{702,54})); // Separate owner, same range.
    partial.push_back(ranged(bindings[0],{702,522})); // Explicit whole == default whole.
    partial.push_back(ranged(bindings[0],{702,0}));
    partial.push_back(ranged(bindings[0],{1224,0}));
    const auto strip_first=unsigned(carrier->parts().at(0).index_first);
    partial.push_back(ranged(bindings[1],{strip_first+1,14})); // Odd start; local parity.
    const auto ranges=storage.store(partial);
    require(ranges.size()==9,"storage lost submitted source ranges");
    for(unsigned i=0;i<8;++i) {
      require(ranges[i].resources[0].buffer==retained.buffer,
              "partial ranges duplicated unchanged section vertex resources");
      compare_range(ranges[i],1335,702,*partial[i].draw().range,directory,gpu);
    }
    compare_range(ranges[8],1448,strip_first,{strip_first+1,14},directory,gpu);
    for(unsigned i=0;i<4;++i) {
      require(ranges[i].indices.buffer!=whole_index.buffer,"partial range aliases whole-part indices");
      for(unsigned j=i+1;j<4;++j)
        require(ranges[i].indices.buffer!=ranges[j].indices.buffer,
                "different source first/count ranges alias one uploaded index buffer");
    }
    require(ranges[0].indices.buffer==ranges[4].indices.buffer && ranges[0].source.owner()!=ranges[4].source.owner() &&
            ranges[0].source.constants()!=ranges[4].source.constants(),
            "same partial range failed sharing or conflated independent owner banks");
    require(ranges[5].indices.buffer==whole_index.buffer && ranges[5].indices.offset==whole_index.offset &&
            ranges[5].indices.size==whole_index.size,
            "explicit whole range does not share the resolved default whole identity");
    const auto separately_loaded=std::make_shared<const MenuWorldSourceSection>(
        load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,52));
    const auto repeated=fixture(cache,directory,1335,separately_loaded,2,MenuWorldSourceTopology::triangle_list,
        {MenuWorldSourceOwnerKind::bsp_instance,owner58.bsp_datum,58,{}},0xe1cc0057u);
    const auto reloaded=storage.store(std::span(&repeated,1));
    require(reloaded.size()==1 && reloaded[0].resources[0].buffer==retained.buffer &&
        reloaded[0].resources[0].offset==retained.offset,"identical separately loaded source section was not shared");
    require(storage.store({}).empty(),"empty metadata submission retained previous draw tokens");
    require(gpu.readback(retained)==expected,"metadata refresh invalidated live immutable geometry buffer");
    MenuWorldSourceStorage triangle_storage({gpu.device,gpu.physical,true});
    const auto triangle_draws=triangle_storage.store(bindings);
    unsigned rejected_triangle_metadata=0;
    for(unsigned i=0;i<4;++i) {
      const auto &draw=triangle_draws[i];const auto wire=menu_world_triangle_storage(draw);
      require(wire.size()==64+draw.index_count,"triangle metadata lost source index count");
      for(unsigned primitive=0;primitive<draw.source.draw().triangle_count();++primitive) {
        const auto source=draw.source.draw().triangle(primitive);
        for(unsigned lane=0;lane<3;++lane)require(wire[64+primitive*3+lane]==source[lane],"triangle metadata changed original corner/parity");
      }
      const auto actual=gpu.readback(draw.raw_storage);
      for(unsigned resource=0;resource<draw.resources.size();++resource) {
        const auto &slice=draw.resources[resource];const auto &expected=draw.source.draw().geometry->resources()[resource].bytes;
        require(slice.offset<=actual.size()&&expected.size()<=actual.size()-slice.offset&&
          std::equal(expected.begin(),expected.end(),actual.begin()+slice.offset),"triangle raw SSBO changed original resource bytes");
      }
      const auto reject=[&](auto change){auto invalid=draw;change(invalid);bool rejected=false;try{menu_world_triangle_storage(invalid);}catch(const FormatError&){rejected=true;}require(rejected,"triangle metadata admitted invalid resource bounds");++rejected_triangle_metadata;};
      reject([](auto &d){d.raw_storage.buffer={};});
      reject([](auto &d){--d.raw_storage.size;});
      reject([](auto &d){d.index_count+=3;});
      reject([](auto &d){d.resources[0].buffer={};});
      reject([](auto &d){d.resources[0].offset=UINT32_MAX;});
      reject([](auto &d){--d.resources[0].size;});
    }
    require(rejected_triangle_metadata==24,"triangle metadata rejection coverage incomplete");
    require(streams==12 && bytes==165660 && triangles==9315,"incomplete original geometry storage comparison");
    std::cout<<"4 original draws: "<<streams<<" raw streams / "<<bytes<<" bytes; "<<triangles
        <<" ordered triangles; 9 explicit range readbacks/empties and cache identities; "
        <<"base/fog and distinct-owner sharing; retained buffer lifetime\n";
  }
  require(validation_errors==0,"raw storage emitted Vulkan validation errors");
  std::cout<<"Vulkan validation errors: 0\n";return 0;
} catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
