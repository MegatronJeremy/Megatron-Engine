#include "pch.h"
#include "Platform/Vulkan/VulkanRendererAPI.h"

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanInstance.h"
#include "VulkanSwapChain.h"


namespace Snowstorm
{
	void VulkanRendererAPI::Init()
	{
		SS_PROFILE_FUNCTION();

		VkDevice device = VulkanInstance::GetInstance()->GetVulkanDevice()->GetVkDevice();
		VkCommandPool commandPool = VulkanInstance::GetInstance()->GetVulkanCommandPool()->GetVkCommandPool();

		m_VulkanCommandBuffer = CreateScope<VulkanCommandBuffers>(device, commandPool, 1);
	}

	void VulkanRendererAPI::SetViewport(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height)
	{
		// Set viewport
		// VkViewport viewport{};
		// viewport.x = static_cast<float>(x);
		// viewport.y = static_cast<float>(y);
		// viewport.width = static_cast<float>(width);
		// viewport.height = static_cast<float>(height);
		// viewport.minDepth = 0.0f;
		// viewport.maxDepth = 1.0f;
		//
		// vkCmdSetViewport((*m_VulkanCommandBuffer)[0], 0, 1, &viewport);
	}

	void VulkanRendererAPI::SetClearColor(const glm::vec4& color)
	{
		// Set clear color
		// const VkClearColorValue clearColor = {{color.r, color.g, color.b, color.a}};
		// VkImageSubresourceRange subresourceRange = {};
		// subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// subresourceRange.baseMipLevel = 0;
		// subresourceRange.levelCount = 1;
		// subresourceRange.baseArrayLayer = 0;
		// subresourceRange.layerCount = 1;
		//
		// vkCmdClearColorImage((*m_VulkanCommandBuffer)[0], swapChainImages[currentImage],
		//                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//                      &clearColor, 1, &subresourceRange);
	}

	void VulkanRendererAPI::Clear()
	{
		// Clear color and depth
		// VkClearAttachment clearAttachments[2] = {};
		// clearAttachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// clearAttachments[0].clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
		// clearAttachments[0].colorAttachment = 0;
		// clearAttachments[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		// clearAttachments[1].clearValue.depthStencil = {1.0f, 0};
		//
		// VkClearRect clearRect = {};
		// clearRect.layerCount = 1;
		// clearRect.rect.offset = {0, 0};
		// clearRect.rect.extent = swapChainExtent;
		//
		// vkCmdClearAttachments((*m_VulkanCommandBuffer)[0], 2, clearAttachments, 1, &clearRect);
	}

	void VulkanRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, const uint32_t indexCount)
	{
		// Draw indexed
		VulkanSwapChainQueue::GetInstance()->AddVertexArray(vertexArray);
	}
}
