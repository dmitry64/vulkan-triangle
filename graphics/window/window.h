#ifndef WINDOW_H
#define WINDOW_H


#include <vulkan/vulkan.hpp>

class GLFWwindow;

class Window
{
    uint32_t _width;
    uint32_t _height;
    GLFWwindow* window;

public:
    void init();
    void destroy();
    bool shouldBeClosed();
    vk::Result createSurface(vk::Instance& instance, vk::SurfaceKHR& surface);
    void setSize(uint32_t width, uint32_t height);
    Window();
    uint32_t width() const;
    uint32_t height() const;
    void setHeight(const uint32_t& height);
    void setWidth(const uint32_t& width);
    GLFWwindow* getWindow() const;
    void pollEvents();
    std::vector<const char*> getRequiredExtensions(bool validation);
};

#endif  // WINDOW_H
