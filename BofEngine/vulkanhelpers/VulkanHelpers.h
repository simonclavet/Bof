#pragma once

#include "core/core.h"
#include "utils/Timer.h"

// sorry.. can't deal with std:: for such basic things, but 'using namespace std' is not a good idea.
template<typename T>
using Vector = std::vector<T>;
using String = std::string;


// check a result from an expression with no return value except the vkresult, assert if error. Will evaluate the expression even in retail
#define CHECK_VKRESULT(result)\
    do { [[maybe_unused]]vk::Result r = (vk::Result)(result); \
    BOF_ASSERT_MSG(r == vk::Result::eSuccess, "bad vkresult: %s", vk::to_string(r).c_str());} while(0)

// check the result from a ResultValue pair, result and return the value. In retail will just return the value
template<typename VALUE>
VALUE checkVkResult(const vk::ResultValue<VALUE>& resultValue)
{
    BOF_ASSERT_MSG(resultValue.result == vk::Result::eSuccess, "bad vkresult: %s", vk::to_string(resultValue.result).c_str());
    return resultValue.value;
}



class VulkanHelpers
{
public:

    static VkResult createDebugUtilsMessengerEXT(
        const vk::UniqueInstance& instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        const auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(*instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void destroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static bool isDeviceSuitable(
        const vk::PhysicalDevice& physicalDevice, 
        const vk::SurfaceKHR& surface,
        const Vector<const char*>& deviceExtensions)
    {
        uint32_t resultGraphicsFamily = UINT32_MAX;
        uint32_t resultPresentFamily = UINT32_MAX;

        const bool foundGoodQueueFamilies = findQueueFamilies(physicalDevice, surface, resultGraphicsFamily, resultPresentFamily);
        if (!foundGoodQueueFamilies)
        {
            return false;
        }

        const bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice, deviceExtensions);
        if (!extensionsSupported)
        {
            return false;
        }

        const vk::PhysicalDeviceFeatures supportedFeatures = physicalDevice.getFeatures();
        if (!supportedFeatures.samplerAnisotropy)
        {
            return false;
        }

        const Vector<vk::SurfaceFormatKHR> formats = checkVkResult(physicalDevice.getSurfaceFormatsKHR(surface));
        if (formats.empty())
        {
            return false;
        }

        const Vector<vk::PresentModeKHR> presentModes = checkVkResult(physicalDevice.getSurfacePresentModesKHR(surface));
        if (presentModes.empty())
        {
            return false;
        }

        return true;
    }

    static vk::Format findSupportedFormat(
        const Vector<vk::Format>& candidates,
        const vk::ImageTiling& tiling,
        const vk::FormatFeatureFlags& features,
        const vk::PhysicalDevice& physicalDevice)
    {
        for (const vk::Format& format : candidates)
        {
            const vk::FormatProperties props = physicalDevice.getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear &&
                (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            if (tiling == vk::ImageTiling::eOptimal &&
                (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }
        BOF_FAIL("unsupported format");
        return vk::Format{};
    }

    static vk::Format findDepthFormat(const vk::PhysicalDevice& physicalDevice)
    {
        return findSupportedFormat(
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment,
            physicalDevice);
    }

    static bool hasStencilComponent(const vk::Format& format)
    {
        return format == vk::Format::eD32SfloatS8Uint ||
            format == vk::Format::eD24UnormS8Uint;
    }

    static bool checkDeviceExtensionSupport(
        const vk::PhysicalDevice& physicalDevice,
        const Vector<const char*>& deviceExtensions)
    {
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        Vector<vk::ExtensionProperties> extensions = physicalDevice.enumerateDeviceExtensionProperties();
        for (const vk::ExtensionProperties& extension : extensions)
        {
            //std::cout << extension.extensionName << std::endl;
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    static bool findQueueFamilies(
        const vk::PhysicalDevice& physicalDevice, 
        const vk::SurfaceKHR& surface,
        // output
        uint32_t& resultGraphicsFamily, 
        uint32_t& resultPresentFamily)
    {
        resultGraphicsFamily = UINT32_MAX;
        resultPresentFamily = UINT32_MAX;

        const Vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

        for (uint32_t i{0}; i < (uint32_t)queueFamilies.size(); i++)
        {
            const vk::QueueFamilyProperties& queueFamily = queueFamilies[i];

            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                resultGraphicsFamily = i;
            }

            if (physicalDevice.getSurfaceSupportKHR(i, surface))
            {
                resultPresentFamily = i;
            }

            if (resultGraphicsFamily != UINT32_MAX &&
                resultPresentFamily != UINT32_MAX)
            {
                return true;
            }
        }
        return false;
    }


    static Vector<const char*> getGLFWRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        const Vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        return extensions;
    }

    static bool checkLayerSupport(const Vector<const char*>& layersToCheck)
    {
        const Vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

        for (const char* layerName : layersToCheck)
        {
            bool layerFound = false;

            for (const vk::LayerProperties& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }


    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const Vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
        {
            return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
        }

        for (const vk::SurfaceFormatKHR& availableFormat : availableFormats)
        {
            if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
                availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    static vk::PresentModeKHR chooseSwapPresentMode(const Vector<vk::PresentModeKHR>& availablePresentModes)
    {
        vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;

        for (const vk::PresentModeKHR& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox)
            {
                return availablePresentMode;
            }
            else if (availablePresentMode == vk::PresentModeKHR::eImmediate)
            {
                bestMode = availablePresentMode;
            }
        }

        return bestMode;
    }

    static Vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            BOF_FAIL("failed to open file %s", filename.c_str());
        }

        const size_t fileSize = static_cast<size_t>(file.tellg());
        Vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);

        return buffer;
    }


    static vk::UniqueShaderModule createShaderModule(
        const vk::Device& device,
        const Vector<char>& code)
    {
        return device.createShaderModuleUnique({
            vk::ShaderModuleCreateFlags(),
            code.size(),
            reinterpret_cast<const uint32_t*>(code.data())
            });
    }

    static uint32_t findMemoryTypeIndex(
        uint32_t acceptableMemoryTypes,
        const vk::MemoryPropertyFlags& desiredProperties,
        const vk::PhysicalDevice& physicalDevice)
    {
        const vk::PhysicalDeviceMemoryProperties physicalMemoryProperties = physicalDevice.getMemoryProperties();

        for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < physicalMemoryProperties.memoryTypeCount; memoryTypeIndex++)
        {
            const bool isInAcceptableTypes = acceptableMemoryTypes & (1 << memoryTypeIndex);
            if (isInAcceptableTypes)
            {
                const vk::MemoryType& memoryType = physicalMemoryProperties.memoryTypes[memoryTypeIndex];
                const vk::MemoryPropertyFlags& memoryPropertyFlags = memoryType.propertyFlags;

                const vk::MemoryPropertyFlags memoryPropertiesAndedWithDesiredProperties = memoryPropertyFlags & desiredProperties;

                const bool hasAllDesiredProperties = memoryPropertiesAndedWithDesiredProperties == desiredProperties;

                if (hasAllDesiredProperties)
                {
                    return memoryTypeIndex;
                }
            }
        }
        BOF_FAIL("can't find suitable memory type");
        return 0;
    }

    static void createImageAndImageView(
        const vk::Extent2D& extent,
        const uint32_t& mipLevels,
        const vk::SampleCountFlagBits& numSamples,
        const vk::Format& format,
        const vk::ImageTiling& tiling,
        const vk::ImageUsageFlags& imageUsage,
        const vk::MemoryPropertyFlagBits& desiredMemoryProperties,
        const vk::ImageAspectFlags& aspectFlags,
        const vk::Device& device,
        const vma::Allocator& allocator,
        // output
        vk::Image& outputImage,
        vma::Allocation& outputImageAllocation,
        vk::ImageView& outputImageView)
    {
        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.setExtent({ extent.width, extent.height, 1 });
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = imageUsage;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.samples = numSamples;
        imageInfo.flags = vk::ImageCreateFlags{};

        vma::AllocationCreateInfo allocInfo{};
        allocInfo.usage = vma::MemoryUsage::eGpuOnly;
        allocInfo.requiredFlags = desiredMemoryProperties;
        allocInfo.preferredFlags = desiredMemoryProperties;
        allocInfo.flags = vma::AllocationCreateFlagBits::eMapped;

        CHECK_VKRESULT(allocator.createImage(
            &imageInfo,
            &allocInfo,
            &outputImage,
            &outputImageAllocation,
            nullptr));

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = outputImage;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        outputImageView = checkVkResult(device.createImageView(viewInfo));
    }



    static void createTextureImage(
        const String& filename,
        const vk::Format& format,
        const vk::Device& device,
        const vk::Queue& queue,
        const vk::CommandPool& commandPool,
        const vma::Allocator& allocator,
        // output
        vk::Image& outputImage,
        vma::Allocation& outputImageAllocation,
        uint32_t& outMipLevels)
    {
        int texWidth, texHeight, texChannels;

        stbi_uc* pixels = stbi_load(
            filename.c_str(),
            &texWidth, &texHeight,
            &texChannels,
            STBI_rgb_alpha);

        BOF_ASSERT_MSG(pixels != nullptr, "can't load %s", filename.c_str());

        createTextureImage(
            pixels,
            texWidth,
            texHeight,
            format,
            device,
            queue,
            commandPool,
            allocator,
            // output
            outputImage,
            outputImageAllocation,
            outMipLevels);

        stbi_image_free(pixels);
    }

    static void createTextureImage(
        const unsigned char* pixels,
        const int texWidth,
        const int texHeight,
        const vk::Format& format,
        const vk::Device& device,
        const vk::Queue& queue,
        const vk::CommandPool& commandPool,
        const vma::Allocator& allocator,
        // output
        vk::Image& outputImage,
        vma::Allocation& outputImageAllocation,
        uint32_t& outMipLevels)
    {
        outMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        const vk::DeviceSize imageSize = texWidth * texHeight * 4;

        vk::Buffer stagingBuffer;
        vma::Allocation stagingBufferAllocation;
        {
            vk::BufferCreateInfo bufferInfo{};
            bufferInfo.size = imageSize;
            bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
            vma::AllocationCreateInfo allocInfo{};
            allocInfo.usage = vma::MemoryUsage::eCpuToGpu;

            CHECK_VKRESULT(allocator.createBuffer(&bufferInfo, &allocInfo, &stagingBuffer, &stagingBufferAllocation, nullptr));
        }

        void* data = checkVkResult(allocator.mapMemory(stagingBufferAllocation));
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        allocator.unmapMemory(stagingBufferAllocation);


        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent = vk::Extent3D{ (uint32_t)texWidth, (uint32_t)texHeight, 1 };
        imageInfo.mipLevels = outMipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.flags = vk::ImageCreateFlags{};

        vma::AllocationCreateInfo allocInfo{};
        allocInfo.usage = vma::MemoryUsage::eGpuOnly;
        allocInfo.flags = vma::AllocationCreateFlagBits::eMapped;

        CHECK_VKRESULT(allocator.createImage(
            &imageInfo,
            &allocInfo,
            &outputImage,
            &outputImageAllocation,
            nullptr));

        transitionImageLayout(
            outputImage,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            outMipLevels,
            device, queue, commandPool);

        copyBufferToImage(
            stagingBuffer,
            outputImage,
            texWidth, texHeight,
            device, queue, commandPool);

        allocator.destroyBuffer(stagingBuffer, stagingBufferAllocation);


        generateMipmaps(
            outputImage,
            texWidth,
            texHeight,
            outMipLevels,
            device, queue, commandPool);

    }






    static void generateMipmaps(
        vk::Image image,
        uint32_t texWidth,
        uint32_t texHeight,
        uint32_t mipLevels,
        const vk::Device& device,
        const vk::Queue& queue,
        const vk::CommandPool& commandPool)

    {
        const vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        vk::ImageMemoryBarrier barrier{};
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i{1}; i < mipLevels; ++i)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eTransfer,
                vk::DependencyFlagBits{},
                0, nullptr,
                0, nullptr,
                1, &barrier);

            vk::ImageBlit blit{};
            blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
            blit.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = vk::Offset3D{ 0, 0,0, };
            blit.dstOffsets[1] = vk::Offset3D{
                mipWidth > 1 ? mipWidth / 2 : 1,
                mipHeight > 1 ? mipHeight / 2 : 1,
                1 };
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            commandBuffer.blitImage(
                image,
                vk::ImageLayout::eTransferSrcOptimal,
                image,
                vk::ImageLayout::eTransferDstOptimal,
                1, &blit,
                vk::Filter::eLinear);

            barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::DependencyFlagBits{},
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1)
            {
                mipWidth /= 2;
            }
            if (mipHeight > 1)
            {
                mipHeight /= 2;
            }
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlagBits{},
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSingleTimeCommands(commandBuffer, queue, device, commandPool);
    }


    static void transitionImageLayout(
        vk::Image image,
        vk::Format format,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        uint32_t mipLevels,
        const vk::Device& device,
        const vk::Queue& queue,
        const vk::CommandPool& commandPool)
    {
        const vk::CommandBuffer commandBuffer =
            beginSingleTimeCommands(device, commandPool);

        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        if (newLayout == vk::ImageLayout::eDepthAttachmentOptimal)
        {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

            if (hasStencilComponent(format))
            {
                barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;


        barrier.srcAccessMask = vk::AccessFlags{};
        barrier.dstAccessMask = vk::AccessFlags{};

        vk::PipelineStageFlags sourceStage{};
        vk::PipelineStageFlags destinationStage{};

        if (oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlags{};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
            newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else if (oldLayout == vk::ImageLayout::eUndefined && 
            newLayout == vk::ImageLayout::eDepthAttachmentOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits{};
            barrier.dstAccessMask =
                vk::AccessFlagBits::eDepthStencilAttachmentRead |
                vk::AccessFlagBits::eDepthStencilAttachmentWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        }
        else
        {
            BOF_FAIL("unsupported layout transition");
        }

        commandBuffer.pipelineBarrier(
            sourceStage,
            destinationStage,
            vk::DependencyFlags{},
            0, nullptr,
            0, nullptr,
            1, &barrier);

        endSingleTimeCommands(commandBuffer, queue, device, commandPool);
    }


    static vk::CommandBuffer beginSingleTimeCommands(
        const vk::Device& device,
        const vk::CommandPool& commandPool)

    {
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        const vk::CommandBuffer commandBuffer = checkVkResult(device.allocateCommandBuffers(allocInfo))[0];

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    static void endSingleTimeCommands(
        const vk::CommandBuffer& commandBuffer,
        const vk::Queue& graphicsQueue,
        const vk::Device& device,
        const vk::CommandPool& commandPool)
    {
        commandBuffer.end();

        vk::SubmitInfo submitInfo = {};
        Vector<vk::CommandBuffer> commandBuffers = { commandBuffer };
        submitInfo.setCommandBuffers(commandBuffers);

        Vector<vk::SubmitInfo> submitInfos = { submitInfo };

        graphicsQueue.submit(submitInfos, nullptr);
        graphicsQueue.waitIdle();

        device.freeCommandBuffers(commandPool, commandBuffers);
    }

    static void copyBuffer(
        vk::Buffer srcBuffer,
        vk::Buffer dstBuffer, // output
        vk::DeviceSize size,
        const vk::Queue& graphicsQueue,
        const vk::Device& device,
        const vk::CommandPool& commandPool)
    {
        const vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        Vector<vk::BufferCopy> copyRegions;
        vk::BufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        copyRegions.push_back(copyRegion);

        commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegions);

        endSingleTimeCommands(commandBuffer, graphicsQueue, device, commandPool);
    }

    static void copyBufferToImage(
        const vk::Buffer buffer,
        const vk::Image image,
        const uint32_t width,
        const uint32_t height,
        const vk::Device& device,
        const vk::Queue& graphicsQueue,
        const vk::CommandPool& commandPool)
    {
        const vk::CommandBuffer commandBuffer =
            beginSingleTimeCommands(device, commandPool);

        vk::BufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = vk::Offset3D{ 0, 0 };
        region.imageExtent = vk::Extent3D{ width, height, 1 };

        Vector<vk::BufferImageCopy> regions = { region };
        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, regions);

        endSingleTimeCommands(commandBuffer, graphicsQueue, device, commandPool);
    }


    template<typename T>
    static void createAndFillBufferWithArrayOld(
        const T& sourceDataArray,
        vk::BufferUsageFlagBits usageBits,
        const vk::Device& device,
        const vk::Queue& queue,
        const vk::CommandPool& commandPool,
        vma::Allocator allocator,
        vk::Buffer& buffer,
        vma::Allocation& bufferAllocation)
    {
        BOF_ASSERT_MSG(sourceDataArray.size() > 0, "no data");
        const vk::DeviceSize bufferSize = sizeof(sourceDataArray[0]) * sourceDataArray.size();

        vk::Buffer stagingBuffer;
        vma::Allocation stagingBufferAllocation;
        {
            vk::BufferCreateInfo bufferInfo{};
            bufferInfo.size = bufferSize;
            bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
            vma::AllocationCreateInfo allocInfo{};
            allocInfo.usage = vma::MemoryUsage::eCpuOnly;

            CHECK_VKRESULT(allocator.createBuffer(&bufferInfo, &allocInfo, &stagingBuffer, &stagingBufferAllocation, nullptr));
        }

        void* data = checkVkResult(allocator.mapMemory(stagingBufferAllocation));
        memcpy(data, sourceDataArray.data(), static_cast<size_t>(bufferSize));
        allocator.unmapMemory(stagingBufferAllocation);

        {
            vk::BufferCreateInfo bufferInfo{};
            bufferInfo.size = bufferSize;
            bufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst | usageBits;
            vma::AllocationCreateInfo allocInfo{};
            allocInfo.usage = vma::MemoryUsage::eGpuOnly;

            CHECK_VKRESULT(allocator.createBuffer(&bufferInfo, &allocInfo, &buffer, &bufferAllocation, nullptr));
        }

        const vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        Vector<vk::BufferCopy> copyRegions;
        vk::BufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = bufferSize;
        copyRegions.push_back(copyRegion);

        commandBuffer.copyBuffer(stagingBuffer, buffer, copyRegions);

        endSingleTimeCommands(commandBuffer, queue, device, commandPool);


        allocator.destroyBuffer(stagingBuffer, stagingBufferAllocation);
    }


    static void createAndFillBuffer(
        const void* sourceData, // if nullptr don't copy data
        VkDeviceSize bufferSize,
        vk::BufferUsageFlagBits bufferUsageBits,
        vma::MemoryUsage memoryUsage,
        const vk::Device& device,
        const vk::Queue& queue,
        const vk::CommandPool& commandPool,
        vma::Allocator allocator,
        // output
        vk::Buffer& buffer,
        vma::Allocation& bufferAllocation)
    {
        if (memoryUsage == vma::MemoryUsage::eCpuOnly ||
            memoryUsage == vma::MemoryUsage::eCpuToGpu) // uniforms that are set every frame from cpu, not transfered, but still read by gpu.
        {
            // just create buffer and allocate memory. Then map and copy data.

            vk::BufferCreateInfo bufferInfo{};
            bufferInfo.size = bufferSize;
            bufferInfo.usage = bufferUsageBits;

            vma::AllocationCreateInfo allocInfo{};
            allocInfo.usage = memoryUsage;

            CHECK_VKRESULT(allocator.createBuffer(&bufferInfo, &allocInfo, &buffer, &bufferAllocation, nullptr));

            if (sourceData != nullptr)
            {
                void* mappedData = checkVkResult(allocator.mapMemory(bufferAllocation));

                memcpy(
                    /*dest*/mappedData,
                    /*source*/sourceData,
                    static_cast<size_t>(bufferSize));

                allocator.unmapMemory(bufferAllocation);
            }
        }
        else if (memoryUsage == vma::MemoryUsage::eGpuOnly)
        {
            // create the actual buffer and allocation on gpu
            {
                vk::BufferCreateInfo bufferInfo{};
                bufferInfo.size = bufferSize;
                bufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst | bufferUsageBits; // transfer source, and actual usage

                vma::AllocationCreateInfo allocInfo{};
                allocInfo.usage = memoryUsage;

                CHECK_VKRESULT(allocator.createBuffer(&bufferInfo, &allocInfo, &buffer, &bufferAllocation, nullptr));
            }

            if (sourceData != nullptr)
            {
                // need to make a staging buffer, copy to it, then and transfer with a command

                vk::Buffer stagingBuffer;
                vma::Allocation stagingBufferAllocation;
                {
                    vk::BufferCreateInfo bufferInfo{};
                    bufferInfo.size = bufferSize;
                    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc; // transfer source

                    vma::AllocationCreateInfo allocInfo{};
                    allocInfo.usage = vma::MemoryUsage::eCpuOnly; // correct type for a staging buffer

                    CHECK_VKRESULT(allocator.createBuffer(&bufferInfo, &allocInfo, &stagingBuffer, &stagingBufferAllocation, nullptr));
                }
                // copy data to staging buffer
                void* mappedData = checkVkResult(allocator.mapMemory(stagingBufferAllocation));
                memcpy(
                    /*dest*/mappedData,
                    /*source*/sourceData,
                    static_cast<size_t>(bufferSize));
                allocator.unmapMemory(stagingBufferAllocation);


                // transfer command
                const vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

                Vector<vk::BufferCopy> copyRegions;
                vk::BufferCopy copyRegion = {};
                copyRegion.srcOffset = 0;
                copyRegion.dstOffset = 0;
                copyRegion.size = bufferSize;
                copyRegions.push_back(copyRegion);

                // transfer staging buffer to actual buffer on gpu
                commandBuffer.copyBuffer(
                    /*source*/stagingBuffer,
                    /*dest*/buffer,
                    copyRegions);

                endSingleTimeCommands(commandBuffer, queue, device, commandPool);
                // this waits for completion.

                // no need for that anymore
                allocator.destroyBuffer(stagingBuffer, stagingBufferAllocation);
            }
        }
        else
        {
            BOF_FAIL("unsupported memory usage, for now");
        }
    }

    static vk::SampleCountFlagBits getMaxUsableSampleCount(const vk::PhysicalDevice& physicalDevice)
    {
        const vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();

        const vk::SampleCountFlags counts =
            physicalDeviceProperties.limits.framebufferColorSampleCounts &
            physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
        if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
        if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
        if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
        if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
        if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }
        return vk::SampleCountFlagBits::e1;
    }

};


//
//class VulkanDevice
//{
//public:
//    vk::Instance m_instance;
//    vk::PhysicalDevice m_physicalDevice;
//    vk::Device m_device;
//    vk::PhysicalDeviceProperties m_physicalDeviceProperties;
//    vk::PhysicalDeviceFeatures m_physicalDeviceFeatures;
//    vk::PhysicalDeviceFeatures m_enabledPhysicalDeviceFeatures;
//    vk::PhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties;
//    std::vector<vk::QueueFamilyProperties> m_queueFamilyProperties;
//    std::vector<std::string> m_supportedExtensions;
//    vk::CommandPool m_commandPool = nullptr;
//    
//    struct
//    {
//        uint32_t graphics;
//        uint32_t compute;
//        uint32_t transfer;
//    } m_queueFamilyIndices;
//
//    VulkanDevice()
//    {
//
//    }
//
//};

namespace bofgltf
{
    enum FileLoadingFlags
    {
        None = 0x00000000,
        PreTransformVertices = 0x00000001,
        PreMultiplyVertexColors = 0x00000002,
        FlipY = 0x00000004,
        DontLoadImages = 0x00000008
    };


    bool loadImageDataFuncEmpty(
        tinygltf::Image*,// image,
        const int,// imageIndex,
        std::string*,// error,
        std::string*,// warning,
        int,// req_width,
        int,// req_height,
        const unsigned char*,// bytes,
        int,// size,
        void*)// userData)
    {
        return true;
    }

    bool loadImageDataFunc(
        tinygltf::Image* image,
        const int imageIndex,
        std::string* error,
        std::string* warning,
        int req_width,
        int req_height,
        const unsigned char* bytes,
        int size,
        void* userData)
    {
        if (image->uri.find_last_of(".") != std::string::npos)
        {
            if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx")
            {
                return true;
            }
        }

        return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
    }

    struct Vertex
    {

    };
    
    struct Texture
    {
        vk::Device m_device;
        vma::Allocator m_allocator;

        vk::Format m_format = vk::Format::eUndefined;
        vk::Image m_image = nullptr;
        vk::ImageLayout m_imageLayout = vk::ImageLayout::eUndefined;
        vma::Allocation m_imageAllocation = nullptr;
        vk::ImageView m_imageView = nullptr;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_mipLevels = 0;
        //uint32_t m_layerCount;
        vk::DescriptorImageInfo m_descriptor{};
        vk::Sampler m_sampler{};


        void fromGltfImage(
            const tinygltf::Image& gltfImage,
            const String& path,
            const vk::Device& device,
            const vk::Queue& transferQueue,
            const vk::CommandPool& commandPool,
            const vma::Allocator& allocator)
        {
            UNUSED(path);

            m_device = device;
            m_allocator = allocator;

            // assume not ktx;
            //bool isKtx = false;

            // maybe not const because might have to delete
            unsigned char* buffer = const_cast<unsigned char*>(&gltfImage.image[0]);
            //VkDeviceSize bufferSize = gltfImage.image.size();
            //bool deleteBuffer = false;

            BOF_ASSERT(gltfImage.component == 4);
            //if (gltfImage.component == 3)
            //{
            //    bufferSize = gltfImage.width * gltfImage.height * 4;
            //    buffer = new unsigned char[bufferSize];
            //    unsigned char* rgba = buffer;
            //    unsigned char* rgb = const_cast<unsigned char*>(&gltfImage.image[0]);
            //    for (size_t i = 0; i < gltfImage.width * gltfImage.height; ++i)
            //    {
            //        for (int32_t j = 0; j < 3; ++j)
            //        {
            //            rgba[j] = rgb[j];
            //        }
            //        rgba += 4;
            //        rgb += 3;
            //    }
            //    deleteBuffer = true;
            //}
 
            m_format = vk::Format::eR8G8B8A8Unorm;



            m_width = gltfImage.width;
            m_height = gltfImage.height;

            // todo: check format properties against physical device...
            VulkanHelpers::createTextureImage(
                buffer,
                (int)m_width,
                (int)m_height,
                m_format,
                device,
                transferQueue,
                commandPool,
                allocator,
                // output
                m_image,
                m_imageAllocation,
                m_mipLevels);



            vk::SamplerCreateInfo samplerInfo{};
            samplerInfo.magFilter = vk::Filter::eLinear;
            samplerInfo.minFilter = vk::Filter::eLinear;
            samplerInfo.addressModeU = vk::SamplerAddressMode::eMirroredRepeat;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eMirroredRepeat;
            samplerInfo.addressModeW = vk::SamplerAddressMode::eMirroredRepeat;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
            //samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
            samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = vk::CompareOp::eNever;
            samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(m_mipLevels);

            m_sampler = checkVkResult(device.createSampler(samplerInfo));



            vk::ImageViewCreateInfo viewInfo{};
            viewInfo.image = m_image;
            viewInfo.viewType = vk::ImageViewType::e2D;
            viewInfo.format = m_format;
            viewInfo.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
            viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = m_mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            m_imageView = checkVkResult(device.createImageView(viewInfo));

            m_imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

            m_descriptor.sampler = m_sampler;
            m_descriptor.imageView = m_imageView;
            m_descriptor.imageLayout = m_imageLayout;

        }


        void destroy()
        {
            m_allocator.destroyImage(m_image, m_imageAllocation);
            m_image = nullptr;
            m_imageAllocation = nullptr;

            m_device.destroyImageView(m_imageView);
            m_device.destroySampler(m_sampler);
        }
    };







    struct Material
    {
        enum class AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
        AlphaMode m_alphaMode = AlphaMode::ALPHAMODE_OPAQUE;
        float m_alphaCutoff = 1.0f;
        float m_metallicFactor = 1.0f;
        float m_roughnessFactor = 1.0f;
        glm::vec4 m_baseColorFactor = glm::vec4(1.0f);
        Texture* m_baseColorTexture = nullptr;
        Texture* m_metallicRoughnessTexture = nullptr;
        Texture* m_normalTexture = nullptr;
        Texture* m_occlusionTexture = nullptr;
        Texture* m_emissiveTexture = nullptr;
        Texture* m_specularGlossinessTexture = nullptr;
        Texture* m_diffuseTexture = nullptr;
    };

    struct Primitive
    {
        
    };

    struct Mesh
    {
        struct UniformBuffer
        {
            vk::Buffer m_buffer;
            vma::Allocation m_allocation;
            vk::DescriptorBufferInfo m_descriptor{};
            vk::DescriptorSet m_descriptorSet = nullptr;
            void* m_mappedMemory = nullptr;
        };

        struct UniformBlock
        {
            glm::mat4 m_matrix;
            glm::mat4 m_jointMatrix[64]{};
            float m_jointCount{ 0 };
        };


        vk::Device m_device = nullptr;
        vma::Allocator m_allocator = nullptr;

        UniformBuffer m_uniformBuffer{};
        UniformBlock m_uniformBlock{};


        Vector<Primitive*> m_primitives;
        String m_name;

        void init(
            vk::Device device,
            vk::Queue queue,
            vk::CommandPool commandPool,
            vma::Allocator allocator,
            glm::mat4 matrix)
        {
            m_device = device;
            m_allocator = allocator;
            m_uniformBlock.m_matrix = matrix;

            VulkanHelpers::createAndFillBuffer(
                &m_uniformBlock,
                sizeof(m_uniformBlock),
                vk::BufferUsageFlagBits::eUniformBuffer,
                vma::MemoryUsage::eCpuToGpu,
                device,
                queue,
                commandPool,
                allocator,
                m_uniformBuffer.m_buffer,
                m_uniformBuffer.m_allocation);

            m_uniformBuffer.m_mappedMemory = checkVkResult(allocator.mapMemory(m_uniformBuffer.m_allocation));

            m_uniformBuffer.m_descriptor = vk::DescriptorBufferInfo{};
            m_uniformBuffer.m_descriptor.buffer = m_uniformBuffer.m_buffer;
            m_uniformBuffer.m_descriptor.offset = 0;
            m_uniformBuffer.m_descriptor.range = sizeof(m_uniformBlock);

        }

        void destroy()
        {
            m_allocator.unmapMemory(m_uniformBuffer.m_allocation);
            m_allocator.destroyBuffer(m_uniformBuffer.m_buffer, m_uniformBuffer.m_allocation);
            m_uniformBuffer.m_buffer = nullptr;
            m_uniformBuffer.m_allocation = nullptr;
        }
    };

    struct Skin
    {
        
    };


    struct Node
    {
        glm::mat4 m_localMatrix{};
        glm::mat4 m_matrix{};

        glm::quat m_rotation{};
        glm::vec3 m_translation{};
        glm::vec3 m_scale{ 1.0f };

        int m_parentIndex = -1;
        uint32_t m_index;
        Vector<int> m_childrenNodeIndices;
        Mesh m_mesh; // should be id in model meshes
        Skin* m_skin; 
        int32_t m_skinIndex = -1;

        String m_name;
        

        glm::mat4 getMatrix() { return glm::mat4{}; }
        void update(){}

        void destroy()
        {
            m_mesh.destroy();            
        }
    };




    struct Model
    {
        vk::Device m_device;
        vk::Queue m_queue;
        vk::CommandPool m_commandPool;
        vma::Allocator m_allocator;

        Vector<Texture> m_textures;
        Vector<Material> m_materials;

        Vector<Node> m_linearNodes;

        // root nodes
        Vector<int> m_childrenNodeIndices; 

        Texture m_emptyTexture;

       

        void loadFromFile(
            String filename,
            const vk::Device& device,
            vk::Queue& transferQueue,
            vk::CommandPool& commandPool,
            vma::Allocator& allocator,
            uint32_t fileLoadingFlags = FileLoadingFlags::None,
            float scale = 1.0f)
        {
            m_device = device;
            m_queue = transferQueue;
            m_commandPool = commandPool;
            m_allocator = allocator;

            UNUSED(scale);
            PROFILE(ModelLoadFromFile);

            tinygltf::Model gltfModel;
            tinygltf::TinyGLTF gltfContext;

            if (fileLoadingFlags & FileLoadingFlags::DontLoadImages)
            {
                gltfContext.SetImageLoader(loadImageDataFuncEmpty, nullptr);
            }
            else
            {
                gltfContext.SetImageLoader(loadImageDataFunc, nullptr);
            }

            size_t pos = filename.find_last_of('/');
            String path = filename.substr(0, pos);

            String error, warning;


            bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);
            BOF_ASSERT_MSG(fileLoaded, "can't load file %s", filename.c_str());

            if (!(fileLoadingFlags & FileLoadingFlags::DontLoadImages))
            {
                for (const tinygltf::Image& image : gltfModel.images)
                {
                    bofgltf::Texture& texture = m_textures.emplace_back(bofgltf::Texture{});
                    texture.fromGltfImage(image, path, m_device, transferQueue, commandPool, allocator);
                }
            }






            for (const tinygltf::Material& mat : gltfModel.materials)
            {
                bofgltf::Material& material = m_materials.emplace_back(bofgltf::Material{});

                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.values.find("baseColorTexture");
                    if (foundParam != mat.values.end())
                    {
                        const int textureIndex = foundParam->second.TextureIndex();
                        const tinygltf::Texture& tex = gltfModel.textures[textureIndex];
                        const int textureIndexInMyArray = tex.source;
                        bofgltf::Texture* texture = &m_textures[textureIndexInMyArray];
                        material.m_baseColorTexture = texture;
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.values.find("metallicRoughnessTexture");
                    if (foundParam != mat.values.end())
                    {
                        const int textureIndex = foundParam->second.TextureIndex();
                        const tinygltf::Texture& tex = gltfModel.textures[textureIndex];
                        const int textureIndexInMyArray = tex.source;
                        bofgltf::Texture* texture = &m_textures[textureIndexInMyArray];
                        material.m_metallicRoughnessTexture = texture;
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.values.find("roughnessFactor");
                    if (foundParam != mat.values.end())
                    {
                        material.m_roughnessFactor = static_cast<float>(foundParam->second.Factor());
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.values.find("metallicFactor");
                    if (foundParam != mat.values.end())
                    {
                        material.m_metallicFactor = static_cast<float>(foundParam->second.Factor());
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.values.find("baseColorFactor");
                    if (foundParam != mat.values.end())
                    {
                        material.m_baseColorFactor = glm::make_vec4(foundParam->second.ColorFactor().data());
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.additionalValues.find("normalTexture");
                    if (foundParam != mat.additionalValues.end())
                    {
                        const int textureIndex = foundParam->second.TextureIndex();
                        const tinygltf::Texture& tex = gltfModel.textures[textureIndex];
                        const int textureIndexInMyArray = tex.source;
                        bofgltf::Texture* texture = &m_textures[textureIndexInMyArray];
                        material.m_normalTexture = texture;
                    }
                    else
                    {
                        material.m_normalTexture = &m_emptyTexture;
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.additionalValues.find("emissiveTexture");
                    if (foundParam != mat.additionalValues.end())
                    {
                        const int textureIndex = foundParam->second.TextureIndex();
                        const tinygltf::Texture& tex = gltfModel.textures[textureIndex];
                        const int textureIndexInMyArray = tex.source;
                        bofgltf::Texture* texture = &m_textures[textureIndexInMyArray];
                        material.m_emissiveTexture = texture;
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.additionalValues.find("occlusionTexture");
                    if (foundParam != mat.additionalValues.end())
                    {
                        const int textureIndex = foundParam->second.TextureIndex();
                        const tinygltf::Texture& tex = gltfModel.textures[textureIndex];
                        const int textureIndexInMyArray = tex.source;
                        bofgltf::Texture* texture = &m_textures[textureIndexInMyArray];
                        material.m_occlusionTexture = texture;
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.additionalValues.find("alphaMode");
                    if (foundParam != mat.additionalValues.end())
                    {
                        const tinygltf::Parameter& param = foundParam->second;
                        if (param.string_value == "BLEND")
                        {
                            material.m_alphaMode = Material::AlphaMode::ALPHAMODE_BLEND;
                        }
                        else if (param.string_value == "MASK")
                        {
                            material.m_alphaMode = Material::AlphaMode::ALPHAMODE_MASK;
                        }
                    }
                }
                {
                    tinygltf::ParameterMap::const_iterator foundParam =
                        mat.additionalValues.find("alphaCutoff");
                    if (foundParam != mat.additionalValues.end())
                    {
                        material.m_alphaCutoff = static_cast<float>(foundParam->second.Factor());
                    }
                }
            }
            // default material
            m_materials.push_back(Material{});



            Vector<uint32_t> indexBuffer;
            Vector<Vertex> vertexBuffer;



            int defaultSceneIndex = gltfModel.defaultScene;
            if (defaultSceneIndex < 0)
            {
                defaultSceneIndex = 0;
            }
            const tinygltf::Scene& scene = gltfModel.scenes[defaultSceneIndex];

            const Vector<int>& sceneNodes = scene.nodes;

            m_linearNodes.resize(gltfModel.nodes.size());

            for (size_t i = 0; i < sceneNodes.size(); i++)
            {
                const int nodeIndex = sceneNodes[i];
                const tinygltf::Node& node = gltfModel.nodes[nodeIndex];

                loadNode(
                    -1, 
                    node,
                    nodeIndex,
                    gltfModel,
                    indexBuffer,
                    vertexBuffer);


            }





        }




        void loadNode(
            int parentIndex,
            const tinygltf::Node& node,
            const int nodeIndex,
            const tinygltf::Model& model,
            // to fill
            Vector<uint32_t>& indexBuffer,
            Vector<Vertex>& vertexBuffer)
        {
            UNUSED(model, indexBuffer, vertexBuffer);

            bofgltf::Node& newNode = m_linearNodes[nodeIndex];

            newNode.m_index = nodeIndex;
            newNode.m_parentIndex = parentIndex;
            newNode.m_name = node.name;
            newNode.m_skinIndex = node.skin;
            newNode.m_matrix = glm::mat4(1.0f);

            if (node.translation.size() == 3)
            {
                newNode.m_translation = glm::make_vec3(node.translation.data());
            }
            if (node.rotation.size() == 4)
            {
                newNode.m_rotation = glm::make_quat(node.rotation.data());
            }
            if (node.scale.size() == 3)
            {
                newNode.m_scale = glm::make_vec3(node.scale.data());
            }
            if (node.matrix.size() == 16)
            {
                newNode.m_matrix = glm::make_mat4x4(node.matrix.data());
            }

            for (int childNodeIndex : node.children)
            {
                const tinygltf::Node& childNode = model.nodes[childNodeIndex];
                loadNode(nodeIndex, childNode, childNodeIndex, model, indexBuffer, vertexBuffer);
            }

            if (node.mesh > -1)
            {
                const int meshIndex = node.mesh;
                const tinygltf::Mesh& tinygltfMesh = model.meshes[meshIndex];

                newNode.m_mesh.init(m_device, m_queue, m_commandPool, m_allocator, newNode.m_matrix);
                newNode.m_mesh.m_name = tinygltfMesh.name;
            }



            if (parentIndex != -1)
            {
                m_linearNodes[parentIndex].m_childrenNodeIndices.push_back(nodeIndex);
            }
            else
            {
                m_childrenNodeIndices.push_back(nodeIndex);
            }
        }


        void destroy()
        {
            for (Texture& texture : m_textures)
            {
                texture.destroy();
            }
            for (Node& node : m_linearNodes)
            {
                node.destroy();
            }
        }

    };



}
