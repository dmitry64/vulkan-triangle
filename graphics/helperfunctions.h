#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <cassert>
#include <fstream>
#include <iostream>
#include <vulkan/vulkan.hpp>

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_parameter_validation", "VK_LAYER_LUNARG_parameter_validation", "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_GOOGLE_threading", "VK_LAYER_LUNARG_core_validation" };

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() const
    {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

static bool checkValidationLayerSupport()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
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

static bool hasStencilComponent(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
        return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        } else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            bestMode = availablePresentMode;
        }
    }
    return bestMode;
}

static vk::Format findSupportedFormat(vk::PhysicalDevice& physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (vk::Format format : candidates) {
        vk::FormatProperties props;
        physicalDevice.getFormatProperties(format, &props);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    std::cerr << "Failed to find supported format!" << std::endl;
    return vk::Format();
}

static vk::Format findDepthFormat(vk::PhysicalDevice& physicalDevice)
{
    return findSupportedFormat(physicalDevice, { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: [" << filename << "]" << std::endl;
        std::abort();
    }

    auto fileSize = file.tellg();
    if (fileSize > 0) {
        std::vector<char> buffer(static_cast<size_t>(fileSize));
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    } else {
        std::cerr << "Empty file: " << filename << std::endl;
        std::abort();
    }

    return std::vector<char>();
}

static uint32_t findMemoryType(vk::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties;
    physicalDevice.getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    std::cerr << "Failed to find suitable memory type!" << std::endl;
    std::abort();
    return 0;
}

static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice& physicalDevice, vk::SurfaceKHR& surface)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    physicalDevice.getQueueFamilyProperties(&queueFamilyCount, nullptr);
    std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
    physicalDevice.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        vk::Bool32 presentSupport = false;
        physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface, &presentSupport);
        if (queueFamily.queueCount > 0 && presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

static vk::CommandBuffer beginSingleTimeCommands(vk::Device& device, vk::CommandPool& commandPool)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer newCommandBuffer;
    device.allocateCommandBuffers(&allocInfo, &newCommandBuffer);

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    newCommandBuffer.begin(&beginInfo);

    return newCommandBuffer;
}

static void endSingleTimeCommands(vk::Device& device, vk::Queue& graphicsQueue, vk::CommandPool& commandPool, vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    graphicsQueue.submit(1, &submitInfo, nullptr);
    graphicsQueue.waitIdle();
    device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

static void createShaderModule(vk::Device& device, const std::vector<char>& code, vk::ShaderModule& shaderModule)
{
    // assert(code.size() % 4 == 0);
    vk::ShaderModuleCreateInfo createInfo(vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t*>(code.data()));

    vk::Result res = device.createShaderModule(&createInfo, nullptr, &shaderModule);
    if (res != vk::Result::eSuccess) {
        std::cerr << "failed to create shader module! error:" << res << std::endl;
        std::abort();
    }
}

static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        vk::Extent2D actualExtent = { width, height };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

static SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice& device, vk::SurfaceKHR surface)
{
    SwapChainSupportDetails details;
    device.getSurfaceCapabilitiesKHR(surface, &details.capabilities);
    uint32_t formatCount = 0;
    device.getSurfaceFormatsKHR(surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        device.getSurfaceFormatsKHR(surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount = 0;
    device.getSurfacePresentModesKHR(surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        device.getSurfacePresentModesKHR(surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

static void createBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) {
        std::cerr << "Failed to create buffer!" << std::endl;
        std::abort();
    }

    vk::MemoryRequirements memRequirements;
    device.getBufferMemoryRequirements(buffer, &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (device.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate buffer memory!" << std::endl;
        std::abort();
    }
    device.bindBufferMemory(buffer, bufferMemory, 0);
}

static void copyBuffer(vk::Device& device, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);

    vk::CommandBuffer commandBuffer;
    vk::Result allocationResult = device.allocateCommandBuffers(&allocInfo, &commandBuffer);
    if (allocationResult != vk::Result::eSuccess) {
        std::cerr << "Allocation failed! code" << allocationResult << std::endl;
        std::abort();
    }

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    vk::BufferCopy copyRegion(0, 0, size);

    if (commandBuffer.begin(&beginInfo) == vk::Result::eSuccess) {
        commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
        commandBuffer.end();
    } else {
        std::cerr << "Failed to begin command buffer!" << std::endl;
        std::abort();
    }

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vk::FenceCreateInfo fenceCreateInfo = {};
    vk::Fence fence;
    vk::Result fenceCreateRes = device.createFence(&fenceCreateInfo, nullptr, &fence);
    if (fenceCreateRes != vk::Result::eSuccess) {
        std::cerr << "Failed to create fence! error:" << fenceCreateRes << std::endl;
        std::abort();
    }

    vk::Result submitRes = graphicsQueue.submit(1, &submitInfo, fence);
    if (submitRes != vk::Result::eSuccess) {
        std::cerr << "Graphics queue submit failed! error:" << submitRes << std::endl;
        std::abort();
    }
    vk::Result fenceWaitRes = device.waitForFences(1, &fence, VK_TRUE, 3 * 1000 * 1000);
    if (fenceWaitRes != vk::Result::eSuccess) {
        std::cerr << "Wait on fence failed! error:" << fenceWaitRes << std::endl;
        std::abort();
    }
    device.destroyFence(fence, nullptr);
    device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

#endif // HELPERFUNCTIONS_H
