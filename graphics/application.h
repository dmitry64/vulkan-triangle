#ifndef APPLICATION_INCONCE_H
#define APPLICATION_INCONCE_H

#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <set>

#include "window/window.h"
#include "debugcallbacks.h"

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() const{
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

class Application {
public:
    Application();
    ~Application();
    void run();

private:
    Window _window;
    vk::Instance _instance;
    vk::SurfaceKHR _surface;
    VkDebugReportCallbackEXT _callback;
    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;
    vk::Queue _graphicsQueue;
    vk::Queue _presentQueue;

    vk::SwapchainKHR _swapChain;
    std::vector<vk::Image> _swapChainImages;
    vk::Format _swapChainImageFormat;
    vk::Extent2D _swapChainExtent;
    std::vector<vk::ImageView> _swapChainImageViews;
    std::vector<vk::Framebuffer> _swapChainFramebuffers;

    vk::RenderPass _renderPass;
    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::PipelineLayout _pipelineLayout;
    vk::Pipeline _graphicsPipeline;

    vk::CommandPool _commandPool;
    std::vector<vk::CommandBuffer> _commandBuffers;

    vk::Buffer _vertexBuffer;
    vk::DeviceMemory _vertexBufferMemory;
    vk::Buffer _indexBuffer;
    vk::DeviceMemory _indexBufferMemory;

    vk::Buffer _uniformStagingBuffer;
    vk::DeviceMemory _uniformStagingBufferMemory;
    vk::Buffer _uniformBuffer;
    vk::DeviceMemory _uniformBufferMemory;
    vk::DescriptorPool _descriptorPool;
    vk::DescriptorSet _descriptorSet;

    vk::Semaphore _imageAvailableSemaphore;
    vk::Semaphore _renderFinishedSemaphore;

private:
    void initVulkan();
    void mainLoop();
    void createInstance();
    void setupDebugCallback();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSemaphores();
    void drawFrame();
    void recreateSwapChain();

    void createVertexBuffer();
    void createIndexBuffer();

    void createUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSet();

    void updateUniformBuffer();

    void createDescriptorSetLayout();

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice &device);
    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice &device);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    static std::vector<char> readFile(const std::string& filename);
    void createShaderModule(const std::vector<char>& code, vk::ShaderModule& shaderModule);

private:
    static bool checkValidationLayerSupport();

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);


};


#endif // APPLICATION_H
