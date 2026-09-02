#ifndef IMGUI_SFML_EXPORT_H
#define IMGUI_SFML_EXPORT_H

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(IMGUI_SFML_EXPORTS)
        #define IMGUI_SFML_API __declspec(dllexport)
    #elif defined(IMGUI_SFML_SHARED)
        #define IMGUI_SFML_API __declspec(dllimport)
    #else
        #define IMGUI_SFML_API
    #endif
#else
    #if __GNUC__ >= 4
        #define IMGUI_SFML_API __attribute__((visibility("default")))
    #else
        #define IMGUI_SFML_API
    #endif
#endif

#endif // IMGUI_SFML_EXPORT_H
