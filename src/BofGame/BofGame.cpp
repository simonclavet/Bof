#include "stdafx.h"


#pragma warning(disable: 4324) // prevent warning when custum aligning 

#include "utils/VulkanHelpers.h"

#include "utils/RingBuffer.h"

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static Vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
    {
        Vector<vk::VertexInputAttributeDescription> attributeDescriptions;

        {
            auto& attributeDescription = attributeDescriptions.emplace_back(vk::VertexInputAttributeDescription{});;
            attributeDescription.binding = 0;
            attributeDescription.location = 0;
            attributeDescription.format = vk::Format::eR32G32B32Sfloat;
            attributeDescription.offset = offsetof(Vertex, pos);
        }

        {
            auto& attributeDescription = attributeDescriptions.emplace_back(vk::VertexInputAttributeDescription{});;
            attributeDescription.binding = 0;
            attributeDescription.location = 1;
            attributeDescription.format = vk::Format::eR32G32B32Sfloat;
            attributeDescription.offset = offsetof(Vertex, color);
        }

        {
            auto& attributeDescription = attributeDescriptions.emplace_back(vk::VertexInputAttributeDescription{});;
            attributeDescription.binding = 0;
            attributeDescription.location = 2;
            attributeDescription.format = vk::Format::eR32G32Sfloat;
            attributeDescription.offset = offsetof(Vertex, texCoord);
        }

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const
    {
        return
            pos == other.pos &&
            color == other.color &&
            texCoord == other.texCoord;
    }
};

namespace std
{
    template<>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            const size_t h1 = hash<glm::vec3>()(vertex.pos);
            const size_t h2 = hash<glm::vec3>()(vertex.color);
            const size_t h3 = hash<glm::vec2>()(vertex.texCoord);

            size_t h = h1;
            h = h << 1;
            h ^= h2;
            h = h >> 1;
            h ^= (h3 >> 1); // but why... probably not important
            
            return h;
        }
    };
}

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};







class BofGame
{
public:
    void run()
    {       
        init();
        mainLoop();
        cleanup();
    }

private:


 
    void init()
    {
        PROFILE(init);

        {
            PROFILE(initWindow);

            glfwInit();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            m_window = glfwCreateWindow(800, 600, "Bof Engine", nullptr, nullptr);
            glfwSetWindowUserPointer(m_window, this);
            glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
        }
        {
            PROFILE(createInstance);

            if (m_enableValidationLayers)
            {
                BOF_ASSERT(VulkanHelpers::checkLayerSupport(m_validationLayers), 
                    "validation layers requested, but not available");
            }
            
            vk::InstanceCreateInfo instanceCreateInfo{};

            vk::ApplicationInfo appInfo{};
            appInfo.pApplicationName = "Bof";
            appInfo.apiVersion = VK_API_VERSION_1_0;
            appInfo.pEngineName = "BofEngine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            instanceCreateInfo.pApplicationInfo = &appInfo;

            const Vector<const char*> extensions = VulkanHelpers::getRequiredExtensions(m_enableValidationLayers);
            instanceCreateInfo.setPEnabledExtensionNames(extensions);

            if (m_enableValidationLayers)
            {
                instanceCreateInfo.setPEnabledLayerNames(m_validationLayers);
            }

            m_instance = vk::createInstanceUnique(instanceCreateInfo);
        }
        {
            PROFILE(setupDebugCallback);

            if (m_enableValidationLayers)
            {
                vk::DebugUtilsMessengerCreateInfoEXT debugUtilsInfo{};
                debugUtilsInfo.messageSeverity =
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
                debugUtilsInfo.messageType =
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
                debugUtilsInfo.pfnUserCallback = debugCallback;

                if (VulkanHelpers::createDebugUtilsMessengerEXT(
                    m_instance,
                    reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&debugUtilsInfo),
                    nullptr, &m_debugCallback) != VK_SUCCESS)
                {
                    BOF_FAIL("failed to set up debug callback");
                }
            }
        }
        {
            PROFILE(createSurface);

            VkSurfaceKHR rawSurface;
            if (glfwCreateWindowSurface(*m_instance, m_window, nullptr, &rawSurface) != VK_SUCCESS)
            {
                BOF_FAIL("failed to create window surface");
            }
            m_surface = rawSurface;
        }
        {
            PROFILE(pickPhysicalDevice);
            const std::vector<vk::PhysicalDevice> physicalDevices = m_instance->enumeratePhysicalDevices();
            BOF_ASSERT(!physicalDevices.empty(), "failed to find GPUs with Vulkan support");
            
            for (const vk::PhysicalDevice& physicalDevice : physicalDevices)
            {
                if (VulkanHelpers::isDeviceSuitable(physicalDevice, m_surface, m_deviceExtensions))
                {
                    m_physicalDevice = physicalDevice;
                    m_msaaSamples = VulkanHelpers::getMaxUsableSampleCount(m_physicalDevice);
                    break;
                }
            }
            BOF_ASSERT(m_physicalDevice, "failed to find a suitable GPU");
        }
        {
            PROFILE(createLogicalDevice);

            QueueFamilyIndices indices = VulkanHelpers::findQueueFamilies(m_physicalDevice, m_surface);

            // probably the same
            const std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

            // just one queue per queue family
            const Vector<float> queuePriorities = { 1.0f };
            Vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
            for (const uint32_t& queueFamily : uniqueQueueFamilies)
            {
                auto& queueCreateInfo = queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo{});
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.setQueuePriorities(queuePriorities);
            }

            vk::DeviceCreateInfo deviceCreateInfo{};
            deviceCreateInfo.setQueueCreateInfos(queueCreateInfos);

            vk::PhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

            deviceCreateInfo.setPEnabledExtensionNames(m_deviceExtensions);

            if (m_enableValidationLayers)
            {
                deviceCreateInfo.setPEnabledLayerNames(m_validationLayers);
            }

            m_device = m_physicalDevice.createDeviceUnique(deviceCreateInfo);

            m_graphicsQueue = m_device->getQueue(indices.graphicsFamily.value(), 0);
            m_presentQueue = m_device->getQueue(indices.presentFamily.value(), 0);
        }
        {
            PROFILE(createCommandPool);
            QueueFamilyIndices queueFamilyIndices = VulkanHelpers::findQueueFamilies(
                m_physicalDevice, m_surface);

            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
            poolInfo.flags = 0;

            m_commandPool = checkVkResult(m_device->createCommandPool(poolInfo));
        }
        {
            PROFILE(createSyncObjects);
            m_imageAvailableSemaphores.resize(m_maxFramesInFlight);
            m_renderFinishedSemaphores.resize(m_maxFramesInFlight);
            m_inFlightFences.resize(m_maxFramesInFlight);

            vk::FenceCreateInfo fenceCreateInfo{};
            fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

            for (size_t i = 0; i < m_maxFramesInFlight; i++)
            {
                m_imageAvailableSemaphores[i] = checkVkResult(m_device->createSemaphore({}));
                m_renderFinishedSemaphores[i] = checkVkResult(m_device->createSemaphore({}));
                m_inFlightFences[i] = checkVkResult(m_device->createFence(fenceCreateInfo));
            }
        }
        {
            PROFILE(createTextureImage);

            VulkanHelpers::createTextureImage(
                m_texturePath.c_str(),
                m_physicalDevice,
                m_device,
                m_graphicsQueue,
                m_commandPool,
                // output
                m_textureImage,
                m_textureImageMemory,
                m_mipLevels);
        }
        {
            PROFILE(createTextureImageView);

            m_textureImageView = VulkanHelpers::createImageView(
                m_textureImage,
                vk::Format::eR8G8B8A8Srgb,
                vk::ImageAspectFlagBits::eColor,
                m_mipLevels,
                m_device);
        }
        {
            PROFILE(createTextureSampler);
            vk::SamplerCreateInfo samplerInfo{};
            samplerInfo.magFilter = vk::Filter::eLinear;
            samplerInfo.minFilter = vk::Filter::eLinear;
            samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
            samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
            samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = vk::CompareOp::eAlways;
            samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(m_mipLevels);

            m_textureSampler = checkVkResult(m_device->createSampler(samplerInfo));
        }
        {
            PROFILE(loadModel);

            tinyobj::attrib_t attrib;
            Vector<tinyobj::shape_t> shapes;
            Vector<tinyobj::material_t> materials;
            String warn, err;

            bool result = tinyobj::LoadObj(
                &attrib,
                &shapes,
                &materials,
                &warn,
                &err,
                m_modelPath.c_str());

            if (warn.length() > 0)
            {
                std::cout << "warning while loading " << m_modelPath << ": " << warn << std::endl;
            }
            BOF_ASSERT(result, "can't load model %s, %s, %s", m_modelPath.c_str(), warn.c_str(), err.c_str());


            std::unordered_map<Vertex, uint32_t> uniqueVertices{};


            for (const tinyobj::shape_t& shape : shapes)
            {
                for (const tinyobj::index_t& index : shape.mesh.indices)
                {
                    Vertex vertex{};

                    const int vertexFirstCoordIndex = 3 * (int)index.vertex_index;
                    vertex.pos =
                    {
                        attrib.vertices[vertexFirstCoordIndex + 0],
                        attrib.vertices[vertexFirstCoordIndex + 1],
                        attrib.vertices[vertexFirstCoordIndex + 2],
                    };
                    const int texFirstCoordIndex = 2 * (int)index.texcoord_index;
                    vertex.texCoord =
                    {
                        attrib.texcoords[texFirstCoordIndex + 0],
                        1.0f - attrib.texcoords[texFirstCoordIndex + 1]
                    };

                    vertex.color = { 1.0f, 1.0f, 1.0f };

                    // add it only if new
                    if (uniqueVertices.count(vertex) == 0)
                    {
                        uint32_t newVertexIndex = static_cast<uint32_t>(m_vertices.size());

                        m_vertices.push_back(vertex);

                        // remember that this unique vertex has this index in m_vertices 
                        uniqueVertices[vertex] = newVertexIndex;
                    }

                    m_indices.push_back(uniqueVertices[vertex]);
                }
            }
        }
        {
            PROFILE(createVertexBuffer);
            VulkanHelpers::createAndFillBuffer(
                m_vertices,
                vk::BufferUsageFlagBits::eVertexBuffer,
                m_physicalDevice,
                m_device,
                m_graphicsQueue,
                m_commandPool,
                m_vertexBuffer,
                m_vertexBufferMemory);
        }
        {
            PROFILE(createIndexBuffer);
            VulkanHelpers::createAndFillBuffer(
                m_indices,
                vk::BufferUsageFlagBits::eIndexBuffer,
                m_physicalDevice,
                m_device,
                m_graphicsQueue,
                m_commandPool,
                m_indexBuffer,
                m_indexBufferMemory);
        }
        {
            PROFILE(createDescriptorSetLayout);

            Vector<vk::DescriptorSetLayoutBinding> layoutBindings;

            auto& uboLayoutBinding = layoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{});
            uboLayoutBinding.binding = 0;
            uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
            uboLayoutBinding.pImmutableSamplers = nullptr;

            auto& samplerLayoutBinding = layoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{});
            samplerLayoutBinding.binding = 1;
            samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
            samplerLayoutBinding.pImmutableSamplers = nullptr;

            vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.setBindings(layoutBindings);

            m_descriptorSetLayout = checkVkResult(m_device->createDescriptorSetLayout(layoutInfo));
        }
        createSwapchainAndRelatedThings();
    }







    void createSwapchainAndRelatedThings()
    {
        PROFILE(createSwapchainAndRelatedThings);
        {
            PROFILE(createSwapchain);

            const SwapChainSupportDetails swapchainSupport = VulkanHelpers::querySwapChainSupport(m_physicalDevice, m_surface);

            vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
            swapchainCreateInfo.surface = m_surface;

            uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
            if (swapchainSupport.capabilities.maxImageCount > 0 &&
                imageCount > swapchainSupport.capabilities.maxImageCount)
            {
                imageCount = swapchainSupport.capabilities.maxImageCount;
            }
            swapchainCreateInfo.minImageCount = imageCount;

            const vk::SurfaceFormatKHR surfaceFormat = VulkanHelpers::chooseSwapSurfaceFormat(swapchainSupport.formats);
            m_swapchainImageFormat = surfaceFormat.format;
            swapchainCreateInfo.imageFormat = m_swapchainImageFormat;

            swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;

            m_swapchainExtent = VulkanHelpers::chooseSwapExtent(swapchainSupport.capabilities, m_window);
            swapchainCreateInfo.imageExtent = m_swapchainExtent;

            swapchainCreateInfo.imageArrayLayers = 1;

            swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

            QueueFamilyIndices indices = VulkanHelpers::findQueueFamilies(m_physicalDevice, m_surface);
            uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

            if (indices.graphicsFamily != indices.presentFamily)
            {
                swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
                swapchainCreateInfo.queueFamilyIndexCount = 2;
                swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else
            {
                swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
            }

            swapchainCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;
            swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

            const vk::PresentModeKHR presentMode = VulkanHelpers::chooseSwapPresentMode(swapchainSupport.presentModes);
            swapchainCreateInfo.presentMode = presentMode;

            swapchainCreateInfo.clipped = VK_TRUE;

            swapchainCreateInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

            m_swapchain = checkVkResult(m_device->createSwapchainKHR(swapchainCreateInfo));

            m_swapchainImages = checkVkResult(m_device->getSwapchainImagesKHR(m_swapchain));

        }

        {
            PROFILE(createSwapchainImageViews);

            m_swapchainImageViews.resize(m_swapchainImages.size());

            for (size_t i = 0; i < m_swapchainImages.size(); i++)
            {
                const uint32_t mipLevelsJustOne = 1;
                m_swapchainImageViews[i] = VulkanHelpers::createImageView(
                    m_swapchainImages[i],
                    m_swapchainImageFormat,
                    vk::ImageAspectFlagBits::eColor,
                    mipLevelsJustOne,
                    m_device);
            }
        }

        {
            PROFILE(createRenderPass);

            vk::RenderPassCreateInfo renderPassInfo{};

            std::array<vk::AttachmentDescription, m_attachmentCount> attachments;

            {
                auto& colorAttachment = attachments[m_colorImageAttachmentIndex];
                colorAttachment = vk::AttachmentDescription{};
                colorAttachment.format = m_swapchainImageFormat;
                colorAttachment.samples = m_msaaSamples;

                colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

                colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
                colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

                colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
                colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
            }

            {
                auto& depthAttachment = attachments[m_depthImageAttachmentIndex];
                depthAttachment.format = VulkanHelpers::findDepthFormat(m_physicalDevice);
                depthAttachment.samples = m_msaaSamples;

                depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;

                depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
                depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

                depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
                depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            }

            {
                auto& resolveAttachment = attachments[m_resolveAttachmentIndex];
                resolveAttachment.format = m_swapchainImageFormat;
                resolveAttachment.samples = vk::SampleCountFlagBits::e1;

                resolveAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
                resolveAttachment.storeOp = vk::AttachmentStoreOp::eStore;

                resolveAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
                resolveAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

                resolveAttachment.initialLayout = vk::ImageLayout::eUndefined;
                resolveAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
            }

            renderPassInfo.setAttachments(attachments);

            Vector<vk::AttachmentReference> colorAttachmentRefs;
            {
                auto& colorAttachmentRef = colorAttachmentRefs.emplace_back(vk::AttachmentReference{});
                colorAttachmentRef.attachment = m_colorImageAttachmentIndex;
                colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
            }

            vk::AttachmentReference resolveColorAttachmentRef{};
            resolveColorAttachmentRef.attachment = m_resolveAttachmentIndex;
            resolveColorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
            
            vk::AttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = m_depthImageAttachmentIndex;
            depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;


            Vector<vk::SubpassDescription> subpasses;
            {
                auto& subpass = subpasses.emplace_back(vk::SubpassDescription{});
                subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

                subpass.setColorAttachments(colorAttachmentRefs);
                subpass.pDepthStencilAttachment = &depthAttachmentRef;

                subpass.pResolveAttachments = &resolveColorAttachmentRef;
            }
            renderPassInfo.setSubpasses(subpasses);


            Vector<vk::SubpassDependency> dependencies;
            {
                auto& dependency = dependencies.emplace_back(vk::SubpassDependency{});
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
                dependency.srcAccessMask = {};
                dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
                dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            }
            renderPassInfo.setDependencies(dependencies);

            m_renderPass = checkVkResult(m_device->createRenderPass(renderPassInfo));
        }

        {
            PROFILE(createGraphicsPipeline);

            vk::GraphicsPipelineCreateInfo pipelineInfo{};

            Vector<char> vertShaderCode = VulkanHelpers::readFile("builtShaders/shader.vert.spv");
            Vector<char> fragShaderCode = VulkanHelpers::readFile("builtShaders/shader.frag.spv");

            vk::UniqueShaderModule vertShaderModule = VulkanHelpers::createShaderModule(m_device, vertShaderCode);
            vk::UniqueShaderModule fragShaderModule = VulkanHelpers::createShaderModule(m_device, fragShaderCode);

            Vector<vk::PipelineShaderStageCreateInfo> shaderStages;
            {
                auto& shaderStage = shaderStages.emplace_back(vk::PipelineShaderStageCreateInfo{});
                shaderStage.stage = vk::ShaderStageFlagBits::eVertex;
                shaderStage.module = *vertShaderModule;
                shaderStage.pName = "main";
            }
            {
                auto& shaderStage = shaderStages.emplace_back(vk::PipelineShaderStageCreateInfo{});
                shaderStage.stage = vk::ShaderStageFlagBits::eFragment;
                shaderStage.module = *fragShaderModule;
                shaderStage.pName = "main";
            }
            pipelineInfo.setStages(shaderStages);


            vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};

            Vector<vk::VertexInputBindingDescription> bindingDescriptions = { Vertex::getBindingDescription() };
            vertexInputInfo.setVertexBindingDescriptions(bindingDescriptions);

            Vector<vk::VertexInputAttributeDescription> attributeDescriptions = Vertex::getAttributeDescriptions();
            vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

            pipelineInfo.pVertexInputState = &vertexInputInfo;


            vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
            inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            pipelineInfo.pInputAssemblyState = &inputAssembly;


            vk::PipelineViewportStateCreateInfo viewportState{};

            Vector<vk::Viewport> viewports;
            viewports.emplace_back(vk::Viewport{ 0.0f, 0.0f, (float)m_swapchainExtent.width, (float)m_swapchainExtent.height, 0.0f, 1.0f });
            viewportState.setViewports(viewports);
            Vector<vk::Rect2D> scissors = { vk::Rect2D({ 0, 0 }, m_swapchainExtent) };

            viewportState.setScissors(scissors);

            pipelineInfo.pViewportState = &viewportState;


            vk::PipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = vk::PolygonMode::eFill;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = vk::CullModeFlagBits::eBack;
            rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f;
            rasterizer.depthBiasClamp = 0.0f;
            rasterizer.depthBiasSlopeFactor = 0.0f;

            pipelineInfo.pRasterizationState = &rasterizer;


            vk::PipelineMultisampleStateCreateInfo multisampling{};
            multisampling.rasterizationSamples = m_msaaSamples;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.minSampleShading = 1.0f;
            multisampling.pSampleMask = nullptr;
            multisampling.alphaToCoverageEnable = VK_FALSE;
            multisampling.alphaToOneEnable = VK_FALSE;

            pipelineInfo.pMultisampleState = &multisampling;


            vk::PipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = vk::CompareOp::eLess;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.minDepthBounds = 0.0f;
            depthStencil.maxDepthBounds = 1.0f;
            depthStencil.stencilTestEnable = VK_FALSE;
            depthStencil.front = vk::StencilOpState{};
            depthStencil.back = vk::StencilOpState{};

            pipelineInfo.pDepthStencilState = &depthStencil;



            vk::PipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = vk::LogicOp::eCopy;


            Vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
            auto& colorBlendAttachment = colorBlendAttachments.emplace_back(vk::PipelineColorBlendAttachmentState{});
            colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
            colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
            colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
            colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

            colorBlending.setAttachments(colorBlendAttachments);
            colorBlending.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

            pipelineInfo.pColorBlendState = &colorBlending;

            pipelineInfo.pDynamicState = nullptr;


            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};

            Vector<vk::DescriptorSetLayout> descriptorSetLayouts = { m_descriptorSetLayout };
            pipelineLayoutInfo.setSetLayouts(descriptorSetLayouts);

            pipelineLayoutInfo.setPushConstantRanges(Vector<vk::PushConstantRange>());

            m_pipelineLayout = checkVkResult(m_device->createPipelineLayout(pipelineLayoutInfo));

            pipelineInfo.layout = m_pipelineLayout;


            pipelineInfo.renderPass = m_renderPass;
            pipelineInfo.subpass = 0;

            pipelineInfo.basePipelineHandle = nullptr;
            pipelineInfo.basePipelineIndex = -1;


            m_graphicsPipeline = checkVkResult(m_device->createGraphicsPipeline(nullptr, pipelineInfo));
        }
        {
            PROFILE(createColorResources);
            
            const vk::Format colorFormat = m_swapchainImageFormat;
            const uint32_t mipLevelsJustOne = 1;
            VulkanHelpers::createImageAndImageView(
                m_swapchainExtent,
                mipLevelsJustOne,
                m_msaaSamples,
                colorFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                vk::ImageAspectFlagBits::eColor,
                m_physicalDevice,
                m_device,
                m_colorImage,
                m_colorImageMemory,
                m_colorImageView);
        }
        {
            PROFILE(createDepthResources);

            const vk::Format depthFormat = VulkanHelpers::findDepthFormat(m_physicalDevice);
            const uint32_t mipLevelsJustOne = 1;
            VulkanHelpers::createImageAndImageView(
                m_swapchainExtent,
                mipLevelsJustOne,
                m_msaaSamples,
                depthFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                vk::ImageAspectFlagBits::eDepth,
                m_physicalDevice,
                m_device,
                m_depthImage,
                m_depthImageMemory,
                m_depthImageView);

            VulkanHelpers::transitionImageLayout(
                m_depthImage,
                depthFormat,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthAttachmentOptimal,
                mipLevelsJustOne,
                m_device, m_graphicsQueue, m_commandPool);
        }

        {
            PROFILE(createFramebuffers);
            m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

            for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
            {
                vk::FramebufferCreateInfo framebufferInfo = {};
                framebufferInfo.renderPass = m_renderPass;

                std::array<vk::ImageView, m_attachmentCount> attachments;
                attachments[m_colorImageAttachmentIndex] = m_colorImageView;
                attachments[m_depthImageAttachmentIndex] = m_depthImageView;
                attachments[m_resolveAttachmentIndex] = m_swapchainImageViews[i];
                framebufferInfo.setAttachments(attachments);

                framebufferInfo.width = m_swapchainExtent.width;
                framebufferInfo.height = m_swapchainExtent.height;
                framebufferInfo.layers = 1;

                m_swapchainFramebuffers[i] = checkVkResult(m_device->createFramebuffer(framebufferInfo));
            }
        }

        {
            PROFILE(createUniformBuffers);

            const vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

            m_uniformBuffers.resize(m_swapchainImages.size());
            m_uniformBuffersMemory.resize(m_swapchainImages.size());

            for (size_t i = 0; i < m_swapchainImages.size(); i++)
            {
                VulkanHelpers::createBuffer(
                    bufferSize,
                    vk::BufferUsageFlagBits::eUniformBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                    m_physicalDevice,
                    m_device,
                    m_uniformBuffers[i],
                    m_uniformBuffersMemory[i]);
            }
        }

        {
            PROFILE(createDescriptorPool);

            uint32_t swapchainImagesCount = static_cast<uint32_t>(m_swapchainImages.size());

            Vector<vk::DescriptorPoolSize> poolSizes;
            {
                auto& uniformBufferPoolSize = poolSizes.emplace_back(vk::DescriptorPoolSize{});
                uniformBufferPoolSize.type = vk::DescriptorType::eUniformBuffer;
                uniformBufferPoolSize.descriptorCount = swapchainImagesCount;
            }
            {
                auto& samplerPoolSize = poolSizes.emplace_back(vk::DescriptorPoolSize{});
                samplerPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
                samplerPoolSize.descriptorCount = swapchainImagesCount;
            }

            vk::DescriptorPoolCreateInfo poolInfo{};
            poolInfo.setPoolSizes(poolSizes);
            poolInfo.maxSets = swapchainImagesCount;

            m_descriptorPool = checkVkResult(m_device->createDescriptorPool(poolInfo, nullptr));
        }

        {
            PROFILE(createDescriptorSets);

            vk::DescriptorSetAllocateInfo allocInfo{};

            allocInfo.descriptorPool = m_descriptorPool;

            Vector<vk::DescriptorSetLayout> layouts(m_swapchainImages.size(), m_descriptorSetLayout);
            allocInfo.setSetLayouts(layouts);

            m_descriptorSets = checkVkResult(m_device->allocateDescriptorSets(allocInfo));

            for (size_t i = 0; i < m_swapchainImages.size(); i++)
            {
                Vector<vk::WriteDescriptorSet> descriptorWrites;

                auto& bufferDescriptorWrite = descriptorWrites.emplace_back(vk::WriteDescriptorSet{});
                bufferDescriptorWrite.dstSet = m_descriptorSets[i];
                bufferDescriptorWrite.dstBinding = static_cast<uint32_t>(descriptorWrites.size() - 1);
                bufferDescriptorWrite.dstArrayElement = 0;
                bufferDescriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;

                vk::DescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = m_uniformBuffers[i];
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(UniformBufferObject);

                Vector<vk::DescriptorBufferInfo> bufferInfos = { bufferInfo };
                bufferDescriptorWrite.setBufferInfo(bufferInfos);


                auto& samplerDescriptorWrite = descriptorWrites.emplace_back(vk::WriteDescriptorSet{});
                samplerDescriptorWrite.dstSet = m_descriptorSets[i];
                samplerDescriptorWrite.dstBinding = static_cast<uint32_t>(descriptorWrites.size() - 1);
                samplerDescriptorWrite.dstArrayElement = 0;
                samplerDescriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;

                vk::DescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                imageInfo.imageView = m_textureImageView;
                imageInfo.sampler = m_textureSampler;

                Vector<vk::DescriptorImageInfo> imageInfos = { imageInfo };
                samplerDescriptorWrite.setImageInfo(imageInfos);

                m_device->updateDescriptorSets(descriptorWrites, nullptr);
            }
        }

        {
            PROFILE(createCommandBuffers);

            m_commandBuffers.resize(m_swapchainFramebuffers.size());

            vk::CommandBufferAllocateInfo allocInfo = {};
            allocInfo.commandPool = m_commandPool;
            allocInfo.level = vk::CommandBufferLevel::ePrimary;
            allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

            m_commandBuffers = checkVkResult(m_device->allocateCommandBuffers(allocInfo));

            for (size_t i = 0; i < m_commandBuffers.size(); i++)
            {
                vk::CommandBuffer& commandBuffer = m_commandBuffers[i];

                vk::CommandBufferBeginInfo beginInfo = {};
                beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
                beginInfo.pInheritanceInfo = nullptr;

                commandBuffer.begin(beginInfo);

                vk::RenderPassBeginInfo renderPassInfo = {};
                renderPassInfo.renderPass = m_renderPass;
                renderPassInfo.framebuffer = m_swapchainFramebuffers[i];
                renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
                renderPassInfo.renderArea.extent = m_swapchainExtent;

                std::array<vk::ClearValue, m_attachmentCount> clearValues{};

                clearValues[m_colorImageAttachmentIndex].color =
                    std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };

                clearValues[m_depthImageAttachmentIndex].depthStencil =
                    vk::ClearDepthStencilValue{ 1.0f, 0 };


                renderPassInfo.setClearValues(clearValues);

                commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

                commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);

                Vector<vk::Buffer> vertexBuffers = { m_vertexBuffer };
                Vector<vk::DeviceSize> vertexOffsets = { 0 };

                const uint32_t firstBinding = 0;
                commandBuffer.bindVertexBuffers(firstBinding, vertexBuffers, vertexOffsets);

                const vk::DeviceSize indexOffset = 0;

                constexpr vk::IndexType indexType = vk::IndexTypeValue<decltype(m_indices)::value_type>::value;
                commandBuffer.bindIndexBuffer(m_indexBuffer, indexOffset, indexType);

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                    m_pipelineLayout, 0, m_descriptorSets[i], nullptr);


                const uint32_t indexCount = static_cast<uint32_t>(m_indices.size());
                const uint32_t instanceCount = 1;
                const uint32_t firstIndex = 0;
                const uint32_t vertexOffset = 0;
                const uint32_t firstInstance = 0;
                commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);


                commandBuffer.endRenderPass();
                commandBuffer.end();
            }
        }
    }

    void cleanup()
    {
        PROFILE(cleanup);

        cleanupSwapchain();

        m_device->destroySampler(m_textureSampler);
        m_device->destroyImageView(m_textureImageView);
        m_device->destroyImage(m_textureImage);
        m_device->freeMemory(m_textureImageMemory);

        m_device->destroyDescriptorSetLayout(m_descriptorSetLayout);

        m_device->destroyBuffer(m_indexBuffer);
        m_device->freeMemory(m_indexBufferMemory);

        m_device->destroyBuffer(m_vertexBuffer);
        m_device->freeMemory(m_vertexBufferMemory);

        for (size_t i = 0; i < m_maxFramesInFlight; i++)
        {
            m_device->destroySemaphore(m_imageAvailableSemaphores[i]);
            m_device->destroySemaphore(m_renderFinishedSemaphores[i]);
            m_device->destroyFence(m_inFlightFences[i]);
        }

        m_device->destroyCommandPool(m_commandPool);

        m_instance->destroySurfaceKHR(m_surface);

        if (m_enableValidationLayers)
        {
            VulkanHelpers::destroyDebugUtilsMessengerEXT(*m_instance, m_debugCallback, nullptr);
        }

        glfwDestroyWindow(m_window);

        glfwTerminate();
    }



    void cleanupSwapchain()
    {
        m_device->destroyImageView(m_colorImageView);
        m_device->destroyImage(m_colorImage);
        m_device->freeMemory(m_colorImageMemory);

        m_device->destroyImageView(m_depthImageView);
        m_device->destroyImage(m_depthImage);
        m_device->freeMemory(m_depthImageMemory);

        for (vk::Framebuffer& framebuffer : m_swapchainFramebuffers)
        {
            m_device->destroyFramebuffer(framebuffer);
        }

        m_device->freeCommandBuffers(m_commandPool, m_commandBuffers);

        m_device->destroyPipeline(m_graphicsPipeline);
        m_device->destroyPipelineLayout(m_pipelineLayout);
        m_device->destroyRenderPass(m_renderPass);

        for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
        {
            m_device->destroyImageView(m_swapchainImageViews[i]);

            m_device->destroyBuffer(m_uniformBuffers[i]);
            m_device->freeMemory(m_uniformBuffersMemory[i]);
        }

        m_device->destroyDescriptorPool(m_descriptorPool);

        m_device->destroySwapchainKHR(m_swapchain);
    }


    void recreateSwapchain()
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);

        while (width == 0 || height == 0)
        {
            glfwWaitEvents();
            glfwGetFramebufferSize(m_window, &width, &height);
        }

        m_device->waitIdle();

        cleanupSwapchain();

        createSwapchainAndRelatedThings();
    }
 
    static void framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/)
    {
        BofGame* app = static_cast<BofGame*>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* /*pUserData*/)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }



    void drawFrame()
    {
        Vector<vk::Fence> currentFences = { m_inFlightFences[m_currentFrame] };
        m_device->waitForFences(
            currentFences,
            VK_TRUE,
            std::numeric_limits<uint64_t>::max());

        m_device->resetFences(currentFences);


        const vk::ResultValue<uint32_t> acquireResultValue = m_device->acquireNextImageKHR(
            m_swapchain,
            std::numeric_limits<uint64_t>::max(),
            m_imageAvailableSemaphores[m_currentFrame],
            nullptr
        );

        if (acquireResultValue.result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapchain();
            return;
        }

        // at this point we want result to be success or suboptimal
        BOF_ASSERT(acquireResultValue.result == vk::Result::eSuccess ||
            acquireResultValue.result == vk::Result::eSuboptimalKHR,
            "can't acquire next image %s", vk::to_string(acquireResultValue.result).c_str());



        const uint32_t imageIndex = acquireResultValue.value;

        updateUniformBuffer(imageIndex);

        Vector<vk::SubmitInfo> submitInfos;
        vk::SubmitInfo submitInfo = {};
        Vector<vk::Semaphore> waitSemaphores = { m_imageAvailableSemaphores[m_currentFrame] };
        submitInfo.setWaitSemaphores(waitSemaphores);

        Vector<vk::PipelineStageFlags> waitDstStageMasks = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.setWaitDstStageMask(waitDstStageMasks);

        Vector<vk::CommandBuffer> commandBuffers = { m_commandBuffers[imageIndex] };
        submitInfo.setCommandBuffers(commandBuffers);

        Vector<vk::Semaphore> signalSemaphores = { m_renderFinishedSemaphores[m_currentFrame] };
        submitInfo.setSignalSemaphores(signalSemaphores);

        submitInfos.push_back(submitInfo);

        m_graphicsQueue.submit(submitInfos, m_inFlightFences[m_currentFrame]);


        vk::PresentInfoKHR presentInfo = {};
        presentInfo.setWaitSemaphores(signalSemaphores);

        Vector<vk::SwapchainKHR> swapchains = { m_swapchain };
        presentInfo.setSwapchains(swapchains);

        Vector<uint32_t> imageIndices = { imageIndex };
        presentInfo.setImageIndices(imageIndices);
        presentInfo.pResults = nullptr;

        const vk::Result presentResult = m_presentQueue.presentKHR(&presentInfo);



        if (presentResult == vk::Result::eErrorOutOfDateKHR ||
            presentResult == vk::Result::eSuboptimalKHR ||
            m_framebufferResized)
        {
            m_framebufferResized = false;
            recreateSwapchain();
            return;
        }
        else if (presentResult != vk::Result::eSuccess)
        {
            BOF_FAIL("can't present image");
        }


        m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
    }

    void updateUniformBuffer(uint32_t currentImage)
    {
        const double time = m_clock.GetTimeSecs();

        UniformBufferObject ubo{};
        static float radsPerSecond = 0.5f;
        const double angle = time * radsPerSecond;
        ubo.model = glm::rotate(
            glm::mat4(1.0f), (float)angle,
            glm::vec3(0.0f, 0.0f, 1.0f));

        ubo.view = glm::lookAt(
            glm::vec3(1.5f, 1.5f, 0.8f),
            glm::vec3(0.0f, 0.0f, 0.2f),
            glm::vec3(0.0f, 0.0f, 1.0f));

        const float aspectRatio = m_swapchainExtent.width / (float)m_swapchainExtent.height;

        ubo.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        const vk::DeviceSize memoryOffset = 0;
        void* data = m_device->mapMemory(m_uniformBuffersMemory[currentImage], memoryOffset, sizeof(ubo), vk::MemoryMapFlags{});
        memcpy(data, &ubo, sizeof(ubo));
        m_device->unmapMemory(m_uniformBuffersMemory[currentImage]);
    }

    void showFps()
    {
        const double thisTimeMS = m_clock.GetTimeMillis();

        const double frameTime = thisTimeMS - m_frameStartTimeMS;
        m_frameTimes.Push(frameTime);
        if (m_showFps &&
            thisTimeMS - m_timeOfLastFpsDisplayMS > 1000.0f)
        {
            m_timeOfLastFpsDisplayMS = thisTimeMS;

            double totalFrameTimesMS = 0.0;
            double worstFrameTimeMS = 0.0f;
            for (int64_t i = m_frameTimes.GetMinimumAvailableIndex(); i <= m_frameTimes.GetMaximumAvailableIndex(); i++)
            {
                const double ft = m_frameTimes.Get(i);;
                totalFrameTimesMS += ft;
                if (ft > worstFrameTimeMS)
                {
                    worstFrameTimeMS = ft;
                }
            }
            const double averageFrameTimesMS = totalFrameTimesMS / m_frameTimes.GetSize();
            const int averageFPS = (int)(1000 / averageFrameTimesMS);
            std::cout << "mean frametime: " << averageFrameTimesMS << " ms (" << averageFPS << " fps)\n";
            std::cout << "worst frametime: " << worstFrameTimeMS << " ms\n";
        }

        m_frameStartTimeMS = m_clock.GetTimeMillis();
    }

    void mainLoop()
    {
        m_frameStartTimeMS = m_clock.GetTimeMillis();
        m_timeOfLastFpsDisplayMS = m_frameStartTimeMS;

        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            drawFrame();

            showFps();
        }
        m_device->waitIdle();
    }



    Vector<const char*> m_validationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    Vector<const char*> m_deviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    bool m_enableValidationLayers = true;


    GLFWwindow* m_window = nullptr;

    vk::UniqueInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugCallback = nullptr;
    vk::SurfaceKHR m_surface;

    vk::PhysicalDevice m_physicalDevice;
    vk::UniqueDevice m_device;

    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;

    vk::SwapchainKHR m_swapchain = nullptr;
    Vector<vk::Image> m_swapchainImages;
    vk::Format m_swapchainImageFormat = vk::Format::eUndefined;
    vk::Extent2D m_swapchainExtent;

    Vector<vk::ImageView> m_swapchainImageViews;

    vk::RenderPass m_renderPass;
    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::PipelineLayout m_pipelineLayout;

    vk::Pipeline m_graphicsPipeline;

    Vector<vk::Framebuffer> m_swapchainFramebuffers;

    vk::CommandPool m_commandPool;
    std::vector<vk::CommandBuffer, std::allocator<vk::CommandBuffer>> m_commandBuffers;

    Vector<vk::Semaphore> m_imageAvailableSemaphores;
    Vector<vk::Semaphore> m_renderFinishedSemaphores;
    Vector<vk::Fence> m_inFlightFences;
    const size_t m_maxFramesInFlight = 2;
    size_t m_currentFrame = 0;

    bool m_framebufferResized = false;

    Vector<Vertex> m_vertices;    
    vk::Buffer m_vertexBuffer;
    vk::DeviceMemory m_vertexBufferMemory;

    Vector<uint32_t> m_indices;
    vk::Buffer m_indexBuffer;
    vk::DeviceMemory m_indexBufferMemory;

    Vector<vk::Buffer> m_uniformBuffers;
    Vector<vk::DeviceMemory> m_uniformBuffersMemory;

    vk::DescriptorPool m_descriptorPool;
    Vector<vk::DescriptorSet> m_descriptorSets;

    uint32_t m_mipLevels = 1;
    vk::Image m_textureImage;
    vk::DeviceMemory m_textureImageMemory;
    vk::ImageView m_textureImageView;
    vk::Sampler m_textureSampler;

    vk::Image m_depthImage;
    vk::DeviceMemory m_depthImageMemory;
    vk::ImageView m_depthImageView;

    vk::Image m_colorImage;
    vk::DeviceMemory m_colorImageMemory;
    vk::ImageView m_colorImageView;

    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;

    static constexpr int32_t m_colorImageAttachmentIndex = 0;
    static constexpr int32_t m_depthImageAttachmentIndex = 1;
    static constexpr int32_t m_resolveAttachmentIndex = 2;
    static constexpr int32_t m_attachmentCount = 3;

    Bof::SimpleClock m_clock;

    bool m_showFps = false;
    RingBuffer<double, 100> m_frameTimes;

    double m_frameStartTimeMS = 0.0;
    double m_timeOfLastFpsDisplayMS = 0.0;

    //const Vector<Vertex> m_vertices =
    //{
    //    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    //    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    //    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    //    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    //    { {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    //    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    //    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    //    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    //};

    //const Vector<uint16_t> m_indices =
    //{
    //    0, 1, 2, 2, 3, 0,
    //    0 + 4, 1 + 4, 2 + 4, 2 + 4, 3 + 4, 0 + 4
    //};


    const std::string m_modelPath = "data/models/viking_room.obj";
    const std::string m_texturePath = "data/textures/viking_room.png";


};






int main(int /*argc*/, char** /*argv*/)
{
    Bof::Log::Init();

    BofGame* app = new BofGame();

    app->run();

    delete app;
}


