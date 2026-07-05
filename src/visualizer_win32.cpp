#include "elastic_wave/simulation.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace {

constexpr wchar_t window_class[] = L"ElasticWaveVisualizer";
bool running = true;

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_CLOSE) {
        running = false;
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_DESTROY) {
        running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

void process_messages() {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

std::uint32_t heat_colour(double value) {
    value = std::clamp(value, 0.0, 1.0);
    const double scaled = value * 4.0;
    const int segment = std::min(3, static_cast<int>(scaled));
    const double fraction = scaled - segment;

    int red = 0;
    int green = 0;
    int blue = 0;
    switch (segment) {
    case 0:
        blue = static_cast<int>(255.0 * fraction);
        break;
    case 1:
        green = static_cast<int>(255.0 * fraction);
        blue = 255;
        break;
    case 2:
        red = static_cast<int>(255.0 * fraction);
        green = 255;
        blue = static_cast<int>(255.0 * (1.0 - fraction));
        break;
    default:
        red = 255;
        green = static_cast<int>(255.0 * (1.0 - fraction));
        break;
    }
    return static_cast<std::uint32_t>(blue | (green << 8) | (red << 16));
}

void draw(HWND window, const elastic_wave::StateView& state, double scale) {
    const int width = static_cast<int>(state.horizontal.width());
    const int height = static_cast<int>(state.horizontal.height());
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const double u = state.horizontal(static_cast<std::size_t>(x),
                                              static_cast<std::size_t>(y));
            const double v = state.vertical(static_cast<std::size_t>(x),
                                            static_cast<std::size_t>(y));
            pixels[static_cast<std::size_t>((height - y - 1) * width + x)] =
                heat_colour(std::hypot(u, v) / scale);
        }
    }

    BITMAPINFO bitmap{};
    bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap.bmiHeader.biWidth = width;
    bitmap.bmiHeader.biHeight = -height;
    bitmap.bmiHeader.biPlanes = 1;
    bitmap.bmiHeader.biBitCount = 32;
    bitmap.bmiHeader.biCompression = BI_RGB;

    RECT client{};
    GetClientRect(window, &client);
    HDC dc = GetDC(window);
    StretchDIBits(dc, 0, 0, client.right, client.bottom,
                  0, 0, width, height, pixels.data(), &bitmap,
                  DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(window, dc);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show_command) {
    try {
        WNDCLASSW window_definition{};
        window_definition.lpfnWndProc = window_proc;
        window_definition.hInstance = instance;
        window_definition.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        window_definition.lpszClassName = window_class;
        if (!RegisterClassW(&window_definition)) return 1;

        HWND window = CreateWindowExW(
            0, window_class, L"Elastic wave FDM 2D", WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 850, 850,
            nullptr, nullptr, instance, nullptr);
        if (!window) return 1;
        ShowWindow(window, show_command);

        elastic_wave::Simulation simulation({});
        const double scale = simulation.configuration().initial_displacement;
        simulation.run([&](const elastic_wave::StateView& state) {
            process_messages();
            if (!running) return;
            if (state.step % 10 != 0 && state.step != simulation.steps()) return;

            draw(window, state, scale);
            const std::wstring title = L"Elastic wave FDM 2D — step " +
                std::to_wstring(state.step) + L" / " +
                std::to_wstring(simulation.steps()) + L", t = " +
                std::to_wstring(state.time) + L" s";
            SetWindowTextW(window, title.c_str());
            Sleep(10);
        });

        if (running) {
            SetWindowTextW(window, L"Elastic wave FDM 2D — finished (close the window to exit)");
            while (running) {
                process_messages();
                Sleep(10);
            }
        }
        return 0;
    } catch (const std::exception& error) {
        MessageBoxA(nullptr, error.what(), "Elastic wave error", MB_OK | MB_ICONERROR);
        return 1;
    }
}
