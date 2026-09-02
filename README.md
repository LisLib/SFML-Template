# SFML 3.1.0 Backend for Dear ImGui 1.92.9b (`imgui-SFML.cpp`)

A custom, modern C++ implementation bridging **SFML 3.1.0** and **Dear ImGui 1.92.9b** without legacy compatibility wrappers. This file serves as both the platform input handler and the hardware-accelerated 2D renderer.

---

## Key Architectural Highlights

### 1. Modern SFML 3.1.0 Event Dispatching
SFML 3.x transitions from C-style unions to type-safe event variants (`std::variant`). `imgui-SFML.cpp` natively adapts to this via member template methods:
* **Event Inspection:** Uses `event.getIf<sf::Event::Type>()` and `event.is<sf::Event::Type>()` rather than `event.type == sf::Event::...`.
* **Keyboard Navigation:** Maps both `sf::Keyboard::Key` (legacy/layout-dependent) and `sf::Keyboard::Scancode` (layout-independent hardware scancodes) directly to `ImGuiKey` primitives.
* **Modifier Forwarding:** Handles `control`, `shift`, `alt`, and `system` modifiers in a single pass via `io.AddKeyEvent(ImGuiMod_*, bool)`.
* **Window & Viewport State:** Captures focus shifts (`sf::Event::FocusGained`/`FocusLost`), window resize events (`sf::Event::Resized`), and pointer boundary exits (`sf::Event::MouseLeft`).

### 2. Dear ImGui 1.92+ Dynamic Texture Lifecycle
Dear ImGui version 1.92 introduces a managed multi-texture lifecycle architecture via `ImTextureData` and `ImGuiBackendFlags_RendererHasTextures`:
* **Zero Custom Loader Glue:** Instead of forcing manual font atlas baking through outdated OpenGL ID conversions, `UpdateTexture(ImTextureData*)` delegates memory management directly through ImGui draw data.
* **Partial Texture Updates:** Listens for `ImTextureStatus_WantCreate`, `ImTextureStatus_WantUpdates`, and `ImTextureStatus_WantDestroy`. Sub-rectangle font updates (`ImTextureRect`) upload straight to `sf::Texture::update()` without reallocating whole atlas buffers.
* **Staged CPU Buffers:** Reuses a static staging buffer (`s_pixelStaging`) to avoid frame-time memory allocations when copying fragmented texture rects.

### 3. Native Platform Clipboard & Lazy Cursor Caching
* **ImGuiPlatformIO Hooks:** Integrates system clipboard reading and writing using `ImGuiPlatformIO` hooks (`Platform_SetClipboardTextFn` and `Platform_GetClipboardTextFn`) backed by `sf::Clipboard` UTF-8 conversions (`sf::String::fromUtf8`).
* **Lazy System Cursors:** Hardware mouse shapes (`Arrow`, `TextInput`, `ResizeAll`, `Hand`, etc.) are cached in an `std::array<std::optional<sf::Cursor>, ImGuiMouseCursor_COUNT>` on demand, bypassing redundant OS handle allocations each frame.

### 4. Direct 2D Batching with Hardware Scissor Rects
Rather than issuing individual draw calls per element, the renderer batches geometry:
* **Pre-allocated Buffers:** Utilizes a persistent `std::vector<sf::Vertex>` buffer resized once per `ImDrawCmd` to eliminate heap churn.
* **Coordinate Inversion & Scissoring:** Calculates clipped bounding boxes per command and applies them through OpenGL hardware testing (`glEnable(GL_SCISSOR_TEST)` / `glScissor`), accurately accounting for SFML's inverted Y origin (bottom-left coordinate origin in OpenGL).
* **View State Isolation:** Backs up the active `sf::RenderTarget::getView()` before rendering and restores it after drawing, guaranteeing that custom scene cameras and UI projections do not contaminate one another.

---

## Core Lifecycle API

```cpp
namespace ImGui::SFML {
    // Context Initialization
    bool Init(sf::RenderWindow& window, bool loadDefaultFont = true);
    bool Init(sf::Window& window, sf::RenderTarget& target, bool loadDefaultFont = true);

    // Event & Frame Pipeline
    void ProcessEvent(const sf::Window& window, const sf::Event& event);
    void Update(sf::RenderWindow& window, sf::Time dt);
    void Update(sf::Window& window, sf::RenderTarget& target, sf::Time dt);

    // Rasterization
    void Render(sf::RenderTarget& target);
    void Render(); // Renders to the current target bound during Update

    // Teardown
    void Shutdown();
    void Shutdown(const sf::Window& window);
}
```

---

## Integration Example

```cpp
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include "imgui-SFML.h"

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "SFML 3.1.0 + ImGui 1.92.9b");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window, true)) {
        return -1;
    }

    sf::Clock deltaClock;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // Advance Dear ImGui state machine
        sf::Time dt = deltaClock.restart();
        ImGui::SFML::Update(window, dt);

        // Submit UI
        ImGui::Begin("Performance Statistics");
        ImGui::Text("Frametime: %.3f ms (%.1f FPS)", dt.asSeconds() * 1000.0f, 1.0f / dt.asSeconds());
        ImGui::End();

        // Render pass
        window.clear(sf::Color(24, 26, 32));
        
        // Render your game/graphics objects here
        // window.draw(...);

        // Render ImGui draw lists
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
```

---

## Technical Specifications

| Component | Target Version | Key Interop Feature |
| :--- | :--- | :--- |
| **Dear ImGui** | `v1.92.9b` | `ImTextureData` lifecycle, `ImGuiPlatformIO` clipboard, dynamic vector scaling |
| **SFML** | `3.1.0` | Variant-based `sf::Event`, modular `sf::Cursor::createFromSystem`, RAII texture management |
| **C++ Standard** | `C++20` or later | `[[nodiscard]]`, `constexpr` branch evaluations, fixed-width standard integers |
