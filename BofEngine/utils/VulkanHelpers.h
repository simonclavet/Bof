#pragma once

#include "core/core.h"
//#include "core/BofEngine.h"
//#include "utils/Timer.h"
//#include "utils/BofLog.h"
//
//#include "utils/GoodSave.h"
//#include "utils/HashSortedVector.h"
//#include "utils/RingBuffer.h"

//#include "utils/VulkanHelpers.h"


// sorry.. can't deal with std:: for such basic things, but 'using namespace std' is not a good idea.
// If you have problems with an already defined Vector or String, sue me.
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




// purelly functional library of little functions that should facilitate stuff.
// do not extend a function if you need to add too many arguments to it.
// Just copy paste its content instead. It's ok to copy paste stuff.
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


    static Vector<const char*> getRequiredExtensions(bool enableValidation)
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        Vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidation)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

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
        const vk::UniqueDevice& device,
        const Vector<char>& code)
    {
        return device->createShaderModuleUnique({
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

    static void createBuffer(
        const vk::DeviceSize size,
        const vk::BufferUsageFlags usage,
        const vk::MemoryPropertyFlags properties,
        const vk::PhysicalDevice& physicalDevice,
        const vk::UniqueDevice& device,
        // output
        vk::Buffer& outputBuffer,
        vk::DeviceMemory& outputMemory)
    {
        vk::BufferCreateInfo bufferInfo = {};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        outputBuffer = checkVkResult(device->createBuffer(bufferInfo, nullptr));

        const vk::MemoryRequirements memRequirements = device->getBufferMemoryRequirements(outputBuffer);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;


        const uint32_t acceptableMemoryTypes = memRequirements.memoryTypeBits;

        allocInfo.memoryTypeIndex = VulkanHelpers::findMemoryTypeIndex(
            acceptableMemoryTypes,
            properties,
            physicalDevice);

        outputMemory = checkVkResult(device->allocateMemory(allocInfo));

        const vk::DeviceSize memoryOffset = 0;
        const vk::Result result = device->bindBufferMemory(outputBuffer, outputMemory, memoryOffset);
        CHECK_VKRESULT(result);
    }

    static void createImage(
        const vk::Extent2D& extent,
        uint32_t mipLevels,
        const vk::SampleCountFlagBits numSamples,
        const vk::Format& format,
        const vk::ImageTiling& tiling,
        const vk::ImageUsageFlags& imageUsage,
        const vk::MemoryPropertyFlagBits& desiredMemoryProperties,
        const vk::PhysicalDevice& physicalDevice,
        const vk::UniqueDevice& device,
        // output
        vk::Image& outputImage,
        vk::DeviceMemory& outputImageMemory)
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

        outputImage = checkVkResult(device->createImage(imageInfo));

        const vk::MemoryRequirements memRequirements = device->getImageMemoryRequirements(outputImage);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanHelpers::findMemoryTypeIndex(
            memRequirements.memoryTypeBits,
            desiredMemoryProperties,
            physicalDevice);

        outputImageMemory = checkVkResult(device->allocateMemory(allocInfo));

        const vk::DeviceSize memoryOffset = 0;
        device->bindImageMemory(outputImage, outputImageMemory, memoryOffset);
    }


    static vk::ImageView createImageView(
        const vk::Image& image,
        const vk::Format& format,
        const vk::ImageAspectFlags& aspectFlags,
        const uint32_t& mipLevels,
        const vk::UniqueDevice& device)
    {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        return checkVkResult(device->createImageView(viewInfo));
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
        const vk::UniqueDevice& device,
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

        outputImageView = createImageView(
            outputImage,
            format,
            aspectFlags,
            mipLevels,
            device);
    }



    static void createTextureImage(
        const char* filename,
        const vk::PhysicalDevice& physicalDevice,
        const vk::UniqueDevice& device,
        const vk::Queue& queue,
        const vk::CommandPool& commandPool,
        // output
        vk::Image& outputImage,
        vk::DeviceMemory& outputImageMemory,
        uint32_t& outMipLevels)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(
            filename, &texWidth, &texHeight,
            &texChannels, STBI_rgb_alpha);
        BOF_ASSERT_MSG(pixels != nullptr, "can't load %s", filename);

        outMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        const vk::DeviceSize imageSize = texWidth * texHeight * 4;

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        createBuffer(
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            physicalDevice, device,
            stagingBuffer, stagingBufferMemory);

        const vk::DeviceSize memoryOffset = 0;
        void* data = checkVkResult(device->mapMemory(
            stagingBufferMemory,
            memoryOffset,
            imageSize,
            vk::MemoryMapFlags{}));

        memcpy(data, pixels, static_cast<size_t>(imageSize));

        device->unmapMemory(stagingBufferMemory);

        stbi_image_free(pixels);

        const vk::Extent2D texExtent{ static_cast<uint32_t>(texWidth),static_cast<uint32_t>(texHeight) };

        createImage(
            texExtent,
            outMipLevels,
            vk::SampleCountFlagBits::e1,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            physicalDevice, device,
            outputImage,
            outputImageMemory);

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

        //transitionImageLayout(
        //    outputImage,
        //    vk::Format::eR8G8B8A8Srgb,
        //    vk::ImageLayout::eTransferDstOptimal,
        //    vk::ImageLayout::eShaderReadOnlyOptimal,
        //    outMipLevels,
        //    device, queue, commandPool);

        device->destroyBuffer(stagingBuffer);
        device->freeMemory(stagingBufferMemory);

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
        const vk::UniqueDevice& device,
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
        const vk::UniqueDevice& device,
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
        const vk::UniqueDevice& device,
        const vk::CommandPool& commandPool)

    {
        vk::CommandBufferAllocateInfo allocInfo = {};
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        const vk::CommandBuffer commandBuffer = checkVkResult(device->allocateCommandBuffers(allocInfo))[0];

        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        commandBuffer.begin(beginInfo);

        return commandBuffer;
    }

    static void endSingleTimeCommands(
        const vk::CommandBuffer& commandBuffer,
        const vk::Queue& graphicsQueue,
        const vk::UniqueDevice& device,
        const vk::CommandPool& commandPool)
    {
        commandBuffer.end();

        vk::SubmitInfo submitInfo = {};
        Vector<vk::CommandBuffer> commandBuffers = { commandBuffer };
        submitInfo.setCommandBuffers(commandBuffers);

        Vector<vk::SubmitInfo> submitInfos = { submitInfo };

        graphicsQueue.submit(submitInfos, nullptr);
        graphicsQueue.waitIdle();

        device->freeCommandBuffers(commandPool, commandBuffers);
    }

    static void copyBuffer(
        vk::Buffer srcBuffer,
        vk::Buffer dstBuffer, // output
        vk::DeviceSize size,
        const vk::Queue& graphicsQueue,
        const vk::UniqueDevice& device,
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
        const vk::UniqueDevice& device,
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
    static void createAndFillBuffer(
        const T& sourceDataArray,
        vk::BufferUsageFlagBits usageBits,
        const vk::PhysicalDevice& physicalDevice,
        const vk::UniqueDevice& device,
        const vk::Queue& queue,
        const vk::CommandPool& commandPool,
        vk::Buffer& buffer,
        vk::DeviceMemory& bufferMemory)
    {
        BOF_ASSERT_MSG(sourceDataArray.size() > 0, "no data");
        const vk::DeviceSize bufferSize = sizeof(sourceDataArray[0]) * sourceDataArray.size();

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            physicalDevice,
            device,
            stagingBuffer,
            stagingBufferMemory);


        const vk::DeviceSize memoryOffset = 0;

        void* data = checkVkResult(device->mapMemory(
            stagingBufferMemory,
            memoryOffset,
            bufferSize,
            vk::MemoryMapFlags{}));

        memcpy(data, sourceDataArray.data(), static_cast<size_t>(bufferSize));

        device->unmapMemory(stagingBufferMemory);


        createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | usageBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            physicalDevice,
            device,
            buffer,
            bufferMemory
        );

        copyBuffer(
            stagingBuffer,
            buffer,
            bufferSize,
            queue,
            device,
            commandPool);

        device->destroyBuffer(stagingBuffer);
        device->freeMemory(stagingBufferMemory);
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





