﻿/*
*--------------------------------------------------------------------------
* Copyright (c) 2015-2016 The Khronos Group Inc.
* Copyright (c) 2015-2016 Valve Corporation
* Copyright (c) 2015-2016 LunarG, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Rene Lindsay <rene@lunarg.com>
*
*--------------------------------------------------------------------------
*/

#include "CInstance.h"

//----------------------------Layers------------------------------
CLayers::CLayers(){
    VkResult result;
    do{
        uint count=0;
        result = vkEnumerateInstanceLayerProperties(&count, NULL);
        if(result==0 && count>0){
          itemList.resize(count);
          result=vkEnumerateInstanceLayerProperties(&count, itemList.data());
        }
    }while (result==VK_INCOMPLETE);
    VKERRCHECK(result);
}
//----------------------------------------------------------------

//--------------------------Extensions----------------------------
CExtensions::CExtensions(const char* layerName){
    VkResult result;
    do{
        uint count=0;
        result = vkEnumerateInstanceExtensionProperties(layerName, &count, NULL);             //Get list size
        if(result==0 && count>0){
          itemList.resize(count);                                                             //Resize buffer
          result=vkEnumerateInstanceExtensionProperties(layerName, &count, itemList.data());  //Fetch list
        }
    }while (result==VK_INCOMPLETE);                                                           //If list is incomplete, try again.
    VKERRCHECK(result);                                                                       //report errors
}
//----------------------------------------------------------------

//---------------------------CInstance----------------------------
CInstance::CInstance(const char* appName, const char* engineName){
    CLayers layers;
#ifndef NDEBUG  //In Debug mode, add standard validation layers
    layers.Pick({"VK_LAYER_GOOGLE_threading",
                 "VK_LAYER_LUNARG_parameter_validation",
                 "VK_LAYER_LUNARG_object_tracker",
                 "VK_LAYER_LUNARG_image",
                 "VK_LAYER_LUNARG_core_validation",
                 "VK_LAYER_LUNARG_swapchain",
                 "VK_LAYER_GOOGLE_unique_objects"});
    layers.Print();
#endif
    CExtensions extensions;
    if(extensions.Pick(VK_KHR_SURFACE_EXTENSION_NAME)){
#ifdef VK_USE_PLATFORM_WIN32_KHR
        extensions.Pick(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);    //Win32
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        extensions.Pick(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);  //Android
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        extensions.Pick(VK_KHR_XCB_SURFACE_EXTENSION_NAME);      //Linux XCB
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        extensions.Pick(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);     //Linux XLib
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        extensions.Pick(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);  //Linux Wayland
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        extensions.Pick(VK_KHR_MIR_SURFACE_EXTENSION_NAME);      //Linux Mir
#endif
    } else LOGE("Failed to load VK_KHR_Surface");
#ifndef NDEBUG
    //extensions.Pick("Fake_Extension"); //triggers a warning
    extensions.Pick(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);         //in Debug mode, Enable Validation
    extensions.Print();
#endif
    assert(extensions.PickCount()>=2);
    Create(layers, extensions, appName, engineName);
}

CInstance::CInstance(const CLayers& layers, const CExtensions& extensions, const char* appName, const char* engineName){
    Create(layers, extensions, appName, engineName);
}

void CInstance::Create(const CLayers& layers, const CExtensions& extensions, const char* appName, const char* engineName){
    // initialize the VkApplicationInfo structure
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = appName;
    app_info.applicationVersion = 1;
    app_info.pEngineName = engineName;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;

    // initialize the VkInstanceCreateInfo structure
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledExtensionCount   = extensions.PickCount();
    inst_info.ppEnabledExtensionNames = extensions.PickList();
    inst_info.enabledLayerCount       = layers.PickCount();
    inst_info.ppEnabledLayerNames     = layers.PickList();

    VKERRCHECK(vkCreateInstance(&inst_info, NULL, &instance));
    LOGI("Vulkan Instance created\n");

    if( extensions.IsPicked(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
        DebugReport.Init(instance);  //If VK_EXT_debug_report is loaded, initialize it.
}

void CInstance::Print(){ printf("->Instance %s created.\n",(!!instance)?"":"NOT"); }

CInstance::~CInstance(){
    DebugReport.Destroy();               //Must be done BEFORE vkDestroyInstance()
    vkDestroyInstance(instance, NULL);
    LOGI("Vulkan Instance destroyed\n");
}

//----------------------------------------------------------------
