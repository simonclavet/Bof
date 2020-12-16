#pragma once




#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
//#include <fcntl.h>
//#include <io.h>
#include <ShellScalingAPI.h>


#include "core/core.h"



//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <assert.h>1
//#include <vector>
//#include <array>
//#include <numeric>
//#include <ctime>
//#include <iostream>
//#include <chrono>
//#include <random>
//#include <algorithm>
//#include <sys/stat.h>

//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_ENABLE_EXPERIMENTAL
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/matrix_inverse.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <string>
//#include <numeric>
//#include <array>

//#include "vulkan/vulkan.h"

#include "keycodes.hpp"
#include "VulkanTools.h"
#include "VulkanDebug.h"
#include "VulkanUIOverlay.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"

#include "VulkanInitializers.hpp"
#include "camera.hpp"
//#include "benchmark.hpp"



//#include "VulkanglTFModel.h"


#include "vulkanhelpers/VulkanHelpers.h"




class Triangle
{
public:

    // Vertex layout used in this example
    struct Vertex
    {
        float position[3];
        float color[3];
    };

    // Vertex buffer and memory
    struct VertexBufferMemory
    {
        vk::DeviceMemory memory; // Handle to the device memory for this buffer
        vk::Buffer buffer;       // Handle to the Vulkan buffer object that the memory is bound to
    };

    VertexBufferMemory m_vertices;

    // Index buffer and memory
    struct IndexBufferMemory
    {
        vk::DeviceMemory memory;
        vk::Buffer buffer;
        u32 count;
    };
    
    IndexBufferMemory m_indices;

    // Uniform buffer block object
    struct UBOMemoryDescriptor
    {
        VkDeviceMemory memory;
        VkBuffer buffer;
        VkDescriptorBufferInfo descriptor;
    };
    
    UBOMemoryDescriptor m_uniformBufferVS;

    // For simplicity we use the same uniform block layout as in the shader:
    //
    //	layout(set = 0, binding = 0) uniform UBO
    //	{
    //		mat4 projectionMatrix;
    //		mat4 modelMatrix;
    //		mat4 viewMatrix;
    //	} ubo;
    //
    // This way we can just memcopy the ubo data to the ubo
    // Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
    struct UBO
    {
        glm::mat4 projectionMatrix;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
    };
    
    UBO m_uboVS;

    // The pipeline layout is used by a pipeline to access the descriptor sets
    // It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader resources
    // A pipeline layout can be shared among multiple pipelines as long as their interfaces match
    vk::PipelineLayout m_pipelineLayout;

    // Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
    // While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute) pipeline states upfront
    // So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few exceptions to this not discussed here)
    // Even though this adds a new dimension of planing ahead, it's a great opportunity for performance optimizations by the driver
    vk::Pipeline m_pipeline;

    // The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
    // Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
    vk::DescriptorSetLayout m_descriptorSetLayout;

    // The descriptor set stores the resources bound to the binding points in a shader
    // It connects the binding points of the different shaders with the buffers and images used for those bindings
    VkDescriptorSet m_descriptorSet;





    // Frame counter to display fps
    uint32_t m_frameCounter = 0;
    uint32_t m_lastFPS = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimestamp;
    vk::Instance m_instance;
    vk::PhysicalDevice m_physicalDevice;
    vk::PhysicalDeviceFeatures m_deviceFeatures;
    vk::PhysicalDeviceMemoryProperties m_deviceMemoryProperties;
    vk::PhysicalDeviceFeatures m_enabledFeatures{};
    //std::vector<const char*> m_enabledDeviceExtensions;
    std::vector<const char*> m_enabledInstanceExtensions;
    std::vector<vk::QueueFamilyProperties> m_queueFamilyProperties;

    struct
    {
        uint32_t graphics;
        uint32_t present;
        uint32_t compute;
        uint32_t transfer;
    } m_queueFamilyIndices;


    vk::Device m_device;
    vk::CommandPool m_cmdPool;
    vk::Queue m_graphicsQueue;
    vk::Format m_depthFormat;
    VkPipelineStageFlags m_submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo m_submitInfo;
    Vector<VkCommandBuffer> m_drawCmdBuffers;
    VkRenderPass m_renderPass;
    Vector<VkFramebuffer> m_frameBuffers;
    uint32_t m_currentBuffer = 0;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    Vector<VkShaderModule> m_shaderModules;
    vk::PipelineCache m_pipelineCache;
    VulkanSwapChain m_swapChain;

    // Synchronization primitives
    // Synchronization is an important concept of Vulkan that OpenGL mostly hid away. Getting this right is crucial to using Vulkan.
    
    // Semaphores
    // Used to coordinate operations within the graphics queue and ensure correct command ordering
    struct Semaphores
    {
        vk::Semaphore presentComplete;
        vk::Semaphore renderComplete;
    } m_semaphores;

    // Fences
    // Used to check the completion of queue operations (e.g. command buffer execution)
    Vector<vk::Fence> m_waitFences;

    

    bool m_prepared = false;
    uint32_t m_width = 1280;
    uint32_t m_height = 720;


    uint32_t m_destWidth;
    uint32_t m_destHeight;
    bool m_resizing = false;


    vks::UIOverlay m_UIOverlay;

    float m_frameTimer = 1.0f;


    //vks::VulkanDevice* m_vulkanDevice;

    /** @brief Example settings that can be changed e.g. by command line arguments */
    struct Settings
    {
        /** @brief Set to true if fullscreen mode has been requested via command line */
        bool fullscreen = false;
        /** @brief Set to true if v-sync will be forced for the swapchain */
        bool vsync = false;
        /** @brief Enable UI overlay */
        bool overlay = false;
    } m_settings;

#ifdef NO_VALIDATION_LAYERS
    static constexpr bool m_enableValidationLayers = false;
#else
    static constexpr bool m_enableValidationLayers = true;
#endif
    VkDebugUtilsMessengerEXT m_debugCallback = nullptr;


    VkClearColorValue m_defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };

    std::vector<const char*> m_programArgs;

    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float m_timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float m_timerSpeed = 0.25f;
    bool m_paused = false;

    Camera m_camera;
    glm::vec2 m_mousePos;

    std::string m_name = "dynamic uniform buffer";
    uint32_t m_apiVersion = VK_API_VERSION_1_0;

    struct
    {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } m_depthStencil;

    struct
    {
        glm::vec2 axisLeft = glm::vec2(0.0f);
        glm::vec2 axisRight = glm::vec2(0.0f);
    } m_gamePadState;

    struct
    {
        bool left = false;
        bool right = false;
        bool middle = false;
    } m_mouseButtons;

    HWND m_window;
    HINSTANCE m_windowInstance;
















    Triangle() {}


    void run(HINSTANCE hInstance, WNDPROC wndproc, const std::vector<const char*>& args);

    void destroy();





    void buildCommandBuffers();




    void OnUpdateUIOverlay(vks::UIOverlay* overlay)
    {
        if (!m_deviceFeatures.fillModeNonSolid)
        {
            if (overlay->header("Info"))
            {
                overlay->text("Non solid fill modes not supported!");
            }
        }
    }



    std::string getWindowTitle();

    void windowResize();
    void handleMouseMove(int32_t x, int32_t y);

    void updateOverlay();
    void createPipelineCache();
    void createCommandPool();

    void createCommandBuffers();
    void destroyCommandBuffers();



    // Returns the path to the root of the glsl or hlsl shader directory.
    std::string getShadersPath() const;



    void setupDPIAwareness();

    void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


    /** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
    void keyPressed(uint32_t) {};
    /** @brief (Virtual) Called after the mouse cursor moved and before internal events (like camera rotation) is handled */
    void mouseMoved(double x, double y, bool& handled) {};
    /** @brief (Virtual) Called when the window has been resized, can be used by the sample application to recreate resources */
    void windowResized();


    /** @brief (Virtual) Setup default depth and stencil views */
    void setupDepthStencil();
    /** @brief (Virtual) Setup default framebuffers for all requested swapchain images */
    void setupFrameBuffer();
    /** @brief (Virtual) Setup a default renderpass */
    void setupRenderPass();


    /** @brief Loads a SPIR-V shader file for the given shader stage */
    VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);

    /** @brief Entry point for the main render loop */
    void renderLoop();

    /** @brief Adds the drawing commands for the ImGui overlay to the given command buffer */
    void drawUI(const VkCommandBuffer commandBuffer);

    u32 getMemoryTypeIndex(u32 typeBits, vk::MemoryPropertyFlags properties);
    VkCommandBuffer getCommandBuffer(bool begin);
    VkShaderModule loadSPIRVShader(String filename);
    u32 getQueueFamilyIndex(vk::QueueFlagBits queueFlags);

    void updateUniformBuffers();
};
