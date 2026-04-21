#ifndef __MY_SIMPLE_RENDER_FACTORY_H__
#define __MY_SIMPLE_RENDER_FACTORY_H__

#include "my_device.h"
#include "my_game_object.h"
#include "my_pipeline.h"
#include "my_camera.h"
#include "my_frame_info.h"

// std
#include <memory>
#include <vector>

class MySimpleRenderFactory
{
public:
	MySimpleRenderFactory(MyDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~MySimpleRenderFactory();

	MySimpleRenderFactory(const MySimpleRenderFactory&) = delete;
	MySimpleRenderFactory& operator=(const MySimpleRenderFactory&) = delete;
	MySimpleRenderFactory(MySimpleRenderFactory&&) = delete;
	MySimpleRenderFactory& operator=(const MySimpleRenderFactory&&) = delete;

	void renderGameObjects(MyFrameInfo& frameInfo, std::vector<MyGameObject>& gameObjects);

private:
	void _createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void _createPipeline(VkRenderPass renderPass);

	MyDevice&                   m_myDevice;

	std::unique_ptr<MyPipeline> m_pMyPipeline;
	VkPipelineLayout            m_vkPipelineLayout;
};

#endif

