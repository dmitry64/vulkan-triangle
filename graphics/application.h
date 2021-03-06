#ifndef APPLICATION_INCONCE_H
#define APPLICATION_INCONCE_H

#include <vector>
#include <vulkan/vulkan.hpp>

#include "debugcallbacks.h"
#include "helperfunctions.h"
#include "window/window.h"

#include "vertex.h"

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const std::vector<Vertex> vertices = { { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
    { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },

    { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
    { { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    { { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } } };

const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 };

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
    vk::PipelineCache _cache;

    QueueFamilyIndices _queueFamilyIndices;

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

    vk::Image _depthImage;
    vk::DeviceMemory _depthImageMemory;
    vk::ImageView _depthImageView;

    vk::Semaphore _imageAvailableSemaphore;
    vk::Semaphore _renderFinishedSemaphore;
    std::vector<vk::Fence> _waitFences;

    vk::ShaderModule _vertShaderModule;
    vk::ShaderModule _fragShaderModule;

private:
    void initVulkan();
    void mainLoop();
    void destroyVulkan();
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
    void createDepthResources();

    void createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageView& imageView);
    void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory);
    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
};

#endif // APPLICATION_H
