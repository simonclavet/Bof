#include "stdafx.h"
#include "DescriptorSetsExample.h"


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
    VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}


void DescriptorSetsExample::run(
    HINSTANCE hInstance, 
    WNDPROC wndproc, 
    const std::vector<const char*>& args)
{
    m_programArgs = args;

    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w+", stdout);
    freopen_s(&stream, "CONOUT$", "w+", stderr);
    SetConsoleTitle(TEXT("debug console"));

    std::cout << "running..." << std::endl;


    // Check for a valid asset path
    struct stat info;
    if (stat(getAssetPath().c_str(), &info) != 0)
    {
        std::string msg = "Could not locate asset path in \"" + getAssetPath() + "\" !";
        MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
        exit(-1);
    }


    char* numConvPtr;

    // Parse command line arguments
    for (size_t i = 0; i < args.size(); i++)
    {
        if (args[i] == std::string("-vsync"))
        {
            m_settings.vsync = true;
        }
        if ((args[i] == std::string("-f")) || (args[i] == std::string("--fullscreen")))
        {
            m_settings.fullscreen = true;
        }
        if ((args[i] == std::string("-w")) || (args[i] == std::string("-width")))
        {
            uint32_t w = strtol(args[i + 1], &numConvPtr, 10);
            if (numConvPtr != args[i + 1]) { m_width = w; };
        }
        if ((args[i] == std::string("-h")) || (args[i] == std::string("-height")))
        {
            uint32_t h = strtol(args[i + 1], &numConvPtr, 10);
            if (numConvPtr != args[i + 1]) { m_height = h; };
        }
    }


    setupDPIAwareness();







    m_title = "Pipeline state objects";
    m_camera.type = Camera::CameraType::lookat;
    m_camera.setPerspective(60.0f, (float)m_width / (float)m_height, 0.1f, 512.0f);
    m_camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    m_camera.setTranslation(glm::vec3(0.0f, 0.0f, -5.0f));
    m_settings.overlay = true;






    VkResult err;

    {
        PROFILE(createInstance);


        auto appInfo = vk::ApplicationInfo{}
            .setPApplicationName(m_name.c_str())
            .setPEngineName(m_name.c_str())
            .setApiVersion(m_apiVersion);

        Vector<const char*> instanceExtensions =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };
        Vector<const char*> layerExtensions = {};

        if constexpr (m_enableValidationLayers)
        {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layerExtensions.push_back("VK_LAYER_KHRONOS_validation");
        }

        m_instance = checkVkResult(vk::createInstance(
            vk::InstanceCreateInfo{}
            .setPApplicationInfo(&appInfo)
            .setPEnabledExtensionNames(instanceExtensions)
            .setPEnabledLayerNames(layerExtensions)));

        if constexpr (m_enableValidationLayers)
        {
            // The report flags determine what type of messages for the layers will be displayed
            // For validating (debugging) an application the error and warning bits should suffice
            VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
            // Additional flags include performance info, loader and layer debug messages, etc.
            vks::debug::setupDebugging(m_instance, debugReportFlags, VK_NULL_HANDLE);
        }
    }


    {
        PROFILE(pickPhysicalDevice);
        uint32_t gpuCount = 0;
        const Vector<vk::PhysicalDevice> physicalDevices = checkVkResult(m_instance.enumeratePhysicalDevices());

        // GPU selection

        // Select physical device to be used for the Vulkan example
        // Defaults to the first device unless specified by command line
        uint32_t selectedDevice = 0;

        // GPU selection via command line argument
        for (size_t i = 0; i < m_programArgs.size(); i++)
        {
            // Select GPU
            if ((m_programArgs[i] == std::string("-g")) || (m_programArgs[i] == std::string("-gpu")))
            {
                char* endptr;
                uint32_t index = strtol(m_programArgs[i + 1], &endptr, 10);
                if (endptr != m_programArgs[i + 1])
                {
                    if (index > gpuCount - 1)
                    {
                        std::cerr << "Selected device index " << index << " is out of range, reverting to device 0 (use -listgpus to show available Vulkan devices)" << "\n";
                    }
                    else
                    {
                        std::cout << "Selected Vulkan device " << index << "\n";
                        selectedDevice = index;
                    }
                };
                break;
            }
            // List available GPUs
            if (m_programArgs[i] == std::string("-listgpus"))
            {
                for (size_t j = 0; j < physicalDevices.size(); j++)
                {
                    vk::PhysicalDeviceProperties deviceProperties = physicalDevices[j].getProperties();
                    std::cout << "Device [" << j << "] : " << deviceProperties.deviceName << std::endl;
                    std::cout << " Type: " << vk::to_string(deviceProperties.deviceType) << "\n";
                    std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "." << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "." << (deviceProperties.apiVersion & 0xfff) << "\n";
                }                
            }
        }

        m_physicalDevice = physicalDevices[selectedDevice];

        // Store properties (including limits), features and memory properties of the physical device (so that examples can check against them)
        m_deviceFeatures = m_physicalDevice.getFeatures();
        m_deviceMemoryProperties = m_physicalDevice.getMemoryProperties();
    }


    // Fill mode non solid is required for wireframe display
    if (m_deviceFeatures.fillModeNonSolid)
    {
        m_enabledFeatures.fillModeNonSolid = VK_TRUE;
        // Wide lines must be present for line width > 1.0f
        if (m_deviceFeatures.wideLines)
        {
            m_enabledFeatures.wideLines = VK_TRUE;
        }
    };
    m_enabledFeatures.samplerAnisotropy = VK_TRUE;


    // Vulkan device creation
    // This is handled by a separate class that gets a logical device representation
    // and encapsulates functions related to a device
    m_vulkanDevice = new vks::VulkanDevice(m_physicalDevice);

    VkResult res = m_vulkanDevice->createLogicalDevice(m_enabledFeatures, m_enabledDeviceExtensions, nullptr);
    CHECK_VKRESULT(res);

    m_device = m_vulkanDevice->logicalDevice;

    // Get a graphics queue from the device
    vkGetDeviceQueue(m_device, m_vulkanDevice->queueFamilyIndices.graphics, 0, &m_queue);

    // Find a suitable depth format
    VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(m_physicalDevice, &m_depthFormat);
    assert(validDepthFormat);

    m_swapChain.connect(m_instance, m_physicalDevice, m_device);

    // Create synchronization objects
    VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
    // Create a semaphore used to synchronize image presentation
    // Ensures that the image is displayed before we start submitting new commands to the queue
    VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.presentComplete));
    // Create a semaphore used to synchronize command submission
    // Ensures that the image is not presented until all commands have been submitted and executed
    VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_semaphores.renderComplete));

    // Set up submit info structure
    // Semaphores will stay the same during application lifetime
    // Command buffer submission info is set by each example
    m_submitInfo = vks::initializers::submitInfo();
    m_submitInfo.pWaitDstStageMask = &m_submitPipelineStages;
    m_submitInfo.waitSemaphoreCount = 1;
    m_submitInfo.pWaitSemaphores = &m_semaphores.presentComplete;
    m_submitInfo.signalSemaphoreCount = 1;
    m_submitInfo.pSignalSemaphores = &m_semaphores.renderComplete;






    // setup window

    m_windowInstance = hInstance;

    WNDCLASSEX wndClass;

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = wndproc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = m_windowInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = m_name.c_str();
    wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

    if (!RegisterClassEx(&wndClass))
    {
        std::cout << "Could not register window class!\n";
        fflush(stdout);
        exit(1);
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (m_settings.fullscreen)
    {
        if ((m_width != (uint32_t)screenWidth) && (m_height != (uint32_t)screenHeight))
        {
            DEVMODE dmScreenSettings;
            memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
            dmScreenSettings.dmSize = sizeof(dmScreenSettings);
            dmScreenSettings.dmPelsWidth = m_width;
            dmScreenSettings.dmPelsHeight = m_height;
            dmScreenSettings.dmBitsPerPel = 32;
            dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
            {
                if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                {
                    m_settings.fullscreen = false;
                }
                else
                {
                    return;
                }
            }
            screenWidth = m_width;
            screenHeight = m_height;
        }

    }

    DWORD dwExStyle;
    DWORD dwStyle;

    if (m_settings.fullscreen)
    {
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    }

    RECT windowRect;
    windowRect.left = 0L;
    windowRect.top = 0L;
    windowRect.right = m_settings.fullscreen ? (long)screenWidth : (long)m_width;
    windowRect.bottom = m_settings.fullscreen ? (long)screenHeight : (long)m_height;

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    std::string windowTitle = getWindowTitle();
    m_window = CreateWindowEx(0,
        m_name.c_str(),
        windowTitle.c_str(),
        dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0,
        0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,
        NULL,
        m_windowInstance,
        NULL);

    if (!m_settings.fullscreen)
    {
        // Center on screen
        uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
        uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
        SetWindowPos(m_window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }

    if (!m_window)
    {
        printf("Could not create window!\n");
        fflush(stdout);
        return;
    }

    ShowWindow(m_window, SW_SHOW);
    SetForegroundWindow(m_window);
    SetFocus(m_window);








    if (m_vulkanDevice->enableDebugMarkers)
    {
        vks::debugmarker::setup(m_device);
    }

    m_swapChain.initSurface(m_windowInstance, m_window);

    // create command pool
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = m_swapChain.queueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, &m_cmdPool));



    m_swapChain.create(&m_width, &m_height, m_settings.vsync);


    createCommandBuffers();

    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    m_waitFences.resize(m_drawCmdBuffers.size());
    for (auto& fence : m_waitFences)
    {
        VK_CHECK_RESULT(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &fence));
    }
    
    
    setupDepthStencil();

    // setup renderpass
    {
        std::array<VkAttachmentDescription, 2> attachments = {};
        // Color attachment
        attachments[0].format = m_swapChain.colorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        // Depth attachment
        attachments[1].format = m_depthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 1;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;
        subpassDescription.inputAttachmentCount = 0;
        subpassDescription.pInputAttachments = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments = nullptr;
        subpassDescription.pResolveAttachments = nullptr;

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        VK_CHECK_RESULT(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass));

    }

    
    
    // pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache));



    setupFrameBuffer();

    m_settings.overlay = m_settings.overlay;
    if (m_settings.overlay)
    {
        m_UIOverlay.device = m_vulkanDevice;
        m_UIOverlay.queue = m_queue;
        m_UIOverlay.shaders = {
            loadShader(getShadersPath() + "uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            loadShader(getShadersPath() + "uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
        };
        m_UIOverlay.prepareResources();
        m_UIOverlay.preparePipeline(m_pipelineCache, m_renderPass);
    }

    
    const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
    m_model.loadFromFile(getAssetPath() + "models/cube.gltf", m_vulkanDevice, m_queue, glTFLoadingFlags);
    m_cubes[0].m_texture.loadFromFile(getAssetPath() + "textures/crate01_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, m_vulkanDevice, m_queue);
    m_cubes[1].m_texture.loadFromFile(getAssetPath() + "textures/crate02_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, m_vulkanDevice, m_queue);


    {

        for (auto& cube : m_cubes)
        {
            VK_CHECK_RESULT(m_vulkanDevice->createBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &cube.m_uniformBuffer,
                sizeof(Cube::Matrices)
            ));

            VK_CHECK_RESULT(cube.m_uniformBuffer.map());
        }
    }


    // descriptor set layout
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
    {
        // Binding 0 : Vertex shader uniform buffer, to pass matrices
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT,
            0),
        // Binding 1 : Combined image sampler (used to pass per object texture information)
        vks::initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            1),
    };

    VkDescriptorSetLayoutCreateInfo descriptorLayout =
        vks::initializers::descriptorSetLayoutCreateInfo(
            setLayoutBindings.data(),
            setLayoutBindings.size());

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &descriptorLayout, nullptr, &m_descriptorSetLayout));




    // descriptor pool


    std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes{};

    // Uniform buffers : 1 for scene and 1 per object (scene and local matrices)
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = 1 + static_cast<uint32_t>(m_cubes.size());

    // Combined image samples : 1 per mesh texture
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(m_cubes.size());


    // Create the global descriptor pool
    VkDescriptorPoolCreateInfo descriptorPoolCI = {};
    descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
    descriptorPoolCI.pPoolSizes = descriptorPoolSizes.data();
    // Max. number of descriptor sets that can be allocated from this pool (one per object)
    descriptorPoolCI.maxSets = static_cast<uint32_t>(m_cubes.size());

    VK_CHECK_RESULT(vkCreateDescriptorPool(m_device, &descriptorPoolCI, nullptr, &m_descriptorPool));


    // descriptor sets

    for (auto& cube : m_cubes)
    {
        // Allocates an empty descriptor set without actual descriptors from the pool using the set layout
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = m_descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &m_descriptorSetLayout;
        VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device, &allocateInfo, &cube.m_descriptorSet));

        std::array<VkWriteDescriptorSet, 2> writeDescriptorSets{};

        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].dstSet = cube.m_descriptorSet;
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].pBufferInfo = &cube.m_uniformBuffer.descriptor;
        writeDescriptorSets[0].descriptorCount = 1;

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].dstSet = cube.m_descriptorSet;
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        // Images use a different descriptor structure, so we use pImageInfo instead of pBufferInfo
        writeDescriptorSets[1].pImageInfo = &cube.m_texture.descriptor;
        writeDescriptorSets[1].descriptorCount = 1;

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }





    // prepare pipelines
    {


        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
            vks::initializers::pipelineLayoutCreateInfo(
                &m_descriptorSetLayout,
                1);


        VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));



        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
        VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        //std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH, };
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

        VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(m_pipelineLayout, m_renderPass);
        pipelineCI.pInputAssemblyState = &inputAssemblyState;
        pipelineCI.pRasterizationState = &rasterizationState;
        pipelineCI.pColorBlendState = &colorBlendState;
        pipelineCI.pMultisampleState = &multisampleState;
        pipelineCI.pViewportState = &viewportState;
        pipelineCI.pDepthStencilState = &depthStencilState;
        pipelineCI.pDynamicState = &dynamicState;
        pipelineCI.stageCount = shaderStages.size();
        pipelineCI.pStages = shaderStages.data();
        pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState(
        {
            vkglTF::VertexComponent::Position, // (location = 0)
            vkglTF::VertexComponent::Normal, // (location = 1)
            vkglTF::VertexComponent::UV, // (location = 2)
            vkglTF::VertexComponent::Color // (location = 3)
        });

        // Textured pipeline
        // Phong shading pipeline
        shaderStages[0] = loadShader(getShadersPath() + "cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = loadShader(getShadersPath() + "cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineCI, nullptr, &m_pipeline));


    }





    buildCommandBuffers();
    
    
    
    m_prepared = true;





    // render loop



    m_destWidth = m_width;
    m_destHeight = m_height;
    m_lastTimestamp = std::chrono::high_resolution_clock::now();
    MSG msg;
    bool quitMessageReceived = false;
    while (!quitMessageReceived)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                quitMessageReceived = true;
                break;
            }
        }

        if (m_prepared &&
            !IsIconic(m_window))
        {
            auto tStart = std::chrono::high_resolution_clock::now();


            if (animate)
            {
                m_cubes[0].m_rotation.x += 20.5f * m_frameTimer;
                if (m_cubes[0].m_rotation.x > 360.0f)
                    m_cubes[0].m_rotation.x -= 360.0f;
                m_cubes[1].m_rotation.y += 2.0f * m_frameTimer;
                if (m_cubes[1].m_rotation.x > 360.0f)
                    m_cubes[1].m_rotation.x -= 360.0f;
            }



            m_cubes[0].m_matrices.model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
            m_cubes[1].m_matrices.model = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.5f, 0.0f));

            for (auto& cube : m_cubes)
            {
                cube.m_matrices.projection = m_camera.matrices.perspective;
                cube.m_matrices.view = m_camera.matrices.view;
                cube.m_matrices.model = glm::rotate(cube.m_matrices.model, glm::radians(cube.m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                cube.m_matrices.model = glm::rotate(cube.m_matrices.model, glm::radians(cube.m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                cube.m_matrices.model = glm::rotate(cube.m_matrices.model, glm::radians(cube.m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                cube.m_matrices.model = glm::scale(cube.m_matrices.model, glm::vec3(0.25f));

                memcpy(cube.m_uniformBuffer.mapped, &cube.m_matrices, sizeof(cube.m_matrices));
            }

            //m_uboVS.m_projection = m_camera.matrices.perspective;
            //m_uboVS.m_modelView = m_camera.matrices.view;
            //memcpy(m_uniformBuffer.mapped, &m_uboVS, sizeof(m_uboVS));


            prepareFrame();

            m_submitInfo.commandBufferCount = 1;
            m_submitInfo.pCommandBuffers = &m_drawCmdBuffers[m_currentBuffer];
            VK_CHECK_RESULT(vkQueueSubmit(m_queue, 1, &m_submitInfo, VK_NULL_HANDLE));

            submitFrame();









            m_frameCounter++;
            auto tEnd = std::chrono::high_resolution_clock::now();
            auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
            m_frameTimer = (float)tDiff / 1000.0f;
            m_camera.update(m_frameTimer);

            // Convert to clamped timer value
            if (!m_paused)
            {
                m_timer += m_timerSpeed * m_frameTimer;
                if (m_timer > 1.0)
                {
                    m_timer -= 1.0f;
                }
            }
            float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - m_lastTimestamp).count());
            if (fpsTimer > 1000.0f)
            {
                m_lastFPS = static_cast<uint32_t>((float)m_frameCounter * (1000.0f / fpsTimer));
                // if (!settings.overlay)
                {
                    std::string windowTitle = getWindowTitle();
                    SetWindowText(m_window, windowTitle.c_str());
                }
                m_frameCounter = 0;
                m_lastTimestamp = tEnd;
            }
            // TODO: Cap UI overlay update rates
            updateOverlay();
        }
    }

    // Flush device to make sure all resources can be freed
    if (m_device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_device);
    }



    // destroy

    m_model.destroy();

    // Clean up used Vulkan resources
    // Note : Inherited destructor cleans up resources stored in base class
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    //if (m_deviceFeatures.fillModeNonSolid)
    //{
    //    vkDestroyPipeline(m_device, m_wireframePipeline, nullptr);
    //}
    //vkDestroyPipeline(m_device, m_toonPipeline, nullptr);

    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    for (auto& cube : m_cubes)
    {
        cube.m_uniformBuffer.destroy();
        cube.m_texture.destroy();
    }


    // Clean up Vulkan resources
    m_swapChain.cleanup();
    if (m_descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }
    destroyCommandBuffers();
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    for (uint32_t i = 0; i < m_frameBuffers.size(); i++)
    {
        vkDestroyFramebuffer(m_device, m_frameBuffers[i], nullptr);
    }

    for (auto& shaderModule : m_shaderModules)
    {
        vkDestroyShaderModule(m_device, shaderModule, nullptr);
    }
    vkDestroyImageView(m_device, m_depthStencil.view, nullptr);
    vkDestroyImage(m_device, m_depthStencil.image, nullptr);
    vkFreeMemory(m_device, m_depthStencil.mem, nullptr);

    vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);

    vkDestroyCommandPool(m_device, m_cmdPool, nullptr);

    vkDestroySemaphore(m_device, m_semaphores.presentComplete, nullptr);
    vkDestroySemaphore(m_device, m_semaphores.renderComplete, nullptr);
    for (auto& fence : m_waitFences)
    {
        vkDestroyFence(m_device, fence, nullptr);
    }

    if (m_settings.overlay)
    {
        m_UIOverlay.freeResources();
    }

    delete m_vulkanDevice;

    if (m_enableValidationLayers)
    {
        vks::debug::freeDebugCallback(m_instance);
    }

    vkDestroyInstance(m_instance, nullptr);



}













std::string DescriptorSetsExample::getWindowTitle()
{
    std::string windowTitle;
    windowTitle = m_title;
    if (!m_settings.overlay) {
        windowTitle += " - " + std::to_string(m_frameCounter) + " fps";
    }
    return windowTitle;
}

void DescriptorSetsExample::createCommandBuffers()
{
    // Create one command buffer for each swap chain image and reuse for rendering
    m_drawCmdBuffers.resize(m_swapChain.imageCount);

    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        vks::initializers::commandBufferAllocateInfo(
            m_cmdPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            static_cast<uint32_t>(m_drawCmdBuffers.size()));

    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, m_drawCmdBuffers.data()));
}

void DescriptorSetsExample::destroyCommandBuffers()
{
    vkFreeCommandBuffers(m_device, m_cmdPool, static_cast<uint32_t>(m_drawCmdBuffers.size()), m_drawCmdBuffers.data());
}

std::string DescriptorSetsExample::getShadersPath() const
{
    return "Data/shaders/";
}



VkPipelineShaderStageCreateInfo DescriptorSetsExample::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
    shaderStage.module = vks::tools::loadShader(fileName.c_str(), m_device);
    shaderStage.pName = "main";
    assert(shaderStage.module != VK_NULL_HANDLE);
    m_shaderModules.push_back(shaderStage.module);
    return shaderStage;
}


void DescriptorSetsExample::updateOverlay()
{
    if (!m_settings.overlay)
        return;

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)m_width, (float)m_height);
    io.DeltaTime = m_frameTimer;

    io.MousePos = ImVec2(m_mousePos.x, m_mousePos.y);
    io.MouseDown[0] = m_mouseButtons.left;
    io.MouseDown[1] = m_mouseButtons.right;

    ImGui::NewFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted(m_title.c_str());
    ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / m_lastFPS), m_lastFPS);

    ImGui::PushItemWidth(110.0f * m_UIOverlay.scale);
    OnUpdateUIOverlay(&m_UIOverlay);
    ImGui::PopItemWidth();

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::Render();

    if (m_UIOverlay.update() || m_UIOverlay.updated) {
        buildCommandBuffers();
        m_UIOverlay.updated = false;
    }

}

void DescriptorSetsExample::drawUI(const VkCommandBuffer commandBuffer)
{
    if (m_settings.overlay) {
        const VkViewport viewport = vks::initializers::viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
        const VkRect2D scissor = vks::initializers::rect2D(m_width, m_height, 0, 0);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        m_UIOverlay.draw(commandBuffer);
    }
}

void DescriptorSetsExample::prepareFrame()
{
    // Acquire the next image from the swap chain
    VkResult result = m_swapChain.acquireNextImage(m_semaphores.presentComplete, &m_currentBuffer);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        windowResize();
    }
    else {
        VK_CHECK_RESULT(result);
    }
}

void DescriptorSetsExample::submitFrame()
{
    VkResult result = m_swapChain.queuePresent(m_queue, m_currentBuffer, m_semaphores.renderComplete);
    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            // Swap chain is no longer compatible with the surface and needs to be recreated
            windowResize();
            return;
        } else {
            VK_CHECK_RESULT(result);
        }
    }
    VK_CHECK_RESULT(vkQueueWaitIdle(m_queue));
}




void DescriptorSetsExample::setupDPIAwareness()
{
    typedef HRESULT *(__stdcall *SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

    HMODULE shCore = LoadLibraryA("Shcore.dll");
    if (shCore)
    {
        SetProcessDpiAwarenessFunc setProcessDpiAwareness =
            (SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

        if (setProcessDpiAwareness != nullptr)
        {
            setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        }

        FreeLibrary(shCore);
    }
}




void DescriptorSetsExample::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        m_prepared = false;
        DestroyWindow(hWnd);
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        ValidateRect(m_window, NULL);
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case KEY_P:
            m_paused = !m_paused;
            break;
        case KEY_F1:
            if (m_settings.overlay) {
                m_UIOverlay.visible = !m_UIOverlay.visible;
            }
            break;
        case KEY_ESCAPE:
            PostQuitMessage(0);
            break;
        }

        if (m_camera.type == Camera::firstperson)
        {
            switch (wParam)
            {
            case KEY_W:
                m_camera.keys.up = true;
                break;
            case KEY_S:
                m_camera.keys.down = true;
                break;
            case KEY_A:
                m_camera.keys.left = true;
                break;
            case KEY_D:
                m_camera.keys.right = true;
                break;
            }
        }

        keyPressed((uint32_t)wParam);
        break;
    case WM_KEYUP:
        if (m_camera.type == Camera::firstperson)
        {
            switch (wParam)
            {
            case KEY_W:
                m_camera.keys.up = false;
                break;
            case KEY_S:
                m_camera.keys.down = false;
                break;
            case KEY_A:
                m_camera.keys.left = false;
                break;
            case KEY_D:
                m_camera.keys.right = false;
                break;
            }
        }
        break;
    case WM_LBUTTONDOWN:
        m_mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
        m_mouseButtons.left = true;
        break;
    case WM_RBUTTONDOWN:
        m_mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
        m_mouseButtons.right = true;
        break;
    case WM_MBUTTONDOWN:
        m_mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
        m_mouseButtons.middle = true;
        break;
    case WM_LBUTTONUP:
        m_mouseButtons.left = false;
        break;
    case WM_RBUTTONUP:
        m_mouseButtons.right = false;
        break;
    case WM_MBUTTONUP:
        m_mouseButtons.middle = false;
        break;
    case WM_MOUSEWHEEL:
    {
        short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        m_camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f));
        break;
    }
    case WM_MOUSEMOVE:
    {
        handleMouseMove(LOWORD(lParam), HIWORD(lParam));
        break;
    }
    case WM_SIZE:
        if ((m_prepared) && (wParam != SIZE_MINIMIZED))
        {
            if ((m_resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
            {
                m_destWidth = LOWORD(lParam);
                m_destHeight = HIWORD(lParam);
                windowResize();
            }
        }
        break;
    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
        minMaxInfo->ptMinTrackSize.x = 64;
        minMaxInfo->ptMinTrackSize.y = 64;
        break;
    }
    case WM_ENTERSIZEMOVE:
        m_resizing = true;
        break;
    case WM_EXITSIZEMOVE:
        m_resizing = false;
        break;
    }
}



void DescriptorSetsExample::setupDepthStencil()
{
    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = m_depthFormat;
    imageCI.extent = { m_width, m_height, 1 };
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VK_CHECK_RESULT(vkCreateImage(m_device, &imageCI, nullptr, &m_depthStencil.image));
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(m_device, m_depthStencil.image, &memReqs);

    VkMemoryAllocateInfo memAlloc{};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = m_vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(m_device, &memAlloc, nullptr, &m_depthStencil.mem));
    VK_CHECK_RESULT(vkBindImageMemory(m_device, m_depthStencil.image, m_depthStencil.mem, 0));

    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = m_depthStencil.image;
    imageViewCI.format = m_depthFormat;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (m_depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VK_CHECK_RESULT(vkCreateImageView(m_device, &imageViewCI, nullptr, &m_depthStencil.view));
}

void DescriptorSetsExample::setupFrameBuffer()
{
    VkImageView attachments[2];

    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = m_depthStencil.view;

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = NULL;
    frameBufferCreateInfo.renderPass = m_renderPass;
    frameBufferCreateInfo.attachmentCount = 2;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = m_width;
    frameBufferCreateInfo.height = m_height;
    frameBufferCreateInfo.layers = 1;

    // Create frame buffers for every swap chain image
    m_frameBuffers.resize(m_swapChain.imageCount);
    for (uint32_t i = 0; i < m_frameBuffers.size(); i++)
    {
        attachments[0] = m_swapChain.buffers[i].view;
        VK_CHECK_RESULT(vkCreateFramebuffer(m_device, &frameBufferCreateInfo, nullptr, &m_frameBuffers[i]));
    }
}



void DescriptorSetsExample::buildCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VkClearValue clearValues[2];
    clearValues[0].color = m_defaultClearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = m_renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = m_width;
    renderPassBeginInfo.renderArea.extent.height = m_height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;

    for (int32_t i = 0; i < m_drawCmdBuffers.size(); ++i)
    {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = m_frameBuffers[i];

        VK_CHECK_RESULT(vkBeginCommandBuffer(m_drawCmdBuffers[i], &cmdBufInfo));

        vkCmdBeginRenderPass(m_drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        VkViewport viewport = vks::initializers::viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
        vkCmdSetViewport(m_drawCmdBuffers[i], 0, 1, &viewport);

        VkRect2D scissor = vks::initializers::rect2D(m_width, m_height, 0, 0);
        vkCmdSetScissor(m_drawCmdBuffers[i], 0, 1, &scissor);

        m_model.bindBuffers(m_drawCmdBuffers[i]);


        for (auto& cube : m_cubes)
        {
            vkCmdBindDescriptorSets(
                m_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                0, 1, &cube.m_descriptorSet, 0, nullptr);

            m_model.draw(m_drawCmdBuffers[i]);
        }

//        vkCmdBindDescriptorSets(m_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, NULL);


//        m_scene.draw(m_drawCmdBuffers[i]);

        //// Center : Toon
        //viewport.x = (float)m_width / 3.0;
        //vkCmdSetViewport(m_drawCmdBuffers[i], 0, 1, &viewport);
        //vkCmdBindPipeline(m_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_toonPipeline);
        //// Line width > 1.0f only if wide lines feature is supported
        //if (m_deviceFeatures.wideLines)
        //{
        //    vkCmdSetLineWidth(m_drawCmdBuffers[i], 2.0f);
        //}
        //m_scene.draw(m_drawCmdBuffers[i]);

        //if (m_deviceFeatures.fillModeNonSolid)
        //{
        //    // Right : Wireframe
        //    viewport.x = (float)m_width / 3.0 + (float)m_width / 3.0;
        //    vkCmdSetViewport(m_drawCmdBuffers[i], 0, 1, &viewport);
        //    vkCmdBindPipeline(m_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_wireframePipeline);
        //    m_scene.draw(m_drawCmdBuffers[i]);
        //}

        drawUI(m_drawCmdBuffers[i]);

        vkCmdEndRenderPass(m_drawCmdBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(m_drawCmdBuffers[i]));
    }
}


void DescriptorSetsExample::windowResize()
{
    if (!m_prepared)
    {
        return;
    }
    m_prepared = false;

    // Ensure all operations on the device have been finished before destroying resources
    vkDeviceWaitIdle(m_device);

    // Recreate swap chain
    m_width = m_destWidth;
    m_height = m_destHeight;
    m_swapChain.create(&m_width, &m_height, m_settings.vsync);

    // Recreate the frame buffers
    vkDestroyImageView(m_device, m_depthStencil.view, nullptr);
    vkDestroyImage(m_device, m_depthStencil.image, nullptr);
    vkFreeMemory(m_device, m_depthStencil.mem, nullptr);
    setupDepthStencil();
    for (uint32_t i = 0; i < m_frameBuffers.size(); i++) {
        vkDestroyFramebuffer(m_device, m_frameBuffers[i], nullptr);
    }
    setupFrameBuffer();

    if ((m_width > 0.0f) && (m_height > 0.0f)) {
        if (m_settings.overlay) {
            m_UIOverlay.resize(m_width, m_height);
        }
    }

    // Command buffers need to be recreated as they may store
    // references to the recreated frame buffer
    destroyCommandBuffers();

    createCommandBuffers();
    buildCommandBuffers();

    vkDeviceWaitIdle(m_device);

    if ((m_width > 0.0f) && (m_height > 0.0f)) 
    {
        m_camera.updateAspectRatio((float)(m_width) / (float)m_height);
    }


    m_prepared = true;
}

void DescriptorSetsExample::handleMouseMove(int32_t x, int32_t y)
{
    int32_t dx = (int32_t)m_mousePos.x - x;
    int32_t dy = (int32_t)m_mousePos.y - y;

    bool handled = false;

    if (m_settings.overlay) {
        ImGuiIO& io = ImGui::GetIO();
        handled = io.WantCaptureMouse;
    }
    mouseMoved((float)x, (float)y, handled);

    if (handled) {
        m_mousePos = glm::vec2((float)x, (float)y);
        return;
    }

    if (m_mouseButtons.left) {
        m_camera.rotate(glm::vec3(dy * m_camera.rotationSpeed, -dx * m_camera.rotationSpeed, 0.0f));
    }
    if (m_mouseButtons.right) {
        m_camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
    }
    if (m_mouseButtons.middle) {
        m_camera.translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));
    }
    m_mousePos = glm::vec2((float)x, (float)y);
}




#if 0

DescriptorSetsExample* vulkanExampleBase = nullptr;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (vulkanExampleBase != NULL)
    {
        vulkanExampleBase->handleMessages(hWnd, uMsg, wParam, lParam);
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    std::vector<const char*> args;
    for (int32_t i = 0; i < __argc; i++)
    {
        args.push_back(__argv[i]);
    };

    vulkanExampleBase = new DescriptorSetsExample();

    vulkanExampleBase->run(hInstance, WndProc, args);

    delete(vulkanExampleBase);
    return 0;
}

#endif