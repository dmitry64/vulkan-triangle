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
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createDepthResources();
    createFramebuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    createCommandBuffers();
    createSemaphores();
}

void Application::mainLoop() {
    while (!_window.shouldBeClosed()) {
        _window.pollEvents();
        updateUniformBuffer();
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
    vk::Result res = _device.createSwapchainKHR(&createInfo, nullptr, &newSwapChain);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Failed to create swap chain! error:" << res << std::endl;
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
        createImageView(_swapChainImages[i], _swapChainImageFormat, vk::ImageAspectFlagBits::eColor, _swapChainImageViews[i]);
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

    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

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
    std::array<vk::AttachmentDescription, 2> attachments = {{colorAttachment, depthAttachment}};

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
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

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
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

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

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

    vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::DescriptorSetLayout setLayouts[] = {_descriptorSetLayout};
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = setLayouts;

    vk::Result pipelineResult = _device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &_pipelineLayout);
    if(pipelineResult != vk::Result::eSuccess) {
        std::cerr << "Failed to create pipeline layout! error:" << pipelineResult << std::endl;
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
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
        std::array<vk::ImageView, 2> attachments = {
            {_swapChainImageViews[i],
            _depthImageView}
        };

        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
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

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

    if(_device.allocateCommandBuffers(&allocInfo, _commandBuffers.data()) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
    }

    size_t size = _commandBuffers.size();
    for (size_t i = 0; i < size; i++) {
        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = _renderPass;
        renderPassInfo.framebuffer = _swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = vk::Offset2D(0,0);
        renderPassInfo.renderArea.extent = _swapChainExtent;

        std::array<vk::ClearValue, 2> clearValues = {};
        clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}});
        clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vk::ClearValue clearColor;
        vk::ClearColorValue color(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}});
        clearColor.setColor(color);

        vk::Buffer vertexBuffers[] = {_vertexBuffer};
        vk::DeviceSize offsets[] = {0};

        _commandBuffers[i].begin(&beginInfo);
        _commandBuffers[i].beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

            _commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline);
            _commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);
            _commandBuffers[i].bindIndexBuffer(_indexBuffer, 0, vk::IndexType::eUint16);
            _commandBuffers[i].bindDescriptorSets( vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
            _commandBuffers[i].drawIndexed( indices.size(), 1, 0, 0, 0);

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

void Application::updateUniformBuffer() {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), _swapChainExtent.width / (float) _swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    void* data;
    _device.mapMemory(_uniformStagingBufferMemory, vk::DeviceSize(0), sizeof(ubo), vk::MemoryMapFlags(), &data);
        memcpy(data, &ubo, sizeof(ubo));
    _device.unmapMemory(_uniformStagingBufferMemory);
    copyBuffer(_uniformStagingBuffer, _uniformBuffer, sizeof(ubo));
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
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
}

void Application::createVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* data;
    _device.mapMemory(stagingBufferMemory, vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags(), &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    _device.unmapMemory(stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, _vertexBuffer, _vertexBufferMemory);
    copyBuffer(stagingBuffer, _vertexBuffer, bufferSize);
}

void Application::createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* data;
    _device.mapMemory(stagingBufferMemory, vk::DeviceSize(0), bufferSize, vk::MemoryMapFlags(), &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    _device.unmapMemory(stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, _indexBuffer, _indexBufferMemory);
    copyBuffer(stagingBuffer, _indexBuffer, bufferSize);
}

void Application::createDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {{uboLayoutBinding, samplerLayoutBinding}};

    vk::DescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    if (_device.createDescriptorSetLayout(&layoutInfo, nullptr, &_descriptorSetLayout) != vk::Result::eSuccess) {
        std::cerr << "Failed to create descriptor set layout!" << std::endl;
    }
}

void Application::createDepthResources() {
    vk::Format depthFormat = findDepthFormat();

    createImage(_swapChainExtent.width, _swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, _depthImage, _depthImageMemory);
    createImageView(_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, _depthImageView);

    transitionImageLayout(_depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

vk::Format Application::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (vk::Format format : candidates) {
        vk::FormatProperties props;
        _physicalDevice.getFormatProperties(format, &props);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    std::cerr << "Failed to find supported format!" << std::endl;
    return vk::Format();
}

vk::Format Application::findDepthFormat() {
    return findSupportedFormat(
    { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                vk::ImageTiling::eOptimal,
                vk::FormatFeatureFlagBits::eDepthStencilAttachment
                );
}

bool Application::hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

void Application::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("/home/deus/repo/engine/texture.jpeg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        std::cerr << "Failed to load texture image!" << std::endl;
    }

    vk::Image stagingImage;
    vk::DeviceMemory stagingImageMemory;
    createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingImage, stagingImageMemory);

    vk::ImageSubresource subresource;
    subresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    subresource.mipLevel = 0;
    subresource.arrayLayer = 0;

    vk::SubresourceLayout stagingImageLayout;
    _device.getImageSubresourceLayout(stagingImage, &subresource, &stagingImageLayout);

    void* data;
    _device.mapMemory(stagingImageMemory, vk::DeviceSize(0), imageSize, vk::MemoryMapFlags(), &data);

    if (stagingImageLayout.rowPitch == texWidth * 4) {
        memcpy(data, pixels, (size_t) imageSize);
    } else {
        uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

        for (int y = 0; y < texHeight; y++) {
            memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &pixels[y * texWidth * 4], texWidth * 4);
        }
    }
    _device.unmapMemory(stagingImageMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, _textureImage, _textureImageMemory);

    transitionImageLayout(stagingImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferSrcOptimal);
    transitionImageLayout(_textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal);
    copyImage(stagingImage, _textureImage, texWidth, texHeight);

    transitionImageLayout(_textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Application::createTextureImageView() {
    createImageView(_textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, _textureImageView);
}

void Application::createTextureSampler() {
    vk::SamplerCreateInfo samplerInfo = {};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

    if (_device.createSampler(&samplerInfo, nullptr, &_textureSampler) != vk::Result::eSuccess) {
        std::cerr << "Failed to create texture sampler!" << std::endl;
    }
}

void Application::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageView& imageView) {
    vk::ImageViewCreateInfo viewInfo = {};
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
    }
}

void Application::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image &image, vk::DeviceMemory &imageMemory) {
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
    }

    vk::MemoryRequirements memRequirements;
    _device.getImageMemoryRequirements(image, &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (_device.allocateMemory(&allocInfo, nullptr, &imageMemory) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate image memory!" << std::endl;
    }
    _device.bindImageMemory(image, imageMemory, 0);
}

void Application::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier = {};
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

    if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferSrcOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
    } else if (oldLayout == vk::ImageLayout::ePreinitialized && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
                barrier.srcAccessMask = vk::AccessFlags();
                barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    } else {
        std::cerr << "unsupported layout transition!" << std::endl;
    }

    commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::DependencyFlags(),
                0, nullptr,
                0, nullptr,
                1, &barrier
                );
    endSingleTimeCommands(commandBuffer);
}

void Application::copyImage(vk::Image srcImage, vk::Image dstImage, uint32_t width, uint32_t height) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::ImageSubresourceLayers subResource = {};
    subResource.aspectMask = vk::ImageAspectFlagBits::eColor;
    subResource.baseArrayLayer = 0;
    subResource.mipLevel = 0;
    subResource.layerCount = 1;

    vk::ImageCopy region = {};
    region.srcSubresource = subResource;
    region.dstSubresource = subResource;
    region.srcOffset = vk::Offset3D(0, 0, 0);
    region.dstOffset = vk::Offset3D(0, 0, 0);
    region.extent.width = width;
    region.extent.height = height;
    region.extent.depth = 1;

    commandBuffer.copyImage(
                srcImage, vk::ImageLayout::eTransferSrcOptimal,
                dstImage, vk::ImageLayout::eTransferDstOptimal,
                1, &region
                );

    endSingleTimeCommands(commandBuffer);
}

void Application::createUniformBuffer() {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, _uniformStagingBuffer, _uniformStagingBufferMemory);
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, _uniformBuffer, _uniformBufferMemory);
}

void Application::createDescriptorPool() {
    std::array<vk::DescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = 1;

    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (_device.createDescriptorPool(&poolInfo, nullptr, &_descriptorPool) != vk::Result::eSuccess) {
        std::cerr << "Failed to create descriptor pool!" << std::endl;
    }
}

void Application::createDescriptorSet() {
    vk::DescriptorSetLayout layouts[] = {_descriptorSetLayout};
    vk::DescriptorSetAllocateInfo allocInfo = {};
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    if (_device.allocateDescriptorSets(&allocInfo, &_descriptorSet) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate descriptor set!" << std::endl;
    }

    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = _textureImageView;
    imageInfo.sampler = _textureSampler;

    std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {};

    descriptorWrites[0].dstSet = _descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].dstSet = _descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    _device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Application::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer &buffer, vk::DeviceMemory &bufferMemory) {
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (_device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) {
        std::cerr << "Failed to create buffer!" << std::endl;
    }

    vk::MemoryRequirements memRequirements;
    _device.getBufferMemoryRequirements(buffer, &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (_device.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate buffer memory!" << std::endl;
    }
    _device.bindBufferMemory(buffer, bufferMemory, 0);
}

vk::CommandBuffer Application::beginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    _device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(&beginInfo);

    return commandBuffer;
}

void Application::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    _graphicsQueue.submit(1, &submitInfo, nullptr);
    _graphicsQueue.waitIdle();
    _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
}

void Application::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    _device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(&beginInfo);

    vk::BufferCopy copyRegion = {};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    commandBuffer.end();

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    _graphicsQueue.submit(1, &submitInfo, nullptr);
    _graphicsQueue.waitIdle();
    _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
}

uint32_t Application::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties;
    _physicalDevice.getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    std::cerr << "Failed to find suitable memory type!" << std::endl;
    return 0;
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

vk::Extent2D Application::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
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
    vk::ShaderModuleCreateInfo createInfo;
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

