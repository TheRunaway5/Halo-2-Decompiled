// Research-only RenderDoc trigger. All Vulkan calls are forwarded unchanged;
// capture the second presented frame without altering the title renderer.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include "renderdoc_app.h"
static RENDERDOC_API_1_6_0 *rdoc;
static VkInstance captured_instance;
static unsigned presents;
VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo *i,
    const VkAllocationCallbacks *a,VkInstance *v) {
    void *lib=dlopen(getenv("HALO2_RENDERDOC_LIBRARY"),RTLD_NOW|RTLD_LOCAL);
    pRENDERDOC_GetAPI get=lib?dlsym(lib,"RENDERDOC_GetAPI"):0;
    if(get && get(eRENDERDOC_API_Version_1_6_0,(void**)&rdoc)) {
        rdoc->SetCaptureFilePathTemplate(getenv("HALO2_CAPTURE_PREFIX"));
        rdoc->MaskOverlayBits(0,0);
    }
    PFN_vkCreateInstance next=dlsym(RTLD_NEXT,"vkCreateInstance");
    VkResult result=next(i,a,v);
    if(result==VK_SUCCESS)captured_instance=*v;
    return result;
}
VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(VkQueue queue,const VkPresentInfoKHR *info) {
    PFN_vkQueuePresentKHR next=dlsym(RTLD_NEXT,"vkQueuePresentKHR");
    VkResult result=next(queue,info);
    if(rdoc && captured_instance) {
        void *device=RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(captured_instance);
        if(++presents==1)rdoc->StartFrameCapture(device,0);
        else if(presents==2)fprintf(stderr,"native research capture: completed %u\n",rdoc->EndFrameCapture(device,0));
    }
    return result;
}
