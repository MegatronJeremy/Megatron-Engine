#include "EditorLayer.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "Snowstorm/Events/KeyEvent.h"
#include "Snowstorm/Events/MouseEvent.h"

namespace Snowstorm
{
	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
	}

	void EditorLayer::OnAttach()
	{
		SS_PROFILE_FUNCTION();

		m_ActiveScene = CreateRef<Scene>();
		m_SceneHierarchyPanel.setContext(m_ActiveScene);

		// Framebuffer setup
		{
			// TODO set this up with the current screen size
			m_FramebufferEntity = m_ActiveScene->CreateEntity("Framebuffer");
			m_FramebufferEntity.AddComponent<ViewportComponent>(glm::vec2{1280.0f, 720.0f});

			FramebufferSpecification fbSpec;
			fbSpec.Width = 1280;
			fbSpec.Height = 720;
			m_FramebufferEntity.AddComponent<FramebufferComponent>(Framebuffer::Create(fbSpec));
		}

		Ref<Texture2D> checkerboardTexture = Texture2D::Create("assets/textures/Checkerboard.png");

		// 3D Entities
		{
			auto& shaderLibrary = m_ActiveScene->getSingletonManager().GetSingleton<ShaderLibrarySingleton>();

			Ref<Shader> basicShader = shaderLibrary.Load("assets/shaders/BasicLit.glsl");

			const Ref<Material> redMaterial = CreateRef<Material>(basicShader);
			redMaterial->SetUniform("u_Color", glm::vec3(1.0f, 0.0f, 0.0f)); // Red color
			redMaterial->SetTexture("u_AlbedoTexture", checkerboardTexture);

			const Ref<Material> blueMaterial = CreateRef<Material>(basicShader);
			blueMaterial->SetUniform("u_Color", glm::vec3(0.0f, 0.0f, 1.0f)); // Blue color
			blueMaterial->SetTexture("u_AlbedoTexture", checkerboardTexture);

			auto blueCube = m_ActiveScene->CreateEntity("Blue Cube");

			blueCube.AddComponent<TransformComponent>();
			blueCube.AddComponent<MaterialComponent>(blueMaterial);
			blueCube.AddComponent<MeshComponent>(Mesh::CreateCube());
			blueCube.AddComponent<RenderTargetComponent>(m_FramebufferEntity);

			blueCube.GetComponent<TransformComponent>().Position -= 3.0f;

			auto redCube = m_ActiveScene->CreateEntity("Red Cube");

			redCube.AddComponent<TransformComponent>();
			redCube.AddComponent<MaterialComponent>(redMaterial);
			redCube.AddComponent<MeshComponent>(Mesh::CreateCube());
			redCube.AddComponent<RenderTargetComponent>(m_FramebufferEntity);

			redCube.GetComponent<TransformComponent>().Position += 3.0f;
		}

		// 2D Entities
		{
			auto checkerboardSquare = m_ActiveScene->CreateEntity("Amazing Square");

			checkerboardSquare.AddComponent<TransformComponent>();
			checkerboardSquare.AddComponent<SpriteComponent>(checkerboardTexture, 1.0f,
			                                                 glm::vec4{0.0f, 0.0f, 1.0f, 1.0f});
			checkerboardSquare.AddComponent<RenderTargetComponent>(m_FramebufferEntity);

			checkerboardSquare.GetComponent<TransformComponent>().Position[0] += 2.0f;

			auto redSquare = m_ActiveScene->CreateEntity("Red Square");

			redSquare.AddComponent<TransformComponent>();
			redSquare.AddComponent<SpriteComponent>(glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
			redSquare.AddComponent<RenderTargetComponent>(m_FramebufferEntity);

			m_SquareEntity = checkerboardSquare;
		}

		// Camera Entities
		{
			m_CameraEntity = m_ActiveScene->CreateEntity("Camera Entity");
			m_CameraEntity.AddComponent<TransformComponent>();
			m_CameraEntity.AddComponent<CameraComponent>();
			m_CameraEntity.AddComponent<CameraControllerComponent>();
			m_CameraEntity.AddComponent<RenderTargetComponent>(m_FramebufferEntity);

			m_CameraEntity.GetComponent<CameraComponent>().Camera.SetProjectionType(
				SceneCamera::ProjectionType::Perspective);
			m_CameraEntity.GetComponent<CameraComponent>().Camera.SetOrthographicFarClip(1000.0f);
			m_CameraEntity.GetComponent<TransformComponent>().Position.z = 15.0f;

			m_SecondCamera = m_ActiveScene->CreateEntity("Clip-Space Entity");
			m_SecondCamera.AddComponent<TransformComponent>();
			auto& cc = m_SecondCamera.AddComponent<CameraComponent>();
			m_SecondCamera.AddComponent<CameraControllerComponent>();
			m_SecondCamera.AddComponent<RenderTargetComponent>(m_FramebufferEntity);
			cc.Primary = false;
		}
	}

	void EditorLayer::OnDetach()
	{
		SS_PROFILE_FUNCTION();
	}

	void EditorLayer::OnUpdate(const Timestep ts)
	{
		SS_PROFILE_FUNCTION();

		// Update scene
		m_ActiveScene->OnUpdate(ts);
	}

	void EditorLayer::OnImGuiRender()
	{
		SS_PROFILE_FUNCTION();

		static bool dockspaceOpen = true;
		static bool optFullscreen = true;
		static bool optPadding = false;
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (optFullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove;
			windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspaceFlags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
			windowFlags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		if (!optPadding)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		}
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, windowFlags);
		if (!optPadding)
		{
			ImGui::PopStyleVar();
		}

		if (optFullscreen)
		{
			ImGui::PopStyleVar(2);
		}

		// Submit the DockSpace
		if (const ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			const ImGuiID dockspaceID = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					Application::Get().Close();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		ImGui::End();

		m_SceneHierarchyPanel.onImGuiRender();

		ImGui::Begin("Settings");

		const auto stats = Renderer2D::GetStats();
		ImGui::Text("Renderer2D Stats:");
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

		if (m_SquareEntity)
		{
			ImGui::Separator();
			ImGui::Text("%s", m_SquareEntity.GetComponent<TagComponent>().Tag.c_str());

			auto& squareColor = m_SquareEntity.GetComponent<SpriteComponent>().TintColor;
			ImGui::ColorEdit4("Square Color", value_ptr(squareColor));
			ImGui::Separator();
		}

		ImGui::DragFloat3("Camera Position", value_ptr(
			                  m_CameraEntity.GetComponent<TransformComponent>().Position));

		if (ImGui::Checkbox("Camera A", &m_PrimaryCamera))
		{
			m_CameraEntity.GetComponent<CameraComponent>().Primary = m_PrimaryCamera;
			m_SecondCamera.GetComponent<CameraComponent>().Primary = !m_PrimaryCamera;
		}

		{
			auto& camera = m_SecondCamera.GetComponent<CameraComponent>().Camera;
			float orthoSize = camera.GetOrthographicSize();
			if (ImGui::DragFloat("Second Camera Ortho Size", &orthoSize))
				camera.SetOrthographicSize(orthoSize);
		}

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
		ImGui::Begin("Viewport");

		// TODO move this to some sort of event system?
		auto& viewportComponent = m_FramebufferEntity.GetComponent<ViewportComponent>();
		const auto& framebufferComponent = m_FramebufferEntity.GetComponent<FramebufferComponent>();

		viewportComponent.Focused = ImGui::IsWindowFocused();
		viewportComponent.Hovered = ImGui::IsWindowHovered();
		Application::Get().GetImGuiLayer()->BlockEvents(!viewportComponent.Focused || !viewportComponent.Hovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		viewportComponent.Size = {viewportPanelSize.x, viewportPanelSize.y};

		const uint32_t textureID = framebufferComponent.Framebuffer->GetColorAttachmentRendererID();
		ImGui::Image(reinterpret_cast<ImTextureID>(textureID),
		             ImVec2{viewportComponent.Size.x, viewportComponent.Size.y}, ImVec2{0, 1},
		             ImVec2{1, 0});
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		auto& eventsHandler = m_ActiveScene->getSingletonManager().GetSingleton<EventsHandlerSingleton>();

		// TODO have to make this better
		static const std::unordered_map<EventType, std::function<void(Event&)>> eventMap = {
			{
				EventType::MouseScrolled, [&eventsHandler](Event& e)
				{
					eventsHandler.PushEvent<MouseScrolledEvent>(dynamic_cast<MouseScrolledEvent&>(e));
				}
			}
		};

		// Check if the event type exists in the map
		if (const auto it = eventMap.find(event.GetEventType()); it != eventMap.end())
		{
			it->second(event); // Call the corresponding function
		}
		else
		{
			// Don't do this for now... mouse moved events etc...
			// SS_ASSERT(false, "Unknown event");
		}
	}
}
