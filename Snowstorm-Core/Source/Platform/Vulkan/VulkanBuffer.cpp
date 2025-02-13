#include "pch.h"
#include "Platform/Vulkan/VulkanBuffer.h"

#include "VulkanDevice.h"
#include "VulkanCommandPool.h"

namespace Snowstorm
{
	/////////////////////////////////////////////////////////////////////////////
	// Buffer Utils /////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	uint32_t VulkanBufferUtils::FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties)
	{
		const VkPhysicalDevice physicalDevice = VulkanInstance::GetInstance()->GetVulkanDevice()->GetVkPhysicalDevice();

		// graphics cards can offer different types of memory to allocate from
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// right now we'll only concern ourselves with the type of memory and not the heap it comes from
		// we also need to be able to write our vertex data to that memory, so we check additional bits
		// so we check if ALL the properties are present
		// TODO this impacts performance, check it out again later
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		SS_CORE_ASSERT(false, "Failed to find suitable memory type!");
		return 0;
	}


	void VulkanBufferUtils::CreateBuffer(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage,
	                                     const VkMemoryPropertyFlags properties, VkBuffer& buffer,
	                                     VkDeviceMemory& bufferMemory)
	{
		// helper function for creating a vertex buffer
		// last two parameters -> output parameters to write to
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // will only be used from the graphics queue
		// flags - for sparse buffer memory, we will leave it at 0 for now

		VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
		SS_CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer!");

		// allocate the memory on the GPU for the buffer
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
		                                           properties);


		// TODO do not use vkAllocateMemory for every individual buffer
		// TODO max number of simultaneous memory allocations may be as low as 4096
		// TODO create a custom allocator that splits up a single allocation among different objects by using the offset param
		// TODO see: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
		// TODO from: https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer
		result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
		SS_CORE_ASSERT(result == VK_SUCCESS, "Failed to allocate vertex buffer memory!");

		// if everything was successful, we can now associate this memory with the buffer
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void VulkanBufferUtils::CopyBuffer(const VkDevice device, const VkBuffer srcBuffer, const VkBuffer dstBuffer,
	                                   const VkDeviceSize size)
	{
		// Memory transfer operations executed using command buffer, just like drawing commands
		// First allocate a temporary command buffer
		// TODO create a separate command pool for these kinds of short-lived buffers
		// TODO using VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag during command pool generation
		const VkCommandPool commandPool = *VulkanInstance::GetInstance()->GetVulkanCommandPool();
		const VkQueue graphicsQueue = VulkanInstance::GetInstance()->GetVulkanDevice()->GetVkGraphicsQueue();

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		// Immediately record the command buffer
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // good practice - tell the driver our intent

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;

		// contents are transferred using this command
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		// done with recording
		vkEndCommandBuffer(commandBuffer);

		// Now execute the command buffer to complete the transfer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

		// TODO use a fence instead (vkWaitForFences) - we can schedule multiple transfers simultaneously
		// TODO and wait for all of them to complete, instead of doing this one task at a time
		// TODO see: https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer
		vkQueueWaitIdle(graphicsQueue);

		// clean up the command buffer used for the transfer operation
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	/////////////////////////////////////////////////////////////////////////////
	// VertexBuffer /////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	VulkanVertexBuffer::VulkanVertexBuffer(const uint32_t size)
		: VulkanVertexBuffer(nullptr, size)
	{
	}

	VulkanVertexBuffer::VulkanVertexBuffer(const void* data, const uint32_t size)
	{
		SS_PROFILE_FUNCTION();

		m_Device = VulkanInstance::GetInstance()->GetVulkanDevice()->GetVkDevice();

		SetData(data, size);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		SS_PROFILE_FUNCTION();

		vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
		vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
	}

	void VulkanVertexBuffer::Bind() const
	{
		SS_PROFILE_FUNCTION();

		// if everything was successful, we can now associate this memory with the buffer
	}

	void VulkanVertexBuffer::Unbind() const
	{
		SS_PROFILE_FUNCTION();
	}

	void VulkanVertexBuffer::SetData(const void* vertices, const uint32_t size)
	{
		SS_PROFILE_FUNCTION();

		// TODO fix all this crap
		if (size == 0)
		{
			return;
		}

		bool verticesCreated = false;
		if (vertices == nullptr)
		{
			verticesCreated = true;
			vertices = new char[size];
		}

		const VkDeviceSize bufferSize = size;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		// we only use this to write from the CPU
		VulkanBufferUtils::CreateBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                                stagingBuffer, stagingBufferMemory);


		// now actually copy the vertex data to the buffer
		// by mapping the buffer memory into CPU accessible memory with vkMapMemory
		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices, bufferSize);
		vkUnmapMemory(m_Device, stagingBufferMemory);
		// the driver may not immediately copy the data into buffer memory -> because of caching for example
		// so we use memory that is host coherent: VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		// OR we call vkFlushMappedMemoryRanged after writing to the mapped memory
		// AND call vkInvalidateMappedMemoryRanges before reading from the mapped memory

		// This buffer will be optimally used on the GPU - DEVICE_LOCAL_BIT in MEMORY
		VulkanBufferUtils::CreateBuffer(m_Device, bufferSize,
		                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		                                m_VertexBuffer, m_VertexBufferMemory);

		// Now we need to transfer from the SRC buffer to the DST buffer
		VulkanBufferUtils::CopyBuffer(m_Device, stagingBuffer, m_VertexBuffer, bufferSize);

		// Finally, clean the staging buffer up
		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);

		if (verticesCreated)
		{
			SS_CORE_INFO("Created vertices");
			delete[] static_cast<const char*>(vertices);
		}
	}

	void VulkanVertexBuffer::SetSubData(const void* vertices, uint32_t size, uint32_t offset)
	{
		SS_CORE_ASSERT(false, "Not implemented");
	}

	/////////////////////////////////////////////////////////////////////////////
	// IndexBuffer //////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	VulkanIndexBuffer::VulkanIndexBuffer(const uint32_t* indices, const uint32_t count)
		: m_Count(count)
	{
		SS_PROFILE_FUNCTION();

		m_Device = VulkanInstance::GetInstance()->GetVulkanDevice()->GetVkDevice();

		const VkDeviceSize bufferSize = sizeof(indices[0]) * count;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		VulkanBufferUtils::CreateBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                                stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices, bufferSize);
		vkUnmapMemory(m_Device, stagingBufferMemory);

		// This is a difference -> VK_BUFFER_USAGE_INDEX_BUFFER_BIT -> a single boolean!
		VulkanBufferUtils::CreateBuffer(m_Device, bufferSize,
		                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		                                m_IndexBuffer, m_IndexBufferMemory);

		VulkanBufferUtils::CopyBuffer(m_Device, stagingBuffer, m_IndexBuffer, bufferSize);

		vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
		vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		SS_PROFILE_FUNCTION();

		vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
		vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
	}

	void VulkanIndexBuffer::Bind() const
	{
		SS_PROFILE_FUNCTION();
	}

	void VulkanIndexBuffer::Unbind() const
	{
		SS_PROFILE_FUNCTION();
	}

	void VulkanIndexBuffer::SetData(const void* data, uint32_t size)
	{
	}

	void VulkanIndexBuffer::SetSubData(const void* data, uint32_t size, uint32_t offset)
	{
		SS_CORE_ASSERT(false, "Not implemented");
	}

	/////////////////////////////////////////////////////////////////////////////
	// UniformBuffer ////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////

	VulkanUniformBuffer::VulkanUniformBuffer(const VkDeviceSize bufferSize)
		: m_BufferSize(bufferSize)
	{
		m_Device = *VulkanInstance::GetInstance()->GetVulkanDevice();

		VulkanBufferUtils::CreateBuffer(m_Device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffer,
		                                m_UniformBufferMemory);

		vkMapMemory(m_Device, m_UniformBufferMemory, 0, bufferSize, 0, &m_MappedMemory);
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		vkDestroyBuffer(m_Device, m_UniformBuffer, nullptr);
		vkFreeMemory(m_Device, m_UniformBufferMemory, nullptr);
	}

	void VulkanUniformBuffer::SetData(const void* data, const uint32_t dataSize) const
	{
		memcpy_s(m_MappedMemory, dataSize, data, dataSize);
	}

	void VulkanUniformBuffer::EnqueueData(const void* data, const uint32_t dataSize)
	{
		SS_CORE_INFO("Enqueuing data for uniform buffer!");

		auto queuedData = new char[dataSize];
		memcpy_s(queuedData, dataSize, data, dataSize);

		m_DataQueue.emplace_back(queuedData, dataSize);
	}

	void VulkanUniformBuffer::SetDataFromQueue()
	{
		SS_CORE_INFO("Setting data from queue for uniform buffer!");

		if (m_DataQueue.empty())
		{
			return;
		}

		auto [data, dataSize] = m_DataQueue.front();
		SetData(data, dataSize);

		delete[] data;
	}
}
