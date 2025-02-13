#pragma once

#include "Snowstorm/Render/RendererAPI.h"

namespace Snowstorm
{
	class OpenGLRendererAPI final : public RendererAPI
	{
	public:
		~OpenGLRendererAPI() override = default;

		void Init() override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		void SetClearColor(const glm::vec4& color) override;
		void Clear() override;

		void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		void DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount) override;
	};
}
