#include "application.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>

Application::Application()
{
}

Application::~Application() {}

void Application::run()
{
    _window.init();
    initVulkan();
    mainLoop();
    destroyVulkan();
    _window.destroy();
}

void Application::initVulkan()
{
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createDepthResources();
    createFramebuffers();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    createCommandBuffers();
    createSemaphores();
    updateUniformBuffer();
}

void Application::mainLoop()
{
    while (!_window.shouldBeClosed()) {
        _window.pollEvents();
        updateUniformBuffer();
        drawFrame();
    }
}

void Application::destroyVulkan()
{
    _graphicsQueue.waitIdle();
    _presentQueue.waitIdle();
    for (const vk::Fence& fence : _waitFences) {
        _device.destroyFence(fence);
    }

    for (const vk::Framebuffer& framebuffer : _swapChainFramebuffers) {
        _device.destroyFramebuffer(framebuffer);
    }

    for (const vk::ImageView& imageview : _swapChainImageViews) {
        _device.destroyImageView(imageview);
    }

    if (_commandBuffers.size() > 0) {
        _device.freeCommandBuffers(_commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());
    }

    _device.destroySemaphore(_imageAvailableSemaphore);
    _device.destroySemaphore(_renderFinishedSemaphore);

    _device.destroyBuffer(_vertexBuffer);
    _device.destroyBuffer(_indexBuffer);
    _device.destroyBuffer(_uniformStagingBuffer);
    _device.destroyBuffer(_uniformBuffer);
    _device.destroyImage(_depthImage);

    _device.freeMemory(_vertexBufferMemory);
    _device.freeMemory(_indexBufferMemory);
    _device.freeMemory(_uniformStagingBufferMemory);
    _device.freeMemory(_uniformBufferMemory);
    _device.freeMemory(_depthImageMemory);
    _device.destroyImageView(_depthImageView);

    _device.destroyDescriptorSetLayout(_descriptorSetLayout);
    _device.destroyDescriptorPool(_descriptorPool);

    _device.destroyCommandPool(_commandPool);

    _device.destroyShaderModule(_vertShaderModule);
    _device.destroyShaderModule(_fragShaderModule);
    _device.destroyPipelineCache(_cache);
    _device.destroyPipelineLayout(_pipelineLayout);
    _device.destroyPipeline(_graphicsPipeline);

    _device.destroyRenderPass(_renderPass);
    _device.destroySwapchainKHR(_swapChain);
    _device.destroy();
    DestroyDebugReportCallbackEXT(_instance, _callback, nullptr);
    _instance.destroySurfaceKHR(_surface);
    _instance.destroy();
}

void Application::createInstance()
{
    std::cerr << "Creating instance..." << std::endl;
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        std::cerr << "validation layers requested, but not available!" << std::endl;
        std::abort();
    }

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;

    const std::vector<const char*>& extensions = _window.getRequiredExtensions(enableValidationLayers);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.enabledLayerCount = 0;
    }

    vk::Result res = vk::createInstance(&createInfo, nullptr, &_instance);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Cannot create instance! error:" << res << std::endl;
        std::abort();
    }
}

void Application::setupDebugCallback()
{
    if (enableValidationLayers) {
        std::cerr << "Setting up callbacks..." << std::endl;
        vk::DebugReportCallbackCreateInfoEXT createInfo;
        createInfo.flags = vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning;
        createInfo.pfnCallback = debugCallback;

        VkResult res = CreateDebugReportCallbackEXT(_instance, &createInfo.operator const VkDebugReportCallbackCreateInfoEXT&(), nullptr, &_callback);
        if (res != VK_SUCCESS) {
            std::cerr << "Failed to set up debug callback! error:" << res << std::endl;
            std::abort();
        }
    } else {
        std::cerr << "No callbacks..." << std::endl;
    }
}

void Application::createSurface()
{
    std::cerr << "Creating surface..." << std::endl;
    vk::Result res = _window.createSurface(_instance, _surface);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to create surface! error:" << res << std::endl;
        std::abort();
    }
}

void Application::pickPhysicalDevice()
{
    std::cerr << "Picking physical device..." << std::endl;
    uint32_t deviceCount = 0;
    _instance.enumeratePhysicalDevices(&deviceCount, nullptr);

    if (deviceCount == 0) {
        std::cerr << "failed to find GPUs with Vulkan support!" << std::endl;
        std::abort();
    }

    std::vector<vk::PhysicalDevice> devices(deviceCount);
    _instance.enumeratePhysicalDevices(&deviceCount, devices.data());

    for (const auto& device : devices) {
        _physicalDevice = device;
        _queueFamilyIndices = findQueueFamilies(_physicalDevice, _surface);
    }

    if (_physicalDevice.operator VkPhysicalDevice_T*() == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU!" << std::endl;
        std::abort();
    }
}

void Application::createLogicalDevice()
{
    std::cerr << "Creating logical device..." << std::endl;

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = { _queueFamilyIndices.graphicsFamily, _queueFamilyIndices.presentFamily };

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamily);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::DeviceCreateInfo createInfo = {};

    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    vk::Result res = _physicalDevice.createDevice(&createInfo, nullptr, &_device);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to create logical device! error:" << res << std::endl;
        std::abort();
    }
    _device.getQueue(static_cast<uint32_t>(_queueFamilyIndices.graphicsFamily), 0, &_graphicsQueue);
    _device.getQueue(static_cast<uint32_t>(_queueFamilyIndices.presentFamily), 0, &_presentQueue);
}

void Application::createSwapChain()
{
    std::cerr << "Creating swap chain..." << std::endl;
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice, _surface);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, _window.width(), _window.height());

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(_queueFamilyIndices.graphicsFamily), static_cast<uint32_t>(_queueFamilyIndices.presentFamily) };

    if (_queueFamilyIndices.graphicsFamily != _queueFamilyIndices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = vk::SwapchainKHR();

    vk::SwapchainKHR newSwapChain;
    _device.waitIdle();
    vk::Result res = _device.createSwapchainKHR(&createInfo, nullptr, &newSwapChain);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to create swap chain! error:" << res << std::endl;
        std::abort();
    }
    _device.waitIdle();
    _swapChain = newSwapChain;
    _device.getSwapchainImagesKHR(_swapChain, &imageCount, nullptr);
    _swapChainImages.clear();
    _swapChainImages.resize(imageCount);
    _device.getSwapchainImagesKHR(_swapChain, &imageCount, _swapChainImages.data());
    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;
    assert(_swapChainExtent.height != 0);
}

void Application::createImageViews()
{
    std::cerr << "Creating image views..." << std::endl;
    _swapChainImageViews.resize(_swapChainImages.size());

    size_t size = _swapChainImages.size();
    for (size_t i = 0; i < size; i++) {
        createImageView(_swapChainImages[i], _swapChainImageFormat, vk::ImageAspectFlagBits::eColor, _swapChainImageViews[i]);
    }
}

void Application::createRenderPass()
{
    std::cerr << "Creating render pass..." << std::endl;
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = findDepthFormat(_physicalDevice);
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    std::array<vk::AttachmentDescription, 2> attachments = { { colorAttachment, depthAttachment } };

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    vk::Result res = _device.createRenderPass(&renderPassInfo, nullptr, &_renderPass);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to create render pass! error:" << res << std::endl;
        std::abort();
    }
}

void Application::createGraphicsPipeline()
{
    std::cerr << "Creating graphics pipeline..." << std::endl;

    // Copy shaders directory from repo or change working directory
    const auto vertShaderCode = readFile("shaders/vert.spv");
    const auto fragShaderCode = readFile("shaders/frag.spv");

    std::cerr << "Creating vertex shader..." << std::endl;
    createShaderModule(_device, vertShaderCode, _vertShaderModule);
    std::cerr << "Creating fragment shader..." << std::endl;
    createShaderModule(_device, fragShaderCode, _fragShaderModule);
    std::cerr << "Shaders created!" << std::endl;

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = _vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = _fragShaderModule;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, VK_FALSE);

    vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(_swapChainExtent.width), static_cast<float>(_swapChainExtent.height), 0.0f, 1.0f);

    vk::Rect2D scissor(vk::Offset2D(0, 0), _swapChainExtent);

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineTessellationStateCreateInfo tesselationState;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depthStencil(vk::PipelineDepthStencilStateCreateFlags(), VK_TRUE, VK_TRUE, vk::CompareOp::eLess, VK_FALSE, VK_FALSE);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending(vk::PipelineColorBlendStateCreateFlags(), VK_FALSE, vk::LogicOp::eCopy, 1, &colorBlendAttachment, { { 0.0f, 0.0f, 0.0f, 0.0f } });

    vk::DescriptorSetLayout setLayouts[] = { _descriptorSetLayout };
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = setLayouts;

    vk::Result pipelineResult = _device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &_pipelineLayout);
    if (pipelineResult != vk::Result::eSuccess) {
        std::cerr << "Failed to create pipeline layout! error:" << pipelineResult << std::endl;
        std::abort();
    }
    std::cerr << "Pipeline layout created!" << std::endl;

    vk::GraphicsPipelineCreateInfo pipelineInfo(vk::PipelineCreateFlags(), 2, shaderStages, &vertexInputInfo, &inputAssembly, &tesselationState, &viewportState, &rasterizer, &multisampling, &depthStencil, &colorBlending, nullptr, _pipelineLayout, _renderPass, 0, vk::Pipeline(), 0);

    std::cerr << "Creating pipeline cache..." << std::endl;
    vk::PipelineCacheCreateInfo cacheCreateInfo;
    vk::Result cacheResult = _device.createPipelineCache(&cacheCreateInfo, nullptr, &_cache);
    if (cacheResult != vk::Result::eSuccess) {
        std::cerr << "Failed to create pipeline cache! error:" << cacheResult << std::endl;
        std::abort();
    }

    vk::Result graphicsResult = _device.createGraphicsPipelines(_cache, 1, &pipelineInfo, nullptr, &_graphicsPipeline);
    if (graphicsResult != vk::Result::eSuccess) {
        std::cerr << "Failed to create graphics pipeline! error:" << graphicsResult << std::endl;
        std::abort();
    }
    std::cerr << "Graphics pipeline created!" << std::endl;
}

void Application::createFramebuffers()
{
    std::cerr << "Creating framebuffers..." << std::endl;
    _swapChainFramebuffers.resize(_swapChainImageViews.size());

    size_t size = _swapChainImageViews.size();
    for (size_t i = 0; i < size; i++) {
        std::array<vk::ImageView, 2> attachments = { { _swapChainImageViews[i], _depthImageView } };

        vk::FramebufferCreateInfo framebufferInfo;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        vk::Result res = _device.createFramebuffer(&framebufferInfo, nullptr, &_swapChainFramebuffers[i]);
        if (res != vk::Result::eSuccess) {
            std::cerr << "Failed to create framebuffer! error:" << res << std::endl;
            std::abort();
        }
    }
}

void Application::createCommandPool()
{
    std::cerr << "Creating command pools..." << std::endl;
    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.queueFamilyIndex = static_cast<uint32_t>(_queueFamilyIndices.graphicsFamily);
    vk::Result res = _device.createCommandPool(&poolInfo, nullptr, &_commandPool);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to create command pool! error:" << res << std::endl;
        std::abort();
    }
}

void Application::createCommandBuffers()
{
    std::cerr << "Creating command buffers..." << std::endl;
    if (_commandBuffers.size() > 0) {
        _device.freeCommandBuffers(_commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());
    }
    _commandBuffers.resize(_swapChainFramebuffers.size());

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

    if (_device.allocateCommandBuffers(&allocInfo, _commandBuffers.data()) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
        std::abort();
    }

    size_t size = _commandBuffers.size();
    for (size_t i = 0; i < size; i++) {
        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = _renderPass;
        renderPassInfo.framebuffer = _swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
        renderPassInfo.renderArea.extent = _swapChainExtent;

        std::array<vk::ClearValue, 2> clearValues = {};
        clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{ { 0.1f, 0.2f, 0.1f, 1.0f } });
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vk::Buffer vertexBuffers[] = { _vertexBuffer };
        vk::DeviceSize offsets[] = { 0 };

        if (_commandBuffers[i].begin(&beginInfo) == vk::Result::eSuccess) {
            _commandBuffers[i].beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

            _commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);
            _commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);
            _commandBuffers[i].bindIndexBuffer(_indexBuffer, 0, vk::IndexType::eUint16);
            _commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
            _commandBuffers[i].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            _commandBuffers[i].endRenderPass();
            _commandBuffers[i].end();
        } else {
            std::cerr << "Command buffers bind fail!" << std::endl;
            std::abort();
        }
    }
}

void Application::createSemaphores()
{
    std::cerr << "Creating semaphores..." << std::endl;
    vk::SemaphoreCreateInfo semaphoreInfo;

    if (_device.createSemaphore(&semaphoreInfo, nullptr, &_imageAvailableSemaphore) != vk::Result::eSuccess || _device.createSemaphore(&semaphoreInfo, nullptr, &_renderFinishedSemaphore) != vk::Result::eSuccess) {
        std::cerr << "Failed to create semaphores!" << std::endl;
        std::abort();
    }
    vk::FenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    _waitFences.resize(_commandBuffers.size());
    for (auto& fence : _waitFences) {
        if (_device.createFence(&fenceCreateInfo, nullptr, &fence) != vk::Result::eSuccess) {
            std::cerr << "Failed to create fence!" << std::endl;
            std::abort();
        }
    }
}

void Application::updateUniformBuffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
    float ratio = static_cast<float>(_swapChainExtent.width) / static_cast<float>(_swapChainExtent.height);
    UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(46.0f), ratio, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1.0f;
    void* dataPtr = nullptr;

    vk::Result res = _device.mapMemory(_uniformStagingBufferMemory, vk::DeviceSize(0), sizeof(UniformBufferObject), vk::MemoryMapFlags(), &dataPtr);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to map memory for uniform buffer! error:" << res << std::endl;
        std::abort();
    }
    memcpy(dataPtr, &ubo, sizeof(UniformBufferObject));
    _device.unmapMemory(_uniformStagingBufferMemory);
    copyBuffer(_device, _commandPool, _graphicsQueue, _uniformStagingBuffer, _uniformBuffer, sizeof(UniformBufferObject));
}

void Application::drawFrame()
{
    uint32_t imageIndex;
    vk::Result result = _device.acquireNextImageKHR(_swapChain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphore, nullptr, &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        std::cerr << "Failed to acquire swap chain image! error:" << result << std::endl;
        std::abort();
    }

    _device.waitForFences(1, &_waitFences[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
    _device.resetFences(1, &_waitFences[imageIndex]);

    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_renderFinishedSemaphore;

    vk::Result submitResult = _graphicsQueue.submit(1, &submitInfo, _waitFences[imageIndex]);
    if (submitResult != vk::Result::eSuccess) {
        std::cerr << "failed to submit draw command buffer! error:" << submitResult << std::endl;
        std::abort();
    }

    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapChain;
    presentInfo.pImageIndices = &imageIndex;

    vk::Result presentResult = _presentQueue.presentKHR(&presentInfo);
    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
        recreateSwapChain();
    } else if (presentResult != vk::Result::eSuccess) {
        std::cerr << "failed to present swap chain image! error:" << presentResult << std::endl;
        std::abort();
    }
}

void Application::recreateSwapChain()
{
    std::cerr << "Recreating swap chain..." << std::endl;
    _graphicsQueue.waitIdle();
    _presentQueue.waitIdle();
    _device.waitIdle();
    for (const vk::ImageView& imageview : _swapChainImageViews) {
        _device.destroyImageView(imageview);
    }
    for (const vk::Framebuffer& framebuffer : _swapChainFramebuffers) {
        _device.destroyFramebuffer(framebuffer);
    }

    _device.destroyImage(_depthImage);
    _device.freeMemory(_depthImageMemory);
    _device.destroyImageView(_depthImageView);
    _device.destroyShaderModule(_vertShaderModule);
    _device.destroyShaderModule(_fragShaderModule);
    _device.destroyPipelineCache(_cache);
    _device.destroyPipelineLayout(_pipelineLayout);
    _device.destroyPipeline(_graphicsPipeline);
    _device.destroyRenderPass(_renderPass);
    _device.destroySwapchainKHR(_swapChain);

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
}

void Application::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(_device, _physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* dataPtr = nullptr;
    vk::Result res = _device.mapMemory(stagingBufferMemory, vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags(), &dataPtr);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to map memory for vertex buffer! error:" << res << std::endl;
        std::abort();
    }
    memcpy(dataPtr, vertices.data(), static_cast<size_t>(bufferSize));
    _device.unmapMemory(stagingBufferMemory);

    createBuffer(_device, _physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, _vertexBuffer, _vertexBufferMemory);
    copyBuffer(_device, _commandPool, _graphicsQueue, stagingBuffer, _vertexBuffer, bufferSize);
    _device.destroyBuffer(stagingBuffer);
    _device.freeMemory(stagingBufferMemory);
}

void Application::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(uint16_t) * indices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(_device, _physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* dataPtr = nullptr;
    vk::Result res = _device.mapMemory(stagingBufferMemory, vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags(), &dataPtr);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to map memory for index buffer! error:" << res << std::endl;
        std::abort();
    }
    memcpy(dataPtr, indices.data(), static_cast<size_t>(bufferSize));
    _device.unmapMemory(stagingBufferMemory);

    createBuffer(_device, _physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, _indexBuffer, _indexBufferMemory);
    copyBuffer(_device, _commandPool, _graphicsQueue, stagingBuffer, _indexBuffer, bufferSize);
    _device.destroyBuffer(stagingBuffer);
    _device.freeMemory(stagingBufferMemory);
}

void Application::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    std::array<vk::DescriptorSetLayoutBinding, 1> bindings = { { uboLayoutBinding } };

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    if (_device.createDescriptorSetLayout(&layoutInfo, nullptr, &_descriptorSetLayout) != vk::Result::eSuccess) {
        std::cerr << "Failed to create descriptor set layout!" << std::endl;
        std::abort();
    }
}

void Application::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat(_physicalDevice);
    createImage(_swapChainExtent.width, _swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, _depthImage, _depthImageMemory);
    createImageView(_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, _depthImageView);
    transitionImageLayout(_depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

void Application::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageView& imageView)
{
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (_device.createImageView(&viewInfo, nullptr, &imageView) != vk::Result::eSuccess) {
        std::cerr << "Failed to create texture image view!" << std::endl;
        std::abort();
    }
}

void Application::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory)
{
    vk::ImageCreateInfo imageInfo = {};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::ePreinitialized;
    imageInfo.usage = usage;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    if (_device.createImage(&imageInfo, nullptr, &image) != vk::Result::eSuccess) {
        std::cerr << "failed to create image!" << std::endl;
        std::abort();
    }

    vk::MemoryRequirements memRequirements;
    _device.getImageMemoryRequirements(image, &memRequirements);

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(_physicalDevice, memRequirements.memoryTypeBits, properties);

    if (_device.allocateMemory(&allocInfo, nullptr, &imageMemory) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate image memory!" << std::endl;
        std::abort();
    }
    _device.bindImageMemory(image, imageMemory, 0);
}

void Application::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(_device, _commandPool);

    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        std::cout << "Failed to transition layout!" << std::endl;
        std::abort();
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(_device, _graphicsQueue, _commandPool, commandBuffer);
}

void Application::createUniformBuffer()
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    createBuffer(_device, _physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, _uniformStagingBuffer, _uniformStagingBufferMemory);
    createBuffer(_device, _physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, _uniformBuffer, _uniformBufferMemory);
}

void Application::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 1> poolSizes;
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = 1;

    vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(), 1, poolSizes.size(), poolSizes.data());

    if (_device.createDescriptorPool(&poolInfo, nullptr, &_descriptorPool) != vk::Result::eSuccess) {
        std::cerr << "Failed to create descriptor pool!" << std::endl;
        std::abort();
    }
}

void Application::createDescriptorSet()
{
    vk::DescriptorSetAllocateInfo allocInfo(_descriptorPool, 1, &_descriptorSetLayout);

    if (_device.allocateDescriptorSets(&allocInfo, &_descriptorSet) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate descriptor set!" << std::endl;
        std::abort();
    }

    vk::DescriptorBufferInfo bufferInfo(_uniformBuffer, 0, sizeof(UniformBufferObject));

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {};

    descriptorWrites[0].dstSet = _descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    _device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
