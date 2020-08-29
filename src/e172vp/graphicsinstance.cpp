#include "graphicsinstance.h"

#include "tools/hardware.h"
#include "tools/validation.h"
#include "tools/stringvector.h"
#include "tools/extensiontools.h"
#include "tools/vulkaninstancefactory.h"
#include "tools/logicdevicefactory.h"

#include <set>
#include <iostream>

vk::Instance e172vp::GraphicsInstance::vulkanInstance() const { return m_vulkanInstance; }
vk::PhysicalDevice e172vp::GraphicsInstance::physicalDevice() const { return m_physicalDevice; }
vk::Device e172vp::GraphicsInstance::logicalDevice() const { return m_logicalDevice; }
vk::SurfaceKHR e172vp::GraphicsInstance::surface() const { return m_surface; }
bool e172vp::GraphicsInstance::isValid() const { return m_isValid; }
e172vp::Hardware::QueueFamilies e172vp::GraphicsInstance::queueFamilies() const { return m_queueFamilies; }
std::vector<std::string> e172vp::GraphicsInstance::enabledValidationLayers() const { return m_enabledValidationLayers; }
vk::Queue e172vp::GraphicsInstance::graphicsQueue() const { return m_graphicsQueue; }
vk::Queue e172vp::GraphicsInstance::presentQueue() const { return m_presentQueue; }
e172vp::SwapChain e172vp::GraphicsInstance::swapChain() const { return m_swapChain; }
e172vp::CommandPool e172vp::GraphicsInstance::commandPool() const { return m_commandPool; }
e172vp::RenderPass e172vp::GraphicsInstance::renderPass() const { return m_renderPass; }
bool e172vp::GraphicsInstance::debugEnabled() const { return m_debugEnabled; }
e172vp::SwapChain::Settings e172vp::GraphicsInstance::swapChainSettings() const { return m_swapChainSettings; }

std::vector<std::string> e172vp::GraphicsInstance::pullErrors() {
    const auto r = m_errors;
    m_errors.clear();
    return r;
}

e172vp::GraphicsInstance::GraphicsInstance(const GraphicsInstanceCreateInfo &createInfo) {
    m_debugEnabled = createInfo.debugEnabled();
    e172vp::VulkanInstanceFactory vulkanInstanceFactory("test", VK_MAKE_VERSION(1, 0, 0));
    vulkanInstanceFactory.setRequiredExtensions(createInfo.requiredExtensions());
    vulkanInstanceFactory.setDebugEnabled(m_debugEnabled);
    m_vulkanInstance = vulkanInstanceFactory.create();
    if(!m_vulkanInstance) {
        m_errors.push_back("[error] Instance not created.");
    }

    if(createInfo.surfaceCreator()) {
        createInfo.surfaceCreator()(m_vulkanInstance, &m_surface);
        if(!m_surface) {
            m_errors.push_back("[error] Surface not created.");
            return;
        }
    } else {
        m_errors.push_back("[error] Surface creator not specified.");
        return;
    }

    m_physicalDevice = Hardware::findSuitablePhysicalDevice(m_vulkanInstance, m_surface, createInfo.requiredDeviceExtensions());
    if(!m_physicalDevice) {
        m_errors.push_back("[error] Suitable device not found.");
        return;
    }
    m_queueFamilies = e172vp::Hardware::queryQueueFamilies(m_physicalDevice, m_surface);

    e172vp::LogicDeviceFactory logicDeviceFactory;
    logicDeviceFactory.setQueueFamilies(m_queueFamilies);
    logicDeviceFactory.setValidationLayersEnabled(m_debugEnabled);
    logicDeviceFactory.setRequiredDeviceExtensions(createInfo.requiredDeviceExtensions());
    m_logicalDevice = logicDeviceFactory.create(m_physicalDevice);
    if(!m_logicalDevice) {
        m_errors.push_back("[error] Logic device not found.");
        return;
    }
    m_enabledValidationLayers = logicDeviceFactory.enabledValidationLayers();
    m_logicalDevice.getQueue(m_queueFamilies.graphicsFamily(), 0, &m_graphicsQueue);
    m_logicalDevice.getQueue(m_queueFamilies.presentFamily(), 0, &m_presentQueue);

    const auto swapChainSupportDetails = e172vp::Hardware::querySwapChainSupport(m_physicalDevice, m_surface);
    m_swapChainSettings = e172vp::SwapChain::createSettings(swapChainSupportDetails);
    m_swapChain = e172vp::SwapChain(m_logicalDevice, m_surface, m_queueFamilies, m_swapChainSettings);
    m_renderPass = e172vp::RenderPass(m_logicalDevice, m_swapChain);
    m_commandPool = e172vp::CommandPool(m_logicalDevice, m_queueFamilies, m_swapChain.imageCount());

    m_isValid = m_swapChain.isValid() && m_renderPass.isValid() && m_commandPool.isValid();
}

