#pragma once
#include "core.hpp"
#include "math.hpp"

#ifdef FTB_WINDOWS
struct Window_Type {
    HWND window;
};
#else
struct Window_Type {
    u64   window;
    void* display;
};
#endif


struct Window_State;
Window_Type create_window(IV2 size, const char* title);
Window_State* update_window(Window_Type);

enum struct Mouse_Buttons {
    Left, Right, Middle,
    _4, _5,

    ENUM_SIZE
};

enum struct Keyboard_Keys {
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9,

    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    Space, Escape, Enter, Tab, Backspace, Delete,
    Arrow_Left, Arrow_Right, Arrow_Up, Arrow_Down,

    Left_Shift, Right_Shift, Left_Control, Right_Control, Left_Alt, Right_Alt,

    ENUM_SIZE
};

enum struct Window_Events {
    Mouse_Left_Window,
    Window_Resized,
    Window_Got_Focus,
    Window_Lost_Focus,
    Window_Wants_To_Close,

    ENUM_SIZE
};

struct User_Input {
    // NOTE(Felix): SOA design so we can quickly clear the
    //   `transition_count' each frame with a single memset
    struct {
        bool       ended_down[(u32)Keyboard_Keys::ENUM_SIZE];
        u32  transition_count[(u32)Keyboard_Keys::ENUM_SIZE];
    } keyboard;
    struct {
        bool       ended_down[(u32)Mouse_Buttons::ENUM_SIZE];
        u32  transition_count[(u32)Mouse_Buttons::ENUM_SIZE];
        f32  scroll_delta;
        V2   position;
    } mouse;


    bool key_went_down(Keyboard_Keys key) {
        return keyboard.ended_down[(u32)key] &&
            keyboard.transition_count[(u32)key] != 0;
    }

    bool key_went_up(Keyboard_Keys key) {
        return !keyboard.ended_down[(u32)key] &&
            keyboard.transition_count[(u32)key] != 0;
    }

    bool key_is_down(Keyboard_Keys key) {
        return keyboard.ended_down[(u32)key];
    }

    bool mouse_went_down(Mouse_Buttons button) {
        return mouse.ended_down[(u32)button] &&
            mouse.transition_count[(u32)button] != 0;
    }

    bool mouse_went_up(Mouse_Buttons button) {
        return !mouse.ended_down[(u32)button] &&
            mouse.transition_count[(u32)button] != 0;
    }

    bool mouse_is_down(Mouse_Buttons button) {
        return mouse.ended_down[(u32)button];
    }
};

struct Window_State {
    bool events[(u32)Window_Events::ENUM_SIZE];
    IV2  new_size;

    User_Input input;
};

#ifdef FTB_WINDOW_IMPL
#  ifdef FTB_WINDOWS
#include "hashmap.hpp"
#include <windowsx.h>

Hash_Map<void*, Window_State> hwnd_to_state;
static const char* class_name = "FTB_WINDOW_CLASS";
static bool initialized = false;

Window_State* update_window(Window_Type window) {
    Window_State* state = hwnd_to_state.get_object_ptr(window.window);

    memset(&state->input.mouse.transition_count, 0, sizeof(state->input.mouse.transition_count));
    memset(&state->input.keyboard.transition_count, 0, sizeof(state->input.keyboard.transition_count));

    memset(&state->events, 0, sizeof(state->events));
    state->input.mouse.scroll_delta = 0;

    while (true) {
        MSG  message;
        BOOL result = PeekMessage(&message, NULL, 0, 0, PM_REMOVE);
        if (result == FALSE)
            break;

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return state;
}

/*
  NOTE(Felix): Much of this window-proc code is adapted from DearImGUI's win32
  implementation, as it gave a good overview how to handle some edge cases (of
  which most I have removed because I think they are not relevant for a quick
  interactive window prototypey thing this is meant to be, but many thanks to
  DearImGUI!)
*/
Keyboard_Keys key_from_vk(int vk) {
    switch (vk) {
        case VK_TAB: return Keyboard_Keys::Tab;
        case VK_LEFT: return Keyboard_Keys::Arrow_Left;
        case VK_RIGHT: return Keyboard_Keys::Arrow_Right;
        case VK_UP: return Keyboard_Keys::Arrow_Up;
        case VK_DOWN: return Keyboard_Keys::Arrow_Down;
        case VK_DELETE: return Keyboard_Keys::Delete;
        case VK_SPACE: return Keyboard_Keys::Space;
        case VK_RETURN: return Keyboard_Keys::Enter;
        case VK_ESCAPE: return Keyboard_Keys::Escape;
        case VK_NUMPAD0: return Keyboard_Keys::_0;
        case VK_NUMPAD1: return Keyboard_Keys::_1;
        case VK_NUMPAD2: return Keyboard_Keys::_2;
        case VK_NUMPAD3: return Keyboard_Keys::_3;
        case VK_NUMPAD4: return Keyboard_Keys::_4;
        case VK_NUMPAD5: return Keyboard_Keys::_5;
        case VK_NUMPAD6: return Keyboard_Keys::_6;
        case VK_NUMPAD7: return Keyboard_Keys::_7;
        case VK_NUMPAD8: return Keyboard_Keys::_8;
        case VK_NUMPAD9: return Keyboard_Keys::_9;
        case VK_LSHIFT: return Keyboard_Keys::Left_Shift;
        case VK_LCONTROL: return Keyboard_Keys::Left_Control;
        case VK_RSHIFT: return Keyboard_Keys::Right_Shift;
        case VK_RCONTROL: return Keyboard_Keys::Right_Control;
        case '0': return Keyboard_Keys::_0;
        case '1': return Keyboard_Keys::_1;
        case '2': return Keyboard_Keys::_2;
        case '3': return Keyboard_Keys::_3;
        case '4': return Keyboard_Keys::_4;
        case '5': return Keyboard_Keys::_5;
        case '6': return Keyboard_Keys::_6;
        case '7': return Keyboard_Keys::_7;
        case '8': return Keyboard_Keys::_8;
        case '9': return Keyboard_Keys::_9;
        case 'A': return Keyboard_Keys::A;
        case 'B': return Keyboard_Keys::B;
        case 'C': return Keyboard_Keys::C;
        case 'D': return Keyboard_Keys::D;
        case 'E': return Keyboard_Keys::E;
        case 'F': return Keyboard_Keys::F;
        case 'G': return Keyboard_Keys::G;
        case 'H': return Keyboard_Keys::H;
        case 'I': return Keyboard_Keys::I;
        case 'J': return Keyboard_Keys::J;
        case 'K': return Keyboard_Keys::K;
        case 'L': return Keyboard_Keys::L;
        case 'M': return Keyboard_Keys::M;
        case 'N': return Keyboard_Keys::N;
        case 'O': return Keyboard_Keys::O;
        case 'P': return Keyboard_Keys::P;
        case 'Q': return Keyboard_Keys::Q;
        case 'R': return Keyboard_Keys::R;
        case 'S': return Keyboard_Keys::S;
        case 'T': return Keyboard_Keys::T;
        case 'U': return Keyboard_Keys::U;
        case 'V': return Keyboard_Keys::V;
        case 'W': return Keyboard_Keys::W;
        case 'X': return Keyboard_Keys::X;
        case 'Y': return Keyboard_Keys::Y;
        case 'Z': return Keyboard_Keys::Z;
        case VK_F1: return Keyboard_Keys::F1;
        case VK_F2: return Keyboard_Keys::F2;
        case VK_F3: return Keyboard_Keys::F3;
        case VK_F4: return Keyboard_Keys::F4;
        case VK_F5: return Keyboard_Keys::F5;
        case VK_F6: return Keyboard_Keys::F6;
        case VK_F7: return Keyboard_Keys::F7;
        case VK_F8: return Keyboard_Keys::F8;
        case VK_F9: return Keyboard_Keys::F9;
        case VK_F10: return Keyboard_Keys::F10;
        case VK_F11: return Keyboard_Keys::F11;
        case VK_F12: return Keyboard_Keys::F12;
        default: return Keyboard_Keys::ENUM_SIZE;
    }
}

LRESULT ftb_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window_State* state = hwnd_to_state.get_object_ptr(hwnd);
    if (state == nullptr)
        return DefWindowProc(hwnd, msg, wParam, lParam);


    switch (msg) {
        case WM_CLOSE: {
            state->events[(u32)Window_Events::Window_Wants_To_Close] = true;
            return 0;
        }
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE: {
            POINT mouse_pos = { (LONG)GET_X_LPARAM(lParam), (LONG)GET_Y_LPARAM(lParam) };
            if (msg == WM_NCMOUSEMOVE && ::ScreenToClient(hwnd, &mouse_pos) == FALSE) // WM_NCMOUSEMOVE are provided in absolute coordinates.
                return 0;

            state->input.mouse.position = {(f32)mouse_pos.x, (f32)mouse_pos.y};
            return 0;
        }
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
            {
                int button = 0;
                if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
                if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
                if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
                if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
                state->input.mouse.ended_down[button] = true;
                ++state->input.mouse.transition_count[button];
                return 0;
            }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
            {
                int button = 0;
                if (msg == WM_LBUTTONUP) { button = 0; }
                if (msg == WM_RBUTTONUP) { button = 1; }
                if (msg == WM_MBUTTONUP) { button = 2; }
                if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
                state->input.mouse.ended_down[button] = false;
                ++state->input.mouse.transition_count[button];
                return 0;
            }
        case WM_MOUSEWHEEL: {
            state->input.mouse.scroll_delta += (f32)GET_WHEEL_DELTA_WPARAM(wParam) / (f32)WHEEL_DELTA;
            return 0;
        }
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            const bool is_key_down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            if (wParam < 256) {
                // Obtain virtual key code
                // (keypad enter doesn't have its own... VK_RETURN with KF_EXTENDED flag means keypad enter, see IM_VK_KEYPAD_ENTER definition for details, it is mapped to ImGuiKey_KeyPadEnter.)
                int vk = (int)wParam;

                Keyboard_Keys key = key_from_vk(vk);
                if (key != Keyboard_Keys::ENUM_SIZE) {
                    state->input.keyboard.ended_down[(s32)key] = is_key_down;
                    ++state->input.keyboard.transition_count[(s32)key];
                }
            }
            return 0;
        }
        case WM_SETFOCUS: {
            state->events[(s32)Window_Events::Window_Got_Focus]  = true;
            state->events[(s32)Window_Events::Window_Lost_Focus] = false;
            return 0;
        }
        case WM_KILLFOCUS: {
            state->events[(s32)Window_Events::Window_Got_Focus]  = false;
            state->events[(s32)Window_Events::Window_Lost_Focus] = true;
            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void maybe_init_window_stuff() {
    if (initialized)
        return;

    hwnd_to_state.init();

    WNDCLASSEX window_class = {
        .cbSize               = sizeof(WNDCLASSEX),
        .style                = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc          = ftb_window_proc,
        .hInstance            = GetModuleHandle(nullptr),
        .hCursor              = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName        = class_name,
    };

    if (RegisterClassEx(&window_class) == 0) {
        panic("RegisterClassExW failed");
        return;
    }

    initialized = true;
}

Window_Type create_window(IV2 size, const char* title) {
    maybe_init_window_stuff();

    u32 style = WS_OVERLAPPEDWINDOW;

    RECT work_area;
    s32 window_left = 0;
    s32 window_top  = 0;
    bool success = SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
    if (success) {
        window_left = work_area.left;
        window_top = work_area.top;
    }

    RECT rect {
        .right  = size.x,
        .bottom = size.y,
    };

    AdjustWindowRect(&rect, style, FALSE);

    s32 window_width  = rect.right  - rect.left;
    s32 window_height = rect.bottom - rect.top;

    Window_Type result {};
    result.window = CreateWindowEx(0, class_name, title,
                                    style,
                                    window_left,  window_top,
                                    window_width, window_height,
                                    NULL, NULL, 0, NULL);

    if (result.window == NULL) {
        panic("CreateWindowEx failed");
        return {};
    }

    UpdateWindow(result.window);
    ShowWindow(result.window, SW_SHOW);

    hwnd_to_state.set_object(result.window, Window_State{});

    return result;
}
#  else
#    error "Not yet implemented"
#  endif
#endif
