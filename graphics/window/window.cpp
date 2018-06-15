#include "window.h"
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

uint32_t Window::width() const
{
    return _width;
}

uint32_t Window::height() const
{
    return _height;
}

void Window::setHeight(const uint32_t& height)
{
    _height = height;
}

void Window::setWidth(const uint32_t& width)
{
    _width = width;
}

GLFWwindow* Window::getWindow() const
{
    return window;
}

void Window::pollEvents()
{
    glfwPollEvents();
}

std::vector<const char*> Window::getRequiredExtensions(bool validation)
{
    std::vector<const char*> extensions;
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (unsigned int i = 0; i < glfwExtensionCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }
    if (validation) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
    return extensions;
}

void Window::init()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(_width, _height, "Vulkan", nullptr, nullptr);
    assert(window);
}

void Window::destroy()
{
    assert(window);
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::shouldBeClosed()
{
    assert(window);
    return glfwWindowShouldClose(window);
}

vk::Result Window::createSurface(vk::Instance& instance, vk::SurfaceKHR& surface)
{
    assert(window);
    VkSurfaceKHR tmp;
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &tmp);
    surface = tmp;
    return vk::Result(result);
}

void Window::setSize(uint32_t width, uint32_t height)
{
    _width = width;
    _height = height;
}

Window::Window()
    : _width(1024)
    , _height(768)
    , window(nullptr)
{
}
