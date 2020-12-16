#include "stdafx.h"
#include "Triangle.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
    VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}


void Triangle::run(
    HINSTANCE hInstance, 
    WNDPROC wndproc, 
    const std::vector<const char*>& args)
{
    m_programArgs = args;

    {
        PROFILE(createConsole);
        AllocConsole();
        AttachConsole(GetCurrentProcessId());
        FILE* stream;
        freopen_s(&stream, "CONOUT$", "w+", stdout);
        freopen_s(&stream, "CONOUT$", "w+", stderr);
        SetConsoleTitle(TEXT("debug console"));

        std::cout << "running..." << std::endl;
    }

    {
        PROFILE(parseCommandLineArguments);

        char* numConvPtr;
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
                u32 w = strtol(args[i + 1], &numConvPtr, 10);
                if (numConvPtr != args[i + 1]) { m_width = w; };
            }
            if ((args[i] == std::string("-h")) || (args[i] == std::string("-height")))
            {
                u32 h = strtol(args[i + 1], &numConvPtr, 10);
                if (numConvPtr != args[i + 1]) { m_height = h; };
            }
        }
    }


    {
        PROFILE(createWindow);

        setupDPIAwareness();

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
            if ((m_width != (u32)screenWidth) && (m_height != (u32)screenHeight))
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
            u32 x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
            u32 y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
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
    }



    // Check for a valid asset path
    struct stat info;
    if (stat(getAssetPath().c_str(), &info) != 0)
    {
        std::string msg = "Could not locate asset path in \"" + getAssetPath() + "\" !";
        MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
        BOF_FAIL(msg.c_str());
    }




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


        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;

        if constexpr (m_enableValidationLayers)
        {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layerExtensions.push_back("VK_LAYER_KHRONOS_validation");
            
            debugCreateInfo
                .setMessageSeverity(
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
                .setMessageType(
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
                .setPfnUserCallback(debugCallback);
        }


        m_instance = checkVkResult(vk::createInstance(
            vk::InstanceCreateInfo{}
            .setPApplicationInfo(&appInfo)
            .setPEnabledExtensionNames(instanceExtensions)
            .setPEnabledLayerNames(layerExtensions)
            .setPNext(&debugCreateInfo)));


        if constexpr (m_enableValidationLayers)
        {
            //vk::DebugUtilsMessengerCreateInfoEXT debugUtilsInfo{};
            //debugUtilsInfo.messageSeverity =
            //    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            //    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            //    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            //debugUtilsInfo.messageType =
            //    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            //    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            //    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
            //debugUtilsInfo.pfnUserCallback = debugCallback;

            if (VulkanHelpers::createDebugUtilsMessengerEXT(
                m_instance,
                reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo),
                nullptr, &m_debugCallback) != VK_SUCCESS)
            {
                BOF_FAIL("failed to set up debug callback");
            }
        }
    }



    {
        PROFILE(pickPhysicalDevice);
        const Vector<vk::PhysicalDevice> physicalDevices = checkVkResult(m_instance.enumeratePhysicalDevices());

        // GPU selection

        // Select physical device to be used for the Vulkan example
        // Defaults to the first device unless specified by command line
        u32 selectedDevice = 0;

        // GPU selection via command line argument
        for (size_t i = 0; i < m_programArgs.size(); i++)
        {
            // Select GPU
            if ((m_programArgs[i] == std::string("-g")) || (m_programArgs[i] == std::string("-gpu")))
            {
                char* endptr;
                u32 index = strtol(m_programArgs[i + 1], &endptr, 10);
                if (endptr != m_programArgs[i + 1])
                {
                    if (index > physicalDevices.size() - 1)
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

    // Get list of supported extensions
    //Vector<vk::ExtensionProperties> supportedExtensions = m_physicalDevice.enumerateDeviceExtensionProperties();
    
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




    // This is handled by a separate class that gets a logical device representation
    // and encapsulates functions related to a device
    //m_vulkanDevice = new vks::VulkanDevice(m_physicalDevice);

    //VkResult res = m_vulkanDevice->createLogicalDevice(m_enabledFeatures, m_enabledDeviceExtensions, nullptr);
    // Desired queues need to be requested upon logical device creation
    // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
    // requests different queue types




    // Vulkan device creation
    {
        m_queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

        Vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

        // Get queue family indices for the requested queue family types
        // Note that the indices may overlap depending on the implementation

        // just one queue per type of queue, then any number for priority works
        const Vector<float> defaultQueuePriorities{ 0.0f };

        // Graphics queue
        m_queueFamilyIndices.graphics = getQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
        
        queueCreateInfos.emplace_back(
            vk::DeviceQueueCreateInfo{}
            .setQueueFamilyIndex(m_queueFamilyIndices.graphics)
            .setQueuePriorities(defaultQueuePriorities));

        // Dedicated compute queue
        m_queueFamilyIndices.compute = getQueueFamilyIndex(vk::QueueFlagBits::eCompute);
        if (m_queueFamilyIndices.compute != m_queueFamilyIndices.graphics)
        {
            // If compute family index differs, we need an additional queue create info for the compute queue
            queueCreateInfos.emplace_back(
                vk::DeviceQueueCreateInfo{}
                .setQueueFamilyIndex(m_queueFamilyIndices.compute)
                .setQueuePriorities(defaultQueuePriorities));
        }

        // Dedicated transfer queue
        m_queueFamilyIndices.transfer = getQueueFamilyIndex(vk::QueueFlagBits::eTransfer);
        if ((m_queueFamilyIndices.transfer != m_queueFamilyIndices.graphics) && 
            (m_queueFamilyIndices.transfer != m_queueFamilyIndices.compute))
        {
            // If compute family index differs, we need an additional queue create info for the compute queue
            queueCreateInfos.emplace_back(
                vk::DeviceQueueCreateInfo{}
                .setQueueFamilyIndex(m_queueFamilyIndices.transfer)
                .setQueuePriorities(defaultQueuePriorities));
        }

        // Create the logical device representation
        Vector<const char*> deviceExtensions{};
        bool useSwapChain = true;
        if (useSwapChain)
        {
            // If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
            deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        m_device = checkVkResult(m_physicalDevice.createDevice(
            vk::DeviceCreateInfo{}
            .setQueueCreateInfos(queueCreateInfos)
            .setPEnabledFeatures(&m_enabledFeatures)
            .setPEnabledExtensionNames(deviceExtensions)));


        // Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
        //bool debugMarkerExtensionSupported =
        //    std::find(supportedExtensions.begin(), supportedExtensions.end(), VK_EXT_DEBUG_MARKER_EXTENSION_NAME) != supportedExtensions.end();
        //if (debugMarkerExtensionSupported)
        //{
        //    deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        //    enableDebugMarkers = true;
        //}
        //if (m_vulkanDevice->enableDebugMarkers)
        //{
        //    vks::debugmarker::setup(m_device);
        //}

    }

    m_graphicsQueue = m_device.getQueue(
        m_queueFamilyIndices.graphics, 
        /*queue index*/ 0);

    // create command pool
    m_cmdPool = checkVkResult(m_device.createCommandPool(
        vk::CommandPoolCreateInfo{}
        .setQueueFamilyIndex(m_queueFamilyIndices.graphics)
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    ));

    // Get a graphics queue from the device
    //vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphics, 0, &m_queue);


    // Find a suitable depth format

    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    Vector<vk::Format> depthFormats = {
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD32Sfloat,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD16Unorm,
    };
    m_depthFormat = vk::Format::eUndefined;
    for (const vk::Format& format : depthFormats)
    {
        const vk::FormatProperties formatProperties = m_physicalDevice.getFormatProperties(format);
        //VkFormatProperties formatProps;
        //vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment);// VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            m_depthFormat = format;
            break;
        }
    }
    BOF_ASSERT(m_depthFormat != vk::Format::eUndefined);


    m_swapChain.connect(m_instance, m_physicalDevice, m_device);

    m_swapChain.initSurface(m_windowInstance, m_window);

    m_swapChain.create(&m_width, &m_height, m_settings.vsync);


 
    {
        PROFILE(createSynchronizationObjects);

        // Create a semaphore used to synchronize image presentation
        // Ensures that the image is displayed before we start submitting new commands to the queue
        m_semaphores.presentComplete = checkVkResult(m_device.createSemaphore(vk::SemaphoreCreateInfo{}));
    
        // Create a semaphore used to synchronize command submission
        // Ensures that the image is not presented until all commands have been submitted and executed
        m_semaphores.renderComplete = checkVkResult(m_device.createSemaphore(vk::SemaphoreCreateInfo{}));


        // Wait fences to sync command buffer access
        auto fenceCreateInfo = vk::FenceCreateInfo{}
            .setFlags(vk::FenceCreateFlagBits::eSignaled);

        m_waitFences.resize(m_swapChain.imageCount);
        for (vk::Fence& fence : m_waitFences)
        {
            fence = checkVkResult(m_device.createFence(fenceCreateInfo));
        }
    }



    {
        PROFILE(setupRenderPass);

        Vector<vk::AttachmentDescription> attachments;

        // color attachment (0)
        attachments.emplace_back(vk::AttachmentDescription{}
            .setFormat((vk::Format)m_swapChain.colorFormat)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR));
            
        Vector<vk::AttachmentReference> colorAttachmentReferences;
        colorAttachmentReferences.emplace_back(vk::AttachmentReference{}
            .setAttachment(0)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal));

        // depth attachment (1)
        attachments.emplace_back(vk::AttachmentDescription{}
            .setFormat(m_depthFormat)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eClear)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal));

        vk::AttachmentReference depthAttachmentReference = vk::AttachmentReference{}
            .setAttachment(1)
            .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        // subpass
        Vector<vk::SubpassDescription> subpassDescriptions;

        subpassDescriptions.emplace_back(vk::SubpassDescription{}
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(colorAttachmentReferences)
            .setPDepthStencilAttachment(&depthAttachmentReference));

        // dependencies
        Vector<vk::SubpassDependency> subpassDependencies;

        subpassDependencies.emplace_back(vk::SubpassDependency{}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion));

        subpassDependencies.emplace_back(vk::SubpassDependency{}
            .setSrcSubpass(0)
            .setDstSubpass(VK_SUBPASS_EXTERNAL)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion));

        // create renderpass
        m_renderPass = checkVkResult(m_device.createRenderPass(
            vk::RenderPassCreateInfo{}
            .setAttachments(attachments)
            .setSubpasses(subpassDescriptions)
            .setDependencies(subpassDependencies)));
    }

    m_pipelineCache = checkVkResult(m_device.createPipelineCache(vk::PipelineCacheCreateInfo{}));


   
    createCommandBuffers();
    
    setupDepthStencil();


    setupFrameBuffer();



    //m_settings.overlay = m_settings.overlay;
    //if (m_settings.overlay)
    //{
    //    m_UIOverlay.device = m_vulkanDevice;
    //    m_UIOverlay.queue = m_queue;
    //    m_UIOverlay.shaders = {
    //        loadShader(getShadersPath() + "uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
    //        loadShader(getShadersPath() + "uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
    //    };
    //    m_UIOverlay.prepareResources();
    //    m_UIOverlay.preparePipeline(m_pipelineCache, m_renderPass);
    //}

    // end of base prepare


    {
        PROFILE(prepareVerticesAndIndices);

        // prepare vertices
        // Prepare vertex and index buffers for an indexed triangle
        // Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader

        // A note on memory management in Vulkan in general:
        // This is a very complex topic and while it's fine for an example application to small individual memory allocations that is not
        // what should be done a real-world application, where you should allocate large chunks of memory at once instead.

        // Setup vertices
        Vector<Vertex> vertexBuffer = 
        {
            { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            { { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            { {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };
        u32 vertexBufferSize = (u32)vertexBuffer.size() * sizeof(Vertex);

        // Setup indices
        Vector<u32> indexBuffer = { 0, 1, 2 };
        m_indices.count = (u32)indexBuffer.size();
        u32 indexBufferSize = m_indices.count * sizeof(u32);
        

        // Static data like vertex and index buffer should be stored on the device memory
        // for optimal (and fastest) access by the GPU
        //
        // To achieve this we use so-called "staging buffers" :
        // - Create a buffer that's visible to the host (and can be mapped)
        // - Copy the data to this buffer
        // - Create another buffer that's local on the device (VRAM) with the same size
        // - Copy the data from the host to the device using a command buffer
        // - Delete the host visible (staging) buffer
        // - Use the device local buffers for rendering


        struct StagingBuffer
        {
            vk::DeviceMemory memory;
            vk::Buffer buffer;
        };


        struct
        {
            StagingBuffer vertices;
            StagingBuffer indices;
        } stagingBuffers;


        // vertices
        {
            // create vertex staging buffer and cpu memory, copy vertices to it
            {
                // create vertex staging buffer
                stagingBuffers.vertices.buffer = checkVkResult(m_device.createBuffer(
                    vk::BufferCreateInfo{}
                    .setSize(vertexBufferSize)
                    .setUsage(vk::BufferUsageFlagBits::eTransferSrc) // Buffer is used as the copy source
                ));

                // find the memory size required, and all memory types allowed by this buffer
               // (size may end up bigger that requested size because of alignment)
                const vk::MemoryRequirements memoryRequirements =
                    m_device.getBufferMemoryRequirements(stagingBuffers.vertices.buffer);

                // Request a host visible memory type that can be used to copy our data do
                // Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer,
                const u32 memoryTypeIndex = getMemoryTypeIndex(
                    memoryRequirements.memoryTypeBits,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

                // allocate staging memory
                stagingBuffers.vertices.memory = checkVkResult(m_device.allocateMemory(
                    vk::MemoryAllocateInfo{}
                    .setAllocationSize(memoryRequirements.size)
                    .setMemoryTypeIndex(memoryTypeIndex)
                ));

                // map memory, since it is visible we can write to it directly, but still need do map it to some pointer
                void* data = checkVkResult(m_device.mapMemory(
                    stagingBuffers.vertices.memory,
                    /*offset*/0,
                    memoryRequirements.size,
                    vk::MemoryMapFlags{}));
                // copy vertices to buffer memory
                memcpy(data, vertexBuffer.data(), vertexBufferSize);
                // unmap memory, since we won't need to write again
                m_device.unmapMemory(stagingBuffers.vertices.memory);
                // bind the buffer to the memory
                m_device.bindBufferMemory(stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, /*offset*/0);
            }

            // gpu vertices
            {
                // Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
                m_vertices.buffer = checkVkResult(m_device.createBuffer(
                    vk::BufferCreateInfo{}
                    .setSize(vertexBufferSize)
                    .setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst)
                ));

                const vk::MemoryRequirements memoryRequirements =
                    m_device.getBufferMemoryRequirements(m_vertices.buffer);

                const u32 memoryTypeIndex = getMemoryTypeIndex(
                    memoryRequirements.memoryTypeBits,
                    vk::MemoryPropertyFlagBits::eDeviceLocal);

                // allocate gpu memory
                m_vertices.memory = checkVkResult(m_device.allocateMemory(
                    vk::MemoryAllocateInfo{}
                    .setAllocationSize(memoryRequirements.size)
                    .setMemoryTypeIndex(memoryTypeIndex)
                ));

                CHECK_VKRESULT(m_device.bindBufferMemory(m_vertices.buffer, m_vertices.memory, /*offset*/0));
            }
        }

        // indices
        {
            // create index staging buffer and cpu memory, copy indices to it
            {
                stagingBuffers.indices.buffer = checkVkResult(m_device.createBuffer(
                    vk::BufferCreateInfo{}
                    .setSize(vertexBufferSize)
                    .setUsage(vk::BufferUsageFlagBits::eTransferSrc) 
                ));
                const vk::MemoryRequirements memoryRequirements =
                    m_device.getBufferMemoryRequirements(stagingBuffers.indices.buffer);
                const u32 memoryTypeIndex = getMemoryTypeIndex(
                    memoryRequirements.memoryTypeBits,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                stagingBuffers.indices.memory = checkVkResult(m_device.allocateMemory(
                    vk::MemoryAllocateInfo{}
                    .setAllocationSize(memoryRequirements.size)
                    .setMemoryTypeIndex(memoryTypeIndex)
                ));
                void* data = checkVkResult(m_device.mapMemory(
                    stagingBuffers.indices.memory,
                    /*offset*/0,
                    memoryRequirements.size,
                    vk::MemoryMapFlags{}));
                memcpy(data, indexBuffer.data(), vertexBufferSize);
                m_device.unmapMemory(stagingBuffers.indices.memory);
                m_device.bindBufferMemory(stagingBuffers.indices.buffer, stagingBuffers.indices.memory, /*offset*/0);
            }

            // gpu indices
            {
                m_indices.buffer = checkVkResult(m_device.createBuffer(
                    vk::BufferCreateInfo{}
                    .setSize(vertexBufferSize)
                    .setUsage(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst)
                ));
                const vk::MemoryRequirements memoryRequirements =
                    m_device.getBufferMemoryRequirements(m_indices.buffer);
                const u32 memoryTypeIndex = getMemoryTypeIndex(
                    memoryRequirements.memoryTypeBits,
                    vk::MemoryPropertyFlagBits::eDeviceLocal);
                m_indices.memory = checkVkResult(m_device.allocateMemory(
                    vk::MemoryAllocateInfo{}
                    .setAllocationSize(memoryRequirements.size)
                    .setMemoryTypeIndex(memoryTypeIndex)
                ));
                CHECK_VKRESULT(m_device.bindBufferMemory(m_indices.buffer, m_indices.memory, /*offset*/0));
            }
        }

        // send command to copy from staging buffer to device buffer
        {
            const Vector<vk::CommandBuffer> copyCommands = checkVkResult(m_device.allocateCommandBuffers(
                vk::CommandBufferAllocateInfo{}
                .setCommandPool(m_cmdPool)
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandBufferCount(1)
            ));
            const vk::CommandBuffer copyCommand = copyCommands[0];


            copyCommand.begin(vk::CommandBufferBeginInfo{});

            {
                Vector<vk::BufferCopy> copyRegions =
                {
                    vk::BufferCopy{}.setSize(vertexBufferSize).setSrcOffset(0).setDstOffset(0)
                };
                copyCommand.copyBuffer(stagingBuffers.vertices.buffer, m_vertices.buffer, copyRegions);
            }
            {
                Vector<vk::BufferCopy> copyRegions =
                {
                    vk::BufferCopy{}.setSize(indexBufferSize).setSrcOffset(0).setDstOffset(0)
                };
                copyCommand.copyBuffer(stagingBuffers.indices.buffer, m_indices.buffer, copyRegions);
            }


            // End the command buffer 
            CHECK_VKRESULT(copyCommand.end());


            // many command buffers per submissions to the queue
            const Vector<vk::CommandBuffer> commandBuffers = { copyCommand };

            // and many submissions to the the queue per call to submit...
            const Vector<vk::SubmitInfo> submits =
            {
                vk::SubmitInfo{}.setCommandBuffers(commandBuffers)
            };

            // Create fence to ensure that the command buffer has finished executing
            const vk::Fence fence = checkVkResult(m_device.createFence(vk::FenceCreateInfo{}));
            
            // submit to queue, send also the fence so we can then wait for it
            CHECK_VKRESULT(m_graphicsQueue.submit(submits, fence));

            // Wait for the fence to signal that command buffer has finished executing
            const Vector<vk::Fence> fences = { fence };
            CHECK_VKRESULT(m_device.waitForFences(fences, /*waitAll*/VK_TRUE, DEFAULT_FENCE_TIMEOUT));

            m_device.destroyFence(fence);

            m_device.freeCommandBuffers(m_cmdPool, commandBuffers);
        }

        // Destroy staging buffers
        // Note: Staging buffer must not be deleted before the copies have been submitted and executed
        m_device.destroyBuffer(stagingBuffers.vertices.buffer);
        m_device.freeMemory(stagingBuffers.vertices.memory);
        m_device.destroyBuffer(stagingBuffers.indices.buffer);
        m_device.freeMemory(stagingBuffers.indices.memory);
    }



    {
        PROFILE(createUniformBuffers);
        // Prepare and initialize a uniform buffer block containing shader uniforms
        // Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks
        m_uniformBufferVS.buffer = checkVkResult(m_device.createBuffer(
            vk::BufferCreateInfo{}
            .setSize(sizeof(m_uboVS))
            .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
        ));
        const vk::MemoryRequirements memoryRequirements =
            m_device.getBufferMemoryRequirements(m_uniformBufferVS.buffer);
        
        // Get the memory type index that supports host visible memory access
        // Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
        // We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
        // Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
        const u32 memoryTypeIndex = getMemoryTypeIndex(
            memoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);

        m_uniformBufferVS.memory = checkVkResult(m_device.allocateMemory(
            vk::MemoryAllocateInfo{}
            .setAllocationSize(memoryRequirements.size)
            .setMemoryTypeIndex(memoryTypeIndex)
        ));
        CHECK_VKRESULT(m_device.bindBufferMemory(m_uniformBufferVS.buffer, m_uniformBufferVS.memory, /*offset*/0));

        // Store information in the uniform's descriptor that is used by the descriptor set
        m_uniformBufferVS.descriptor.buffer = m_uniformBufferVS.buffer;
        m_uniformBufferVS.descriptor.offset = 0;
        m_uniformBufferVS.descriptor.range = sizeof(m_uboVS);
    }


    {
        PROFILE(createDescriptorSetLayout);
        // Connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
        // So every shader binding should map to one descriptor set layout binding

        Vector<vk::DescriptorSetLayoutBinding> layoutBindings;

        // Binding 0: Uniform buffer (Vertex shader)
        layoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{}
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex)
            .setPImmutableSamplers(nullptr)
        );

        m_descriptorSetLayout = checkVkResult(m_device.createDescriptorSetLayout(
            vk::DescriptorSetLayoutCreateInfo{}
            .setBindings(layoutBindings)
        ));
    }

    {
        PROFILE(createPipelineLayout);

        // Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
        // In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused

        Vector<vk::DescriptorSetLayout> descriptorSetLayouts = { m_descriptorSetLayout };
        Vector<vk::PushConstantRange> pushConstantRanges = {};

        m_pipelineLayout = checkVkResult(m_device.createPipelineLayout(
            vk::PipelineLayoutCreateInfo{}
            .setSetLayouts(descriptorSetLayouts)
            .setPushConstantRanges(pushConstantRanges)
        ));
    }

    {
        PROFILE(createPipelines);

        // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
        // A pipeline is then stored and hashed on the GPU making pipeline changes very fast
        // Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

        vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};


        // Shaders
        Vector<vk::PipelineShaderStageCreateInfo> shaderStages;

        // Vertex shader (0)
        shaderStages.emplace_back(vk::PipelineShaderStageCreateInfo{}
            .setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(loadSPIRVShader(getShadersPath() + "triangle.vert.spv"))
            .setPName("main")
            .setPSpecializationInfo(nullptr)
        );

        // Fragment shader (1)
        shaderStages.emplace_back(vk::PipelineShaderStageCreateInfo{}
            .setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(loadSPIRVShader(getShadersPath() + "triangle.frag.spv"))
            .setPName("main")
            .setPSpecializationInfo(nullptr)
        );

        pipelineCreateInfo.setStages(shaderStages);

        // Vertex input descriptions
        // Specifies the vertex input parameters for a pipeline

        // Vertex input binding
        // This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
        Vector<vk::VertexInputBindingDescription> vertexInputBindings;
        vertexInputBindings.emplace_back(vk::VertexInputBindingDescription{}
            .setBinding(0)
            .setStride(sizeof(Vertex))
            .setInputRate(vk::VertexInputRate::eVertex)
        );

        // Input attribute bindings describe shader attribute locations and memory layouts
        Vector<vk::VertexInputAttributeDescription> vertexInputAttributes;

        // Attribute location 0: Position
        vertexInputAttributes.emplace_back(vk::VertexInputAttributeDescription{}
            .setBinding(0)
            .setLocation(0)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(offsetof(Vertex, position))
        );
        // Attribute location 1: Color
        vertexInputAttributes.emplace_back(vk::VertexInputAttributeDescription{}
            .setBinding(0)
            .setLocation(1)
            .setFormat(vk::Format::eR32G32B32Sfloat)
            .setOffset(offsetof(Vertex, color))
        );

        // Vertex input state used for pipeline creation
        const auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{}
            .setVertexBindingDescriptions(vertexInputBindings)
            .setVertexAttributeDescriptions(vertexInputAttributes);

        pipelineCreateInfo.setPVertexInputState(&vertexInputState);


        // Input assembly state describes how primitives are assembled
        // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
        const auto inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{}
            .setTopology(vk::PrimitiveTopology::eTriangleList)
            .setPrimitiveRestartEnable(VK_FALSE);

        pipelineCreateInfo.setPInputAssemblyState(&inputAssemblyStateCreateInfo);

        pipelineCreateInfo.setPTessellationState(nullptr);

        // Viewport state sets the number of viewports and scissor used in this pipeline
        // Note: This is actually overridden by the dynamic states (see below)
        const auto viewportState = vk::PipelineViewportStateCreateInfo{}
            .setViewportCount(1)
            .setScissorCount(1);
        pipelineCreateInfo.setPViewportState(&viewportState);

        const auto rasterizationState = vk::PipelineRasterizationStateCreateInfo{}
            .setDepthClampEnable(VK_FALSE)
            .setRasterizerDiscardEnable(VK_FALSE)
            .setPolygonMode(vk::PolygonMode::eFill)
            .setCullMode(vk::CullModeFlagBits::eNone)
            .setFrontFace(vk::FrontFace::eCounterClockwise)
            .setDepthBiasEnable(VK_FALSE)
            .setDepthBiasConstantFactor(0.0f)
            .setDepthBiasClamp(0.0f)
            .setLineWidth(1.0f);
        pipelineCreateInfo.setPRasterizationState(&rasterizationState);

        const auto multisampleState = vk::PipelineMultisampleStateCreateInfo{}
            .setRasterizationSamples(vk::SampleCountFlagBits::e1)
            .setSampleShadingEnable(VK_FALSE)
            .setMinSampleShading(0)
            .setPSampleMask(nullptr)
            .setAlphaToCoverageEnable(VK_FALSE)
            .setAlphaToOneEnable(VK_FALSE);
        pipelineCreateInfo.setPMultisampleState(&multisampleState);

        // Depth and stencil state containing depth and stencil compare and test operations
        // We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
        const auto stencilOpState = vk::StencilOpState{}
            .setFailOp(vk::StencilOp::eKeep)
            .setPassOp(vk::StencilOp::eKeep)
            .setCompareOp(vk::CompareOp::eAlways);
        const auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo{}
            .setDepthTestEnable(VK_TRUE)
            .setDepthWriteEnable(VK_TRUE)
            .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
            .setDepthBoundsTestEnable(VK_FALSE)
            .setStencilTestEnable(VK_FALSE)
            .setFront(stencilOpState)
            .setBack(stencilOpState)
            .setMinDepthBounds(0.0f)
            .setMaxDepthBounds(0.0f);
        pipelineCreateInfo.setPDepthStencilState(&depthStencilState);

        Vector<vk::PipelineColorBlendAttachmentState> blendAttachmentStates;
        blendAttachmentStates.emplace_back(vk::PipelineColorBlendAttachmentState{}
            .setColorWriteMask(vk::ColorComponentFlags{ 0xf })
            .setBlendEnable(VK_FALSE)
        );
        const auto colorBlendState = vk::PipelineColorBlendStateCreateInfo{}.setAttachments(blendAttachmentStates);
        pipelineCreateInfo.setPColorBlendState(&colorBlendState);

        // Enable dynamic states
        // Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
        // To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
        // For this example we will set the viewport and scissor using dynamic states
        const Vector<vk::DynamicState> dynamicStateEnables =
        {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        const auto dynamicState = vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamicStateEnables);
        pipelineCreateInfo.setPDynamicState(&dynamicState);


        // The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
        pipelineCreateInfo.setLayout(m_pipelineLayout);

        // Renderpass this pipeline is attached to
        pipelineCreateInfo.setRenderPass(m_renderPass);

        m_pipeline = checkVkResult(m_device.createGraphicsPipeline(m_pipelineCache, pipelineCreateInfo));

        // Shader modules are no longer needed once the graphics pipeline has been created
        for (vk::PipelineShaderStageCreateInfo& shaderStage : shaderStages)
        {
            m_device.destroyShaderModule(shaderStage.module);
        }
    }


    // descriptor pool
    {
        // We need to tell the API the number of max. requested descriptors per type
        VkDescriptorPoolSize typeCounts[1];
        // This example only uses one descriptor type (uniform buffer) and only requests one descriptor of this type
        typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        typeCounts[0].descriptorCount = 1;
        // For additional types you need to add new entries in the type count list
        // E.g. for two combined image samplers :
        // typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        // typeCounts[1].descriptorCount = 2;

        // Create the global descriptor pool
        // All descriptors used in this example are allocated from this pool
        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.pNext = nullptr;
        descriptorPoolInfo.poolSizeCount = 1;
        descriptorPoolInfo.pPoolSizes = typeCounts;
        // Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
        descriptorPoolInfo.maxSets = 1;

        VK_CHECK_RESULT(vkCreateDescriptorPool(m_device, &descriptorPoolInfo, nullptr, &m_descriptorPool));
    }



    // descriptor sets
    {
        // Allocate a new descriptor set from the global descriptor pool
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = (VkDescriptorSetLayout*)&m_descriptorSetLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet));

        // Update the descriptor set determining the shader binding points
        // For every binding point used in a shader there needs to be one
        // descriptor set matching that binding point

        VkWriteDescriptorSet writeDescriptorSet = {};

        // Binding 0 : Uniform buffer
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = m_descriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &m_uniformBufferVS.descriptor;
        // Binds this uniform buffer to binding point 0
        writeDescriptorSet.dstBinding = 0;

        vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSet, 0, nullptr);
    }

 


    // Setup a default look-at camera
    m_camera.type = Camera::CameraType::lookat;
    m_camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
    m_camera.setRotation(glm::vec3(0.0f));
    m_camera.setPerspective(60.0f, (float)m_width / (float)m_height, 1.0f, 256.0f);
    m_settings.overlay = true;





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
        auto tStart = std::chrono::high_resolution_clock::now();
        
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
            updateUniformBuffers();

            // Acquire the next image from the swap chain
            VkResult result = m_swapChain.acquireNextImage(m_semaphores.presentComplete, &m_currentBuffer);
            // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
            if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
            {
                windowResize();
            }
            else
            {
                VK_CHECK_RESULT(result);
            }


            // Set up submit info structure
            // Semaphores will stay the same during application lifetime
            m_submitInfo = vks::initializers::submitInfo();
            m_submitInfo.pWaitDstStageMask = &m_submitPipelineStages;
            m_submitInfo.waitSemaphoreCount = 1;
            m_submitInfo.pWaitSemaphores = (VkSemaphore*)&m_semaphores.presentComplete;
            m_submitInfo.signalSemaphoreCount = 1;
            m_submitInfo.pSignalSemaphores = (VkSemaphore*)&m_semaphores.renderComplete;


            m_submitInfo.commandBufferCount = 1;
            m_submitInfo.pCommandBuffers = &m_drawCmdBuffers[m_currentBuffer];
            
            VK_CHECK_RESULT(vkQueueSubmit(m_graphicsQueue, 1, &m_submitInfo, VK_NULL_HANDLE));

            result = m_swapChain.queuePresent(m_graphicsQueue, m_currentBuffer, m_semaphores.renderComplete);
            if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)))
            {
                if (result == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    // Swap chain is no longer compatible with the surface and needs to be recreated
                    windowResize();
                    return;
                }
                else
                {
                    VK_CHECK_RESULT(result);
                }
            }
            VK_CHECK_RESULT(vkQueueWaitIdle(m_graphicsQueue));


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
                m_lastFPS = static_cast<u32>((float)m_frameCounter * (1000.0f / fpsTimer));
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
    if (m_device)
    {
        vkDeviceWaitIdle(m_device);
    }


    vkDestroyPipeline(m_device, m_pipeline, nullptr);

    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    vkDestroyBuffer(m_device, m_vertices.buffer, nullptr);
    vkFreeMemory(m_device, m_vertices.memory, nullptr);

    vkDestroyBuffer(m_device, m_indices.buffer, nullptr);
    vkFreeMemory(m_device, m_indices.memory, nullptr);

    vkDestroyBuffer(m_device, m_uniformBufferVS.buffer, nullptr);
    vkFreeMemory(m_device, m_uniformBufferVS.memory, nullptr);


    m_swapChain.cleanup();
    if (m_descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }
    destroyCommandBuffers();
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    for (u32 i = 0; i < m_frameBuffers.size(); i++)
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




    //if (m_settings.overlay)
    //{
    //    m_UIOverlay.freeResources();
    //}

    //delete m_vulkanDevice;

    //if (m_enableValidationLayers)
    //{
    //    vks::debug::freeDebugCallback(m_instance);
    //}

    vkDestroyDevice(m_device, nullptr);

    if (m_enableValidationLayers)
    {
        VulkanHelpers::destroyDebugUtilsMessengerEXT(m_instance, m_debugCallback, nullptr);
    }


    vkDestroyInstance(m_instance, nullptr);



}













std::string Triangle::getWindowTitle()
{
    std::string windowTitle;
    windowTitle = m_name;
    if (!m_settings.overlay) {
        windowTitle += " - " + std::to_string(m_frameCounter) + " fps";
    }
    return windowTitle;
}

void Triangle::createCommandBuffers()
{
    PROFILE(createCommandBuffers);
    // Create one command buffer for each swap chain image and reuse for rendering
    m_drawCmdBuffers.resize(m_swapChain.imageCount);

    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
        vks::initializers::commandBufferAllocateInfo(
            m_cmdPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            static_cast<u32>(m_drawCmdBuffers.size()));

    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, m_drawCmdBuffers.data()));
}

void Triangle::destroyCommandBuffers()
{
    vkFreeCommandBuffers(m_device, m_cmdPool, static_cast<u32>(m_drawCmdBuffers.size()), m_drawCmdBuffers.data());
}

std::string Triangle::getShadersPath() const
{
    return "Data/shaders/";
}



VkPipelineShaderStageCreateInfo Triangle::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = stage;
    shaderStage.module = vks::tools::loadShader(fileName.c_str(), m_device);
    shaderStage.pName = "main";
    BOF_ASSERT(shaderStage.module != VK_NULL_HANDLE);
    m_shaderModules.push_back(shaderStage.module);
    return shaderStage;
}


void Triangle::updateOverlay()
{
    //if (!m_settings.overlay)
    //    return;

    //ImGuiIO& io = ImGui::GetIO();

    //io.DisplaySize = ImVec2((float)m_width, (float)m_height);
    //io.DeltaTime = m_frameTimer;

    //io.MousePos = ImVec2(m_mousePos.x, m_mousePos.y);
    //io.MouseDown[0] = m_mouseButtons.left;
    //io.MouseDown[1] = m_mouseButtons.right;

    //ImGui::NewFrame();

    //ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    //ImGui::SetNextWindowPos(ImVec2(10, 10));
    //ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    //ImGui::Begin(m_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    //ImGui::TextUnformatted(m_name.c_str());
    //ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / m_lastFPS), m_lastFPS);

    //ImGui::PushItemWidth(110.0f * m_UIOverlay.scale);
    //OnUpdateUIOverlay(&m_UIOverlay);
    //ImGui::PopItemWidth();

    //ImGui::End();
    //ImGui::PopStyleVar();
    //ImGui::Render();

    //if (m_UIOverlay.update() || m_UIOverlay.updated) {
    //    buildCommandBuffers();
    //    m_UIOverlay.updated = false;
    //}

}

void Triangle::drawUI(const VkCommandBuffer commandBuffer)
{
    //if (m_settings.overlay) {
    //    const VkViewport viewport = vks::initializers::viewport((float)m_width, (float)m_height, 0.0f, 1.0f);
    //    const VkRect2D scissor = vks::initializers::rect2D(m_width, m_height, 0, 0);
    //    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    //    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    //    m_UIOverlay.draw(commandBuffer);
    //}
}




void Triangle::setupDPIAwareness()
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




void Triangle::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

        keyPressed((u32)wParam);
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



void Triangle::setupDepthStencil()
{
    PROFILE(setupDepthStencil);

    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = (VkFormat)m_depthFormat;
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
    memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    VK_CHECK_RESULT(vkAllocateMemory(m_device, &memAlloc, nullptr, &m_depthStencil.mem));
    VK_CHECK_RESULT(vkBindImageMemory(m_device, m_depthStencil.image, m_depthStencil.mem, 0));

    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = m_depthStencil.image;
    imageViewCI.format = (VkFormat)m_depthFormat;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (m_depthFormat >= (vk::Format)VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VK_CHECK_RESULT(vkCreateImageView(m_device, &imageViewCI, nullptr, &m_depthStencil.view));
}

void Triangle::setupFrameBuffer()
{
    PROFILE(setupFrameBuffer);

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
    for (u32 i = 0; i < m_frameBuffers.size(); i++)
    {
        attachments[0] = m_swapChain.buffers[i].view;
        VK_CHECK_RESULT(vkCreateFramebuffer(m_device, &frameBufferCreateInfo, nullptr, &m_frameBuffers[i]));
    }
}



void Triangle::buildCommandBuffers()
{
    // Build separate command buffers for every framebuffer image
    // Unlike in OpenGL all rendering commands are recorded once into command buffers that are then resubmitted to the queue
    // This allows to generate work upfront and from multiple threads, one of the biggest advantages of Vulkan
    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
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

        // Start the first sub pass specified in our default render pass setup by the base class
        // This will clear the color and depth attachment
        vkCmdBeginRenderPass(m_drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Update dynamic viewport state
        VkViewport viewport = {};
        viewport.height = (float)m_height;
        viewport.width = (float)m_width;
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        vkCmdSetViewport(m_drawCmdBuffers[i], 0, 1, &viewport);

        // Update dynamic scissor state
        VkRect2D scissor = {};
        scissor.extent.width = m_width;
        scissor.extent.height = m_height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(m_drawCmdBuffers[i], 0, 1, &scissor);

        // Bind descriptor sets describing shader binding points
        vkCmdBindDescriptorSets(m_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

        // Bind the rendering pipeline
        // The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
        vkCmdBindPipeline(m_drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        // Bind triangle vertex buffer (contains position and colors)
        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(m_drawCmdBuffers[i], 0, 1, (VkBuffer*)&m_vertices.buffer, offsets);

        // Bind triangle index buffer
        vkCmdBindIndexBuffer(m_drawCmdBuffers[i], m_indices.buffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw indexed triangle
        vkCmdDrawIndexed(m_drawCmdBuffers[i], m_indices.count, 1, 0, 0, 1);

        vkCmdEndRenderPass(m_drawCmdBuffers[i]);

        // Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
        // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

        VK_CHECK_RESULT(vkEndCommandBuffer(m_drawCmdBuffers[i]));
    }
}


void Triangle::windowResize()
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
    for (u32 i = 0; i < m_frameBuffers.size(); i++) {
        vkDestroyFramebuffer(m_device, m_frameBuffers[i], nullptr);
    }
    setupFrameBuffer();

    //if ((m_width > 0.0f) && (m_height > 0.0f)) {
    //    if (m_settings.overlay) {
    //        m_UIOverlay.resize(m_width, m_height);
    //    }
    //}

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

void Triangle::handleMouseMove(int32_t x, int32_t y)
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



void Triangle::updateUniformBuffers()
{
    // Pass matrices to the shaders
    m_uboVS.projectionMatrix = m_camera.matrices.perspective;
    m_uboVS.viewMatrix = m_camera.matrices.view;
    m_uboVS.modelMatrix = glm::mat4(1.0f);

    // Map uniform buffer and update it
    uint8_t* pData;
    VK_CHECK_RESULT(vkMapMemory(m_device, m_uniformBufferVS.memory, 0, sizeof(m_uboVS), 0, (void**)&pData));
    memcpy(pData, &m_uboVS, sizeof(m_uboVS));
    // Unmap after data has been copied
    // Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
    vkUnmapMemory(m_device, m_uniformBufferVS.memory);
}

// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
// Upon success it will return the index of the memory type that fits our requested memory properties
// This is necessary as implementations can offer an arbitrary number of memory types with different
// memory properties.
// You can check http://vulkan.gpuinfo.org/ for details on different memory configurations
u32 Triangle::getMemoryTypeIndex(u32 acceptableTypeBits, vk::MemoryPropertyFlags desiredProperties)
{
    // Iterate over all memory types available for the device used in this example
    for (u32 i = 0; i < m_deviceMemoryProperties.memoryTypeCount; i++)
    {
        // make sure this memory type index is part of the acceptable types (and with one, so we check the last bit, shift every loop)
        if ((acceptableTypeBits & 1) == 1)
        {
            const vk::MemoryType& availableMemoryType = m_deviceMemoryProperties.memoryTypes[i];
            const vk::MemoryPropertyFlags& thisMemoryPropertyFlags = availableMemoryType.propertyFlags;

            // check if this memory type has all desired properties
            if ((thisMemoryPropertyFlags & desiredProperties) == desiredProperties)
            {
                return i; // return the first one we find
            }
        }
        // this shifts to the next bit to be anded with 1
        acceptableTypeBits >>= 1;
    }

    throw "Could not find a suitable memory type!";
}


// Vulkan loads its shaders from an immediate binary representation called SPIR-V
// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
// This function loads such a shader from a binary file and returns a shader module structure
VkShaderModule Triangle::loadSPIRVShader(String filename)
{
    size_t shaderSize;
    char* shaderCode = NULL;

    std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open())
    {
        shaderSize = is.tellg();
        is.seekg(0, std::ios::beg);
        // Copy file contents into a buffer
        shaderCode = new char[shaderSize];
        is.read(shaderCode, shaderSize);
        is.close();
        assert(shaderSize > 0);
    }

    if (shaderCode)
    {
        // Create a new shader module that will be used for pipeline creation
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = shaderSize;
        moduleCreateInfo.pCode = (u32*)shaderCode;

        VkShaderModule shaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(m_device, &moduleCreateInfo, NULL, &shaderModule));

        delete[] shaderCode;

        return shaderModule;
    }
    else
    {
        std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
        return VK_NULL_HANDLE;
    }
}


// Get a new command buffer from the command pool
// If begin is true, the command buffer is also started so we can start adding commands
VkCommandBuffer Triangle::getCommandBuffer(bool begin)
{
    VkCommandBuffer cmdBuffer;

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = m_cmdPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;

    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &cmdBuffer));

    // If requested, also start the new command buffer
    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
    }

    return cmdBuffer;
}

u32 Triangle::getQueueFamilyIndex(vk::QueueFlagBits queueFlags)
{
    // Dedicated queue for compute
    // Try to find a queue family index that supports compute but not graphics
    if (queueFlags & vk::QueueFlagBits::eCompute)
    {
        for (u32 i = 0; i < static_cast<u32>(m_queueFamilyProperties.size()); i++)
        {
            if ((m_queueFamilyProperties[i].queueFlags & queueFlags) && 
                ((m_queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) == (vk::QueueFlagBits)0))
            {
                return i;
            }
        }
    }

    // Dedicated queue for transfer
    // Try to find a queue family index that supports transfer but not graphics and compute
    if (queueFlags & vk::QueueFlagBits::eTransfer)
    {
        for (u32 i = 0; i < static_cast<u32>(m_queueFamilyProperties.size()); i++)
        {
            if ((m_queueFamilyProperties[i].queueFlags & queueFlags) && 
                ((m_queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) == (vk::QueueFlagBits)0) &&
                ((m_queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute) == (vk::QueueFlagBits)0))
            {
                return i;
            }
        }
    }

    // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
    for (u32 i = 0; i < (u32)m_queueFamilyProperties.size(); i++)
    {
        if (m_queueFamilyProperties[i].queueFlags & queueFlags)
        {
            return i;
        }
    }
    BOF_FAIL("Could not find a matching queue family index");
    return 0;
}





#if 1

Triangle* vulkanExampleBase = nullptr;
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

    vulkanExampleBase = new Triangle();

    vulkanExampleBase->run(hInstance, WndProc, args);

    delete(vulkanExampleBase);
    
    
    std::cout << "bye" << std::endl;
    
    return 0;
}

#endif




