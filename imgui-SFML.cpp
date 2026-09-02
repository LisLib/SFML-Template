#include "imgui-SFML.h"
#include <imgui.h>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {
sf::Window* s_currentWindow = nullptr;
sf::RenderTarget* s_currentTarget = nullptr;

// Reused across frames to eliminate per-draw-command heap allocations
std::vector<sf::Vertex> s_vertexBuffer;
std::vector<std::uint8_t> s_pixelStaging;

// System cursors are cached lazily to avoid redundant OS handle allocations
std::array<std::optional<sf::Cursor>, ImGuiMouseCursor_COUNT> s_cursors;
ImGuiMouseCursor s_lastCursor = ImGuiMouseCursor_COUNT;
bool s_lastCursorVisible = true;

[[nodiscard]] sf::Texture* textureFromID(ImTextureID id) noexcept {
    return reinterpret_cast<sf::Texture*>(static_cast<std::uintptr_t>(id));
}

[[nodiscard]] constexpr ImGuiKey keyToImGuiKey(sf::Keyboard::Key key) noexcept {
    switch (key) {
        case sf::Keyboard::Key::A: return ImGuiKey_A;
        case sf::Keyboard::Key::B: return ImGuiKey_B;
        case sf::Keyboard::Key::C: return ImGuiKey_C;
        case sf::Keyboard::Key::D: return ImGuiKey_D;
        case sf::Keyboard::Key::E: return ImGuiKey_E;
        case sf::Keyboard::Key::F: return ImGuiKey_F;
        case sf::Keyboard::Key::G: return ImGuiKey_G;
        case sf::Keyboard::Key::H: return ImGuiKey_H;
        case sf::Keyboard::Key::I: return ImGuiKey_I;
        case sf::Keyboard::Key::J: return ImGuiKey_J;
        case sf::Keyboard::Key::K: return ImGuiKey_K;
        case sf::Keyboard::Key::L: return ImGuiKey_L;
        case sf::Keyboard::Key::M: return ImGuiKey_M;
        case sf::Keyboard::Key::N: return ImGuiKey_N;
        case sf::Keyboard::Key::O: return ImGuiKey_O;
        case sf::Keyboard::Key::P: return ImGuiKey_P;
        case sf::Keyboard::Key::Q: return ImGuiKey_Q;
        case sf::Keyboard::Key::R: return ImGuiKey_R;
        case sf::Keyboard::Key::S: return ImGuiKey_S;
        case sf::Keyboard::Key::T: return ImGuiKey_T;
        case sf::Keyboard::Key::U: return ImGuiKey_U;
        case sf::Keyboard::Key::V: return ImGuiKey_V;
        case sf::Keyboard::Key::W: return ImGuiKey_W;
        case sf::Keyboard::Key::X: return ImGuiKey_X;
        case sf::Keyboard::Key::Y: return ImGuiKey_Y;
        case sf::Keyboard::Key::Z: return ImGuiKey_Z;
        case sf::Keyboard::Key::Num0: return ImGuiKey_0;
        case sf::Keyboard::Key::Num1: return ImGuiKey_1;
        case sf::Keyboard::Key::Num2: return ImGuiKey_2;
        case sf::Keyboard::Key::Num3: return ImGuiKey_3;
        case sf::Keyboard::Key::Num4: return ImGuiKey_4;
        case sf::Keyboard::Key::Num5: return ImGuiKey_5;
        case sf::Keyboard::Key::Num6: return ImGuiKey_6;
        case sf::Keyboard::Key::Num7: return ImGuiKey_7;
        case sf::Keyboard::Key::Num8: return ImGuiKey_8;
        case sf::Keyboard::Key::Num9: return ImGuiKey_9;
        case sf::Keyboard::Key::Escape: return ImGuiKey_Escape;
        case sf::Keyboard::Key::LControl: return ImGuiKey_LeftCtrl;
        case sf::Keyboard::Key::LShift: return ImGuiKey_LeftShift;
        case sf::Keyboard::Key::LAlt: return ImGuiKey_LeftAlt;
        case sf::Keyboard::Key::LSystem: return ImGuiKey_LeftSuper;
        case sf::Keyboard::Key::RControl: return ImGuiKey_RightCtrl;
        case sf::Keyboard::Key::RShift: return ImGuiKey_RightShift;
        case sf::Keyboard::Key::RAlt: return ImGuiKey_RightAlt;
        case sf::Keyboard::Key::RSystem: return ImGuiKey_RightSuper;
        case sf::Keyboard::Key::Menu: return ImGuiKey_Menu;
        case sf::Keyboard::Key::LBracket: return ImGuiKey_LeftBracket;
        case sf::Keyboard::Key::RBracket: return ImGuiKey_RightBracket;
        case sf::Keyboard::Key::Semicolon: return ImGuiKey_Semicolon;
        case sf::Keyboard::Key::Comma: return ImGuiKey_Comma;
        case sf::Keyboard::Key::Period: return ImGuiKey_Period;
        case sf::Keyboard::Key::Apostrophe: return ImGuiKey_Apostrophe;
        case sf::Keyboard::Key::Slash: return ImGuiKey_Slash;
        case sf::Keyboard::Key::Backslash: return ImGuiKey_Backslash;
        case sf::Keyboard::Key::Equal: return ImGuiKey_Equal;
        case sf::Keyboard::Key::Hyphen: return ImGuiKey_Minus;
        case sf::Keyboard::Key::Space: return ImGuiKey_Space;
        case sf::Keyboard::Key::Enter: return ImGuiKey_Enter;
        case sf::Keyboard::Key::Backspace: return ImGuiKey_Backspace;
        case sf::Keyboard::Key::Tab: return ImGuiKey_Tab;
        case sf::Keyboard::Key::PageUp: return ImGuiKey_PageUp;
        case sf::Keyboard::Key::PageDown: return ImGuiKey_PageDown;
        case sf::Keyboard::Key::End: return ImGuiKey_End;
        case sf::Keyboard::Key::Home: return ImGuiKey_Home;
        case sf::Keyboard::Key::Insert: return ImGuiKey_Insert;
        case sf::Keyboard::Key::Delete: return ImGuiKey_Delete;
        case sf::Keyboard::Key::Left: return ImGuiKey_LeftArrow;
        case sf::Keyboard::Key::Right: return ImGuiKey_RightArrow;
        case sf::Keyboard::Key::Up: return ImGuiKey_UpArrow;
        case sf::Keyboard::Key::Down: return ImGuiKey_DownArrow;
        case sf::Keyboard::Key::F1: return ImGuiKey_F1;
        case sf::Keyboard::Key::F2: return ImGuiKey_F2;
        case sf::Keyboard::Key::F3: return ImGuiKey_F3;
        case sf::Keyboard::Key::F4: return ImGuiKey_F4;
        case sf::Keyboard::Key::F5: return ImGuiKey_F5;
        case sf::Keyboard::Key::F6: return ImGuiKey_F6;
        case sf::Keyboard::Key::F7: return ImGuiKey_F7;
        case sf::Keyboard::Key::F8: return ImGuiKey_F8;
        case sf::Keyboard::Key::F9: return ImGuiKey_F9;
        case sf::Keyboard::Key::F10: return ImGuiKey_F10;
        case sf::Keyboard::Key::F11: return ImGuiKey_F11;
        case sf::Keyboard::Key::F12: return ImGuiKey_F12;
        case sf::Keyboard::Key::Grave: return ImGuiKey_GraveAccent;
        case sf::Keyboard::Key::Pause: return ImGuiKey_Pause;
        case sf::Keyboard::Key::Numpad0: return ImGuiKey_Keypad0;
        case sf::Keyboard::Key::Numpad1: return ImGuiKey_Keypad1;
        case sf::Keyboard::Key::Numpad2: return ImGuiKey_Keypad2;
        case sf::Keyboard::Key::Numpad3: return ImGuiKey_Keypad3;
        case sf::Keyboard::Key::Numpad4: return ImGuiKey_Keypad4;
        case sf::Keyboard::Key::Numpad5: return ImGuiKey_Keypad5;
        case sf::Keyboard::Key::Numpad6: return ImGuiKey_Keypad6;
        case sf::Keyboard::Key::Numpad7: return ImGuiKey_Keypad7;
        case sf::Keyboard::Key::Numpad8: return ImGuiKey_Keypad8;
        case sf::Keyboard::Key::Numpad9: return ImGuiKey_Keypad9;
        case sf::Keyboard::Key::Divide: return ImGuiKey_KeypadDivide;
        case sf::Keyboard::Key::Multiply: return ImGuiKey_KeypadMultiply;
        case sf::Keyboard::Key::Subtract: return ImGuiKey_KeypadSubtract;
        case sf::Keyboard::Key::Add: return ImGuiKey_KeypadAdd;
        default: return ImGuiKey_None;
    }
}

[[nodiscard]] constexpr ImGuiKey scancodeToImGuiKey(sf::Keyboard::Scancode scancode) noexcept {
    switch (scancode) {
        case sf::Keyboard::Scan::CapsLock: return ImGuiKey_CapsLock;
        case sf::Keyboard::Scan::ScrollLock: return ImGuiKey_ScrollLock;
        case sf::Keyboard::Scan::NumLock: return ImGuiKey_NumLock;
        case sf::Keyboard::Scan::PrintScreen: return ImGuiKey_PrintScreen;
        default: return ImGuiKey_None;
    }
}

[[nodiscard]] constexpr int mouseButtonToImGuiButton(sf::Mouse::Button button) noexcept {
    switch (button) {
        case sf::Mouse::Button::Left: return 0;
        case sf::Mouse::Button::Right: return 1;
        case sf::Mouse::Button::Middle: return 2;
        case sf::Mouse::Button::Extra1: return 3;
        case sf::Mouse::Button::Extra2: return 4;
        default: return -1;
    }
}
} // namespace

namespace ImGui::SFML {

bool Init(sf::RenderWindow& window, bool loadDefaultFont) {
    SetCurrentWindow(window);
    return Init(window, window, loadDefaultFont);
}

bool Init(sf::Window& window, sf::RenderTarget& target, bool loadDefaultFont) {
    SetCurrentWindow(window);
    s_currentTarget = &target;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl_sfml3";
    io.BackendRendererName = "imgui_impl_sfml3";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos |
                       ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

    // Clipboard hooks moved to ImGuiPlatformIO in Dear ImGui 1.91+
    ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
    platformIO.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) {
        sf::Clipboard::setString(sf::String::fromUtf8(text, text + std::strlen(text)));
    };
    platformIO.Platform_GetClipboardTextFn = [](ImGuiContext*) -> const char* {
        static std::string clipboardBuffer;
        const auto u8str = sf::Clipboard::getString().toUtf8();
        clipboardBuffer.assign(reinterpret_cast<const char*>(u8str.data()), u8str.size());
        return clipboardBuffer.c_str();
    };

    if (loadDefaultFont) {
        io.Fonts->AddFontDefault();
    }

    const sf::Vector2u size = target.getSize();
    io.DisplaySize = ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
    return true;
}

void ProcessEvent(const sf::Window& /*window*/, const sf::Event& event) {
    ImGuiIO& io = ImGui::GetIO();

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        io.AddKeyEvent(ImGuiMod_Ctrl, key->control);
        io.AddKeyEvent(ImGuiMod_Shift, key->shift);
        io.AddKeyEvent(ImGuiMod_Alt, key->alt);
        io.AddKeyEvent(ImGuiMod_Super, key->system);

        ImGuiKey imguiKey = keyToImGuiKey(key->code);
        if (imguiKey == ImGuiKey_None) {
            imguiKey = scancodeToImGuiKey(key->scancode);
        }
        if (imguiKey != ImGuiKey_None) {
            io.AddKeyEvent(imguiKey, true);
        }
    } else if (const auto* key = event.getIf<sf::Event::KeyReleased>()) {
        io.AddKeyEvent(ImGuiMod_Ctrl, key->control);
        io.AddKeyEvent(ImGuiMod_Shift, key->shift);
        io.AddKeyEvent(ImGuiMod_Alt, key->alt);
        io.AddKeyEvent(ImGuiMod_Super, key->system);

        ImGuiKey imguiKey = keyToImGuiKey(key->code);
        if (imguiKey == ImGuiKey_None) {
            imguiKey = scancodeToImGuiKey(key->scancode);
        }
        if (imguiKey != ImGuiKey_None) {
            io.AddKeyEvent(imguiKey, false);
        }
    } else if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
        if (text->unicode >= 32 && text->unicode != 127) {
            io.AddInputCharacter(static_cast<unsigned int>(text->unicode));
        }
    } else if (const auto* mouseBtn = event.getIf<sf::Event::MouseButtonPressed>()) {
        const int button = mouseButtonToImGuiButton(mouseBtn->button);
        if (button != -1) io.AddMouseButtonEvent(button, true);
    } else if (const auto* mouseBtn = event.getIf<sf::Event::MouseButtonReleased>()) {
        const int button = mouseButtonToImGuiButton(mouseBtn->button);
        if (button != -1) io.AddMouseButtonEvent(button, false);
    } else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        io.AddMousePosEvent(static_cast<float>(mouseMoved->position.x), static_cast<float>(mouseMoved->position.y));
    } else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (wheel->wheel == sf::Mouse::Wheel::Vertical) {
            io.AddMouseWheelEvent(0.0f, wheel->delta);
        } else if (wheel->wheel == sf::Mouse::Wheel::Horizontal) {
            io.AddMouseWheelEvent(wheel->delta, 0.0f);
        }
    } else if (event.is<sf::Event::FocusGained>()) {
        io.AddFocusEvent(true);
    } else if (event.is<sf::Event::FocusLost>()) {
        io.AddFocusEvent(false);
    } else if (event.is<sf::Event::MouseLeft>()) {
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    } else if (const auto* resized = event.getIf<sf::Event::Resized>()) {
        io.DisplaySize = ImVec2(static_cast<float>(resized->size.x), static_cast<float>(resized->size.y));
    }
}

void Update(sf::RenderWindow& window, sf::Time dt) {
    Update(window, window, dt);
}

void Update(sf::Window& window, sf::RenderTarget& target, sf::Time dt) {
    SetCurrentWindow(window);
    s_currentTarget = &target;
    const sf::Vector2u targetSize = target.getSize();
    Update(sf::Mouse::getPosition(window),
           sf::Vector2f(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y)), dt);
}

void Update(const sf::Vector2i& mousePos, const sf::Vector2f& displaySize, sf::Time dt) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(displaySize.x, displaySize.y);
    io.DeltaTime = dt.asSeconds() > 0.0f ? dt.asSeconds() : 0.00001f;

    if (s_currentWindow == nullptr) {
        io.AddMousePosEvent(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    }

    if (io.WantSetMousePos && s_currentWindow != nullptr) {
        sf::Mouse::setPosition(sf::Vector2i(static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y)),
                               *s_currentWindow);
    }

    if (s_currentWindow != nullptr) {
        UpdateCursor(*s_currentWindow);
    }

    ImGui::NewFrame();
}

void UpdateTexture(ImTextureData* textureData) {
    switch (textureData->Status) {
        case ImTextureStatus_WantCreate: {
            IM_ASSERT(textureData->Format == ImTextureFormat_RGBA32);
            auto texture = std::make_unique<sf::Texture>();
            if (!texture->resize(sf::Vector2u(static_cast<unsigned int>(textureData->Width),
                                              static_cast<unsigned int>(textureData->Height)))) {
                IM_ASSERT(false && "Failed to allocate an sf::Texture for Dear ImGui");
                return;
            }
            texture->setSmooth(true);
            texture->update(static_cast<const std::uint8_t*>(textureData->GetPixels()));
            sf::Texture* raw = texture.release();
            textureData->BackendUserData = raw;
            textureData->SetTexID(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(raw)));
            textureData->SetStatus(ImTextureStatus_OK);
            break;
        }
        case ImTextureStatus_WantUpdates: {
            auto* texture = static_cast<sf::Texture*>(textureData->BackendUserData);
            if (texture == nullptr) return;
            const int bpp = textureData->BytesPerPixel;

            for (const ImTextureRect& rect : textureData->Updates) {
                if (rect.x == 0 && rect.w == textureData->Width) {
                    texture->update(
                        static_cast<const std::uint8_t*>(textureData->GetPixelsAt(0, rect.y)),
                        sf::Vector2u(static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h)),
                        sf::Vector2u(0, static_cast<unsigned int>(rect.y))
                    );
                } else {
                    s_pixelStaging.resize(static_cast<std::size_t>(rect.w) * rect.h * bpp);
                    for (int y = 0; y < rect.h; ++y) {
                        std::memcpy(s_pixelStaging.data() + static_cast<std::size_t>(y) * rect.w * bpp,
                                    textureData->GetPixelsAt(rect.x, rect.y + y),
                                    static_cast<std::size_t>(rect.w) * bpp);
                    }
                    texture->update(s_pixelStaging.data(),
                                    sf::Vector2u(static_cast<unsigned int>(rect.w), static_cast<unsigned int>(rect.h)),
                                    sf::Vector2u(static_cast<unsigned int>(rect.x), static_cast<unsigned int>(rect.y)));
                }
            }
            textureData->SetStatus(ImTextureStatus_OK);
            break;
        }
        case ImTextureStatus_WantDestroy: {
            delete static_cast<sf::Texture*>(textureData->BackendUserData);
            textureData->BackendUserData = nullptr;
            textureData->SetTexID(ImTextureID_Invalid);
            textureData->SetStatus(ImTextureStatus_Destroyed);
            break;
        }
        default:
            break;
    }
}

void UpdateCursor(sf::Window& window) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) return;

    const ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
    const bool visible = (cursor != ImGuiMouseCursor_None) && !io.MouseDrawCursor;
    if (visible != s_lastCursorVisible) {
        window.setMouseCursorVisible(visible);
        s_lastCursorVisible = visible;
    }

    if (!visible || cursor == s_lastCursor) return;
    s_lastCursor = cursor;

    sf::Cursor::Type sfmlCursorType = sf::Cursor::Type::Arrow;
    switch (cursor) {
        case ImGuiMouseCursor_TextInput:  sfmlCursorType = sf::Cursor::Type::Text; break;
        case ImGuiMouseCursor_ResizeAll:  sfmlCursorType = sf::Cursor::Type::SizeAll; break;
        case ImGuiMouseCursor_ResizeNS:   sfmlCursorType = sf::Cursor::Type::SizeVertical; break;
        case ImGuiMouseCursor_ResizeEW:   sfmlCursorType = sf::Cursor::Type::SizeHorizontal; break;
        case ImGuiMouseCursor_ResizeNESW: sfmlCursorType = sf::Cursor::Type::SizeBottomLeftTopRight; break;
        case ImGuiMouseCursor_ResizeNWSE: sfmlCursorType = sf::Cursor::Type::SizeTopLeftBottomRight; break;
        case ImGuiMouseCursor_Hand:       sfmlCursorType = sf::Cursor::Type::Hand; break;
        case ImGuiMouseCursor_NotAllowed: sfmlCursorType = sf::Cursor::Type::NotAllowed; break;
        default:                          sfmlCursorType = sf::Cursor::Type::Arrow; break;
    }

    auto& slot = s_cursors[static_cast<std::size_t>(cursor)];
    if (!slot.has_value()) {
        slot = sf::Cursor::createFromSystem(sfmlCursorType);
    }
    if (slot.has_value()) {
        window.setMouseCursor(*slot);
    }
}

void SetCurrentWindow(sf::Window& window) {
    s_currentWindow = &window;
}

void Render(sf::RenderTarget& target) {
    s_currentTarget = &target;
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData == nullptr) return;

    // Dear ImGui 1.92+ dynamic texture management
    if (drawData->Textures != nullptr) {
        for (ImTextureData* textureData : *drawData->Textures) {
            if (textureData->Status != ImTextureStatus_OK) {
                UpdateTexture(textureData);
            }
        }
    }

    if (drawData->CmdLists.Size == 0) return;

    const sf::Vector2f targetSize(
        static_cast<float>(target.getSize().x),
        static_cast<float>(target.getSize().y)
    );

    // Save previous user view to prevent corruption of next-frame scene rendering
    const sf::View previousView = target.getView();

    // Construct a pixel-perfect 1:1 view matching the current target dimensions
    sf::View imguiView(sf::FloatRect({ 0.0f, 0.0f }, targetSize));
    target.resetGLStates();
    target.setView(imguiView);

    // Hardware OpenGL scissor test avoids continuous sf::View recomputation overhead
    glEnable(GL_SCISSOR_TEST);

    sf::RenderStates states;
    const ImVec2 clipOffset = drawData->DisplayPos;
    const ImVec2 clipScale = drawData->FramebufferScale;

    for (int n = 0; n < drawData->CmdLists.Size; ++n) {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        const ImDrawVert* vtxBuffer = cmdList->VtxBuffer.Data;
        const ImDrawIdx* idxBuffer = cmdList->IdxBuffer.Data;

        for (int cmdi = 0; cmdi < cmdList->CmdBuffer.Size; ++cmdi) {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmdi];
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmdList, pcmd);
                continue;
            }
            if (pcmd->ElemCount == 0) {
                continue;
            }

            const float clipMinX = (pcmd->ClipRect.x - clipOffset.x) * clipScale.x;
            const float clipMinY = (pcmd->ClipRect.y - clipOffset.y) * clipScale.y;
            const float clipMaxX = (pcmd->ClipRect.z - clipOffset.x) * clipScale.x;
            const float clipMaxY = (pcmd->ClipRect.w - clipOffset.y) * clipScale.y;

            if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) {
                continue;
            }

            // OpenGL coordinates start from the bottom-left corner
            glScissor(
                static_cast<GLint>(clipMinX),
                static_cast<GLint>(targetSize.y - clipMaxY),
                static_cast<GLsizei>(clipMaxX - clipMinX),
                static_cast<GLsizei>(clipMaxY - clipMinY)
            );

            const auto* texture = textureFromID(pcmd->GetTexID());
            sf::Vector2f texSize{ 1.0f, 1.0f };
            if (texture != nullptr) {
                const sf::Vector2u size = texture->getSize();
                texSize = sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y));
            }

            // Pre-allocated contiguous block assignment without vector reallocations
            s_vertexBuffer.resize(pcmd->ElemCount);
            sf::Vertex* dst = s_vertexBuffer.data();
            const ImDrawIdx* cmdIdxBuffer = idxBuffer + pcmd->IdxOffset;
            const ImDrawVert* vtxBase = vtxBuffer + pcmd->VtxOffset;

            for (unsigned int i = 0; i < pcmd->ElemCount; ++i) {
                const ImDrawVert& src = vtxBase[cmdIdxBuffer[i]];
                sf::Color col;
                std::memcpy(&col, &src.col, sizeof(col));
                dst[i] = sf::Vertex{
                    sf::Vector2f{src.pos.x, src.pos.y},
                    col,
                    sf::Vector2f{src.uv.x * texSize.x, src.uv.y * texSize.y}
                };
            }

            states.texture = texture;
            target.draw(s_vertexBuffer.data(), s_vertexBuffer.size(), sf::PrimitiveType::Triangles, states);
        }
    }

    glDisable(GL_SCISSOR_TEST);

    // Restore the user's active scene view
    target.setView(previousView);
}

void Render() {
    if (s_currentTarget != nullptr) {
        Render(*s_currentTarget);
    } else {
        ImGui::EndFrame();
    }
}

void Shutdown(const sf::Window&) {
    Shutdown();
}

void Shutdown() {
    for (ImTextureData* textureData : ImGui::GetPlatformIO().Textures) {
        if (textureData->RefCount == 1) {
            textureData->SetStatus(ImTextureStatus_WantDestroy);
            UpdateTexture(textureData);
        }
    }

    for (auto& cursor : s_cursors) {
        cursor.reset();
    }

    s_lastCursor = ImGuiMouseCursor_COUNT;
    s_lastCursorVisible = true;
    s_vertexBuffer.clear();
    s_vertexBuffer.shrink_to_fit();
    s_pixelStaging.clear();
    s_pixelStaging.shrink_to_fit();
    s_currentWindow = nullptr;
    s_currentTarget = nullptr;

    ImGui::DestroyContext();
}

} // namespace ImGui::SFML
