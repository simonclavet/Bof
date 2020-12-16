#pragma once

#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
//#include <fcntl.h>
//#include <io.h>
#include <ShellScalingAPI.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <numeric>
#include <ctime>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <numeric>
#include <array>

#include "vulkan/vulkan.h"

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
#include "benchmark.hpp"



#include "VulkanglTFModel.h"


#include "vulkanhelpers/VulkanHelpers.h"

#define VERTEX_BUFFER_BIND_ID 0
#define OBJECT_INSTANCES 50 * 3 * 50

struct Vertex
{
    float pos[3];
    float color[3];
};


class DynamicUniformBufferExample
{
public:

    struct
    {
        VkPipelineVertexInputStateCreateInfo inputState;
        Vector<VkVertexInputBindingDescription> bindingDescriptions;
        Vector<VkVertexInputAttributeDescription> attributeDescriptions;
    } m_vertices;

    vks::Buffer m_vertexBuffer;
    vks::Buffer m_indexBuffer;

    u32 m_indexCount;

    struct
    {
        vks::Buffer view;
        vks::Buffer dynamic;
    } m_uniformBuffers;

    struct
    {
        glm::mat4 projection;
        glm::mat4 view;
    } m_uboVS;

    glm::vec3 m_rotations[OBJECT_INSTANCES];
    glm::vec3 m_rotationSpeeds[OBJECT_INSTANCES];

    struct UboDataDynamic
    {
        glm::mat4* model = nullptr;
    } m_uboDataDynamic;


    size_t m_dynamicAlignment;



    struct UBOVS
    {
        glm::mat4 m_projection;
        glm::mat4 m_modelView;
        glm::vec4 m_lightPos = glm::vec4(0.0f, 2.0f, 1.0f, 0.0f);
    };

    //UBOVS m_uboVS;


    //vkglTF::Model m_scene;

    //vks::Buffer m_uniformBuffer;

    //VkPipeline m_phongPipeline;
    //VkPipeline m_wireframePipeline;
    //VkPipeline m_toonPipeline;
    VkPipeline m_pipeline;

    VkPipelineLayout m_pipelineLayout;
    VkDescriptorSet m_descriptorSet;
    VkDescriptorSetLayout m_descriptorSetLayout;



    //bool animate = true;

    //struct Cube
    //{
    //    struct Matrices
    //    {
    //        glm::mat4 projection;
    //        glm::mat4 view;
    //        glm::mat4 model;
    //    };

    //    Matrices m_matrices;

    //    VkDescriptorSet m_descriptorSet;
    //    vks::Texture2D m_texture;
    //    vks::Buffer m_uniformBuffer;
    //    glm::vec3 m_rotation;

    //};

    //std::array<Cube, 2> m_cubes;

    //vkglTF::Model m_model;





    // Frame counter to display fps
    uint32_t m_frameCounter = 0;
    uint32_t m_lastFPS = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimestamp;
    vk::Instance m_instance;
    vk::PhysicalDevice m_physicalDevice;
    vk::PhysicalDeviceFeatures m_deviceFeatures;
    VkPhysicalDeviceMemoryProperties m_deviceMemoryProperties;
    VkPhysicalDeviceFeatures m_enabledFeatures{};
    std::vector<const char*> m_enabledDeviceExtensions;
    std::vector<const char*> m_enabledInstanceExtensions;

    VkDevice m_device;
    VkQueue m_queue;
    VkFormat m_depthFormat;
    VkCommandPool m_cmdPool;
    VkPipelineStageFlags m_submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo m_submitInfo;
    std::vector<VkCommandBuffer> m_drawCmdBuffers;
    VkRenderPass m_renderPass;
    std::vector<VkFramebuffer> m_frameBuffers;
    uint32_t m_currentBuffer = 0;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::vector<VkShaderModule> m_shaderModules;
    VkPipelineCache m_pipelineCache;
    VulkanSwapChain m_swapChain;

    struct Semaphores
    {
        VkSemaphore presentComplete;
        VkSemaphore renderComplete;
    } m_semaphores;

    std::vector<VkFence> m_waitFences;

    

    bool m_prepared = false;
    uint32_t m_width = 1280;
    uint32_t m_height = 720;


    uint32_t m_destWidth;
    uint32_t m_destHeight;
    bool m_resizing = false;


    vks::UIOverlay m_UIOverlay;

    float m_frameTimer = 1.0f;


    vks::VulkanDevice* m_vulkanDevice;

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



    void updateUniformBuffers();
};
