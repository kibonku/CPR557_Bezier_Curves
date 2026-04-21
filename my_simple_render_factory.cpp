#include "my_simple_render_factory.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

struct MySimplePushConstantData
{
    glm::mat4 modelMatrix{ 1.0f };
    glm::mat4 normalMatrix{ 1.0f };
};

MySimpleRenderFactory::MySimpleRenderFactory(MyDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
    : m_myDevice{ device } 
{
    _createPipelineLayout(globalSetLayout);
    _createPipeline(renderPass);
}

MySimpleRenderFactory::~MySimpleRenderFactory()
{
    vkDestroyPipelineLayout(m_myDevice.device(), m_vkPipelineLayout, nullptr);
}

void MySimpleRenderFactory::_createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(MySimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(m_myDevice.device(), &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void MySimpleRenderFactory::_createPipeline(VkRenderPass renderPass)
{
    assert(m_vkPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    MyPipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = m_vkPipelineLayout;

    m_pMyPipeline = std::make_unique<MyPipeline>(
        m_myDevice,
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipelineConfig);
}

void MySimpleRenderFactory::renderGameObjects(MyFrameInfo& frameInfo, std::vector<MyGameObject>& gameObjects)
{
    m_pMyPipeline->bind(frameInfo.commandBuffer);
    //auto projectionView = frameInfo.camera.projectionMatrix() * frameInfo.camera.viewMatrix();

    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_vkPipelineLayout,
        0, // only bind set 0 for now
        1, // count of 1
        &frameInfo.globalDescriptorSet,
        0,
        nullptr);

    for (auto& obj : gameObjects)
    {
        // Note: X to the right, Y up and Z out of the screen
        // obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.0005f, glm::two_pi<float>());  // Y up
        // obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.0005f, glm::two_pi<float>());  // X to the right
        // obj.transform.rotation.z = glm::mod(obj.transform.rotation.z + 0.0005f, glm::two_pi<float>());  // Z out of the screen

        if (obj.name() == std::string("bezier_surface") && obj.model != nullptr)
        {
            MySimplePushConstantData push{};

            // Note: do this for now to perform on CPU
            // We will do it later to perform it on GPU
        	push.modelMatrix = obj.transform.mat4();
        	push.normalMatrix = obj.transform.normalMatrix();

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                m_vkPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(MySimplePushConstantData),
                &push);

            obj.model->bind(frameInfo.commandBuffer);
            obj.model->draw(frameInfo.commandBuffer);
        }
    }
}

