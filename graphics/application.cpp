#include "application.h"

Application::Application()
{

}

Application::~Application()
{

}

void Application::run() {
    _window.init();
    initVulkan();
    mainLoop();
}

void Application::initVulkan() {
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSemaphores();
}

void Application::mainLoop() {
    while (!_window.shouldBeClosed()) {
        _window.pollEvents();
        drawFrame();
    }
    _window.destroy();
}

void Application::createInstance() {
    std::cout << "Creating instance..." << std::endl;
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        std::cerr << "validation layers requested, but not available!" << std::endl;
    }

    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = _window.getRequiredExtensions(enableValidationLayers);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    vk::Result res = vk::createInstance(&createInfo, nullptr, &_instance);
    if(res != vk::Result::eSuccess) {
        std::cerr << "Cannot create instance! error:" << res << std::endl;
    }
}

void Application::setupDebugCallback() {
    if (enableValidationLayers) {
        std::cout << "Setting up callbacks..." << std::endl;
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = debugCallback;

        VkResult res = CreateDebugReportCallbackEXT(_instance, &createInfo, nullptr, &_callback);
        if (res != VK_SUCCESS) {
            std::cerr << "Failed to set up debug callback! error:" << res << std::endl;
        }
    } else {
        std::cout << "No callbacks..." << std::endl;
    }
}

void Application::createSurface() {
    std::cout << "Creating surface..." << std::endl;
    vk::Result res = _window.createSurface(_instance,nullptr,_surface);
    if(res!=vk::Result::eSuccess) {
         std::cerr << "Failed to create surface! error:" << res << std::endl;
    }
}

void Application::pickPhysicalDevice() {
    std::cout << "Picking physical device..." << std::endl;
    uint32_t deviceCount = 0;
    _instance.enumeratePhysicalDevices(&deviceCount,nullptr);

    if (deviceCount == 0) {
        std::cerr << "failed to find GPUs with Vulkan support!" << std::endl;
    }

    std::vector<vk::PhysicalDevice> devices(deviceCount);
    _instance.enumeratePhysicalDevices(&deviceCount,devices.data());

    for (const auto& device : devices) {
        _physicalDevice = device;
    }

    if (_physicalDevice.operator VkPhysicalDevice_T *() == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU!" << std::endl;
    }
}

void Application::createLogicalDevice() {
    std::cout << "Creating logical device..." << std::endl;
    QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
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
    vk::Result res = _physicalDevice.createDevice(&createInfo,nullptr,&_device);
    if(res != vk::Result::eSuccess) {
        std::cerr << "Failed to create logical device! error:" << res << std::endl;
    }
    _device.getQueue(static_cast<uint32_t>(indices.graphicsFamily), 0, &_graphicsQueue);
    _device.getQueue(static_cast<uint32_t>(indices.presentFamily), 0, &_presentQueue);
}

void Application::createSwapChain() {
    std::cout << "Creating swap chain..." << std::endl;
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

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

    QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
    uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.presentFamily)};

    if (indices.graphicsFamily != indices.presentFamily) {
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

    vk::SwapchainKHR oldSwapChain = _swapChain;
    createInfo.oldSwapchain = oldSwapChain;

    vk::SwapchainKHR newSwapChain;

    if (_device.createSwapchainKHR(&createInfo, nullptr, &newSwapChain) != vk::Result::eSuccess) {
        std::cerr << "Failed to create swap chain!" << std::endl;
    }

    _swapChain = newSwapChain;

    _device.getSwapchainImagesKHR(_swapChain, &imageCount, nullptr);
    _swapChainImages.resize(imageCount);
    _device.getSwapchainImagesKHR(_swapChain, &imageCount, _swapChainImages.data());
    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;
}

void Application::createImageViews() {
    std::cout << "Creating image views..." << std::endl;
    _swapChainImageViews.resize(_swapChainImages.size());

    size_t size = _swapChainImages.size();
    for (size_t i = 0; i < size; i++) {
        vk::ImageViewCreateInfo createInfo;
        createInfo.image = _swapChainImages[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = _swapChainImageFormat;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        vk::Result res = _device.createImageView(&createInfo, nullptr, &_swapChainImageViews[i]);
        if (res != vk::Result::eSuccess) {
            std::cerr << "Failed to create image views! error:" << res << std::endl;
        }
    }
}

void Application::createRenderPass() {
    std::cout << "Creating render pass..." << std::endl;
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    vk::Result res = _device.createRenderPass(&renderPassInfo, nullptr, &_renderPass);
    if(res != vk::Result::eSuccess) {
        std::cerr << "Failed to create render pass! error:" << res << std::endl;
    }
}

void Application::createGraphicsPipeline() {
    std::cout << "Creating graphics pipeline..." << std::endl;
    // TODO: use relative paths
    auto vertShaderCode = readFile("/home/deus/repo/engine/shader.vert");
    auto fragShaderCode = readFile("/home/deus/repo/engine/shader.frag");

    vk::ShaderModule vertShaderModule;
    vk::ShaderModule fragShaderModule;
    createShaderModule(vertShaderCode, vertShaderModule);
    createShaderModule(fragShaderCode, fragShaderModule);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapChainExtent.width);
    viewport.height = static_cast<float>(_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0,0);
    scissor.extent = _swapChainExtent;

    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    vk::Result pipelineResult = _device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &_pipelineLayout);
    if(pipelineResult != vk::Result::eSuccess) {
        std::cerr << "Failed to create pipeline layout! error:" << pipelineResult << std::endl;
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;

    vk::Result graphicsResult = _device.createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &_graphicsPipeline);
    if(graphicsResult != vk::Result::eSuccess) {
         std::cerr << "Failed to create graphics pipeline! error:" << graphicsResult << std::endl;
    }
}

void Application::createFramebuffers() {
    std::cout << "Creating framebuffers..." << std::endl;
    _swapChainFramebuffers.resize(_swapChainImageViews.size());

    size_t size = _swapChainImageViews.size();
    for (size_t i = 0; i < size; i++) {
        vk::ImageView attachments[] = { _swapChainImageViews[i] };

        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        vk::Result res = _device.createFramebuffer(&framebufferInfo, nullptr, &_swapChainFramebuffers[i]);
        if(res != vk::Result::eSuccess) {
            std::cerr << "Failed to create framebuffer! error:" << res << std::endl;
        }
    }
}

void Application::createCommandPool() {
    std::cout << "Creating command pools..." << std::endl;
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physicalDevice);

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamilyIndices.graphicsFamily);
    vk::Result res = _device.createCommandPool(&poolInfo, nullptr, &_commandPool);
    if(res != vk::Result::eSuccess ) {
        std::cerr << "Failed to create command pool! error:" << res << std::endl;
    }
}

void Application::createCommandBuffers() {
    std::cout << "Creating command buffers..." << std::endl;
    if (_commandBuffers.size() > 0) {
        _device.freeCommandBuffers(_commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());
    }
    _commandBuffers.resize(_swapChainFramebuffers.size());

    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = _commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

    if(_device.allocateCommandBuffers(&allocInfo, _commandBuffers.data()) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
    }

    size_t size = _commandBuffers.size();
    for (size_t i = 0; i < size; i++) {
        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = _renderPass;
        renderPassInfo.framebuffer = _swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = vk::Offset2D(0,0);
        renderPassInfo.renderArea.extent = _swapChainExtent;

        vk::ClearValue clearColor;
        vk::ClearColorValue color(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}});
        clearColor.setColor(color);
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        _commandBuffers[i].begin(&beginInfo);
        _commandBuffers[i].beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
        _commandBuffers[i].bindPipeline( vk::PipelineBindPoint::eGraphics, _graphicsPipeline);
        _commandBuffers[i].draw(3, 1, 0, 0);
        _commandBuffers[i].endRenderPass();
        _commandBuffers[i].end();
    }
}

void Application::createSemaphores() {
    std::cout << "Creating semaphores..." << std::endl;
    vk::SemaphoreCreateInfo semaphoreInfo;

    if(_device.createSemaphore(&semaphoreInfo, nullptr, &_imageAvailableSemaphore) != vk::Result::eSuccess ||
       _device.createSemaphore(&semaphoreInfo, nullptr, &_renderFinishedSemaphore) != vk::Result::eSuccess) {
        std::cerr << "Failed to create semaphores!" << std::endl;
    }
}

void Application::drawFrame() {
    uint32_t imageIndex;
    vk::Result result = _device.acquireNextImageKHR(_swapChain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphore, nullptr, &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        std::cerr << "Failed to acquire swap chain image! error:" << result<< std::endl;
    }

    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {_imageAvailableSemaphore};
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

    vk::Semaphore signalSemaphores[] = {_renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vk::Result submitResult = _graphicsQueue.submit(1, &submitInfo, nullptr);
    if(submitResult != vk::Result::eSuccess) {
        std::cerr << "failed to submit draw command buffer! error:" << submitResult << std::endl;
    }

    vk::PresentInfoKHR presentInfo = {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = {_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vk::Result presentResult = _presentQueue.presentKHR(&presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
        recreateSwapChain();
    } else if (presentResult != vk::Result::eSuccess) {
        std::cerr << "failed to present swap chain image! error:" << presentResult << std::endl;
    }
}

void Application::recreateSwapChain() {
    std::cout << "Recreating swap chain..." << std::endl;
    _device.waitIdle();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
}

QueueFamilyIndices Application::findQueueFamilies(vk::PhysicalDevice & device) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    device.getQueueFamilyProperties(&queueFamilyCount, nullptr);
    std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
    device.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        vk::Bool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, static_cast<uint32_t>(i), _surface, &presentSupport);
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

SwapChainSupportDetails Application::querySwapChainSupport(vk::PhysicalDevice & device) {
    SwapChainSupportDetails details;
    device.getSurfaceCapabilitiesKHR(_surface, &details.capabilities);
    uint32_t formatCount = 0;
    device.getSurfaceFormatsKHR(_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        device.getSurfaceFormatsKHR( _surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount = 0;
    device.getSurfacePresentModesKHR(_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        device.getSurfacePresentModesKHR(_surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

vk::SurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
        return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
    }
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR Application::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    vk::PresentModeKHR bestMode = vk::PresentModeKHR::eFifo;
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox){
            return availablePresentMode;
        } else if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            bestMode = availablePresentMode;
        }
    }
    return bestMode;
}

vk::Extent2D Application::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        vk::Extent2D actualExtent = {_window.width(),  _window.height()};
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

std::vector<char> Application::readFile(const std::__cxx11::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
    }

    auto fileSize = file.tellg();
    if(fileSize > 0) {
        std::vector<char> buffer(static_cast<size_t>(fileSize));
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    } else {
        std::cerr << "Empty file: " << filename << std::endl;
    }

    return std::vector<char>();
}

void Application::createShaderModule(const std::vector<char> &code, vk::ShaderModule &shaderModule) {
    vk::ShaderModuleCreateInfo createInfo = {};
    createInfo.codeSize = code.size();
    std::vector<uint32_t> codeAligned(code.size() / 4 + 1);
    memcpy(codeAligned.data(), code.data(), code.size());
    createInfo.pCode = codeAligned.data();
    vk::Result res = _device.createShaderModule(&createInfo, nullptr, &shaderModule);
    if(res != vk::Result::eSuccess) {
        std::cerr << "failed to create shader module! error:" << res << std::endl;
    }
}

bool Application::checkValidationLayerSupport() {
    uint32_t layerCount;
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

