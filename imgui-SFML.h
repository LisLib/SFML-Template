#ifndef IMGUI_SFML_H
#define IMGUI_SFML_H

#include "imgui-SFML_export.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>

struct ImTextureData;

namespace sf {
class RenderTarget;
class Texture;
}

namespace ImGui::SFML {

IMGUI_SFML_API bool Init(sf::RenderWindow& window, bool loadDefaultFont = true);
IMGUI_SFML_API bool Init(sf::Window& window, sf::RenderTarget& target, bool loadDefaultFont = true);

IMGUI_SFML_API void ProcessEvent(const sf::Window& window, const sf::Event& event);

IMGUI_SFML_API void Update(sf::RenderWindow& window, sf::Time dt);
IMGUI_SFML_API void Update(sf::Window& window, sf::RenderTarget& target, sf::Time dt);
IMGUI_SFML_API void Update(const sf::Vector2i& mousePos, const sf::Vector2f& displaySize, sf::Time dt);

IMGUI_SFML_API void Render(sf::RenderTarget& target);
IMGUI_SFML_API void Render();

IMGUI_SFML_API void Shutdown(const sf::Window& window);
IMGUI_SFML_API void Shutdown();

// Texture back-end entry point (ImGuiBackendFlags_RendererHasTextures, ImGui >= 1.92).
// Called automatically by Render(), exposed for custom render loops.
IMGUI_SFML_API void UpdateTexture(ImTextureData* textureData);

IMGUI_SFML_API void UpdateCursor(sf::Window& window);
IMGUI_SFML_API void SetCurrentWindow(sf::Window& window);

} // namespace ImGui::SFML

#endif // IMGUI_SFML_H
