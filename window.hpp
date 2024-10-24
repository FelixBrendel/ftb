#pragma once
#include "core.hpp"
#include "math.hpp"
#include "ringbuffer.hpp"

#ifdef FTB_WINDOWS
struct Window_Type {
    HWND window;
};
#else
# include <X11/Xlib.h>
struct Window_Type {
    u64      window;
    Display* display;
};
#endif

struct Window_State;
Window_Type   create_window(IV2 size, const char* title);
Window_State* update_window(Window_Type);
void          destroy_window(Window_Type);

enum struct Mouse_Buttons {
    Left, Right, Middle,
    _4, _5,

    ENUM_SIZE
};

enum struct Keyboard_Keys {
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    Space, Escape, Enter, Tab, Backspace, Delete,
    Arrow_Left, Arrow_Right, Arrow_Up, Arrow_Down,

    Left_Shift, Right_Shift, Left_Control, Right_Control, Left_Alt, Right_Alt,

    _0='0', _1, _2, _3, _4, _5, _6, _7, _8, _9,
    A='a', B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    Comma, Semicolon, Period, Colon, Minus, Underscore, Plus, Asterisk, Hash, Apostrophe,

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

    Ringbuffer<char> input_buffer;

    void add_key_event(Keyboard_Keys key, bool down) {
        keyboard.ended_down[(s32)key] = down;
        ++keyboard.transition_count[(s32)key];
    }

    void add_mouse_event(Mouse_Buttons button, bool down) {
        mouse.ended_down[(s32)button] = down;
        ++mouse.transition_count[(s32)button];
    }

    void set_key_state(Keyboard_Keys key, bool down) {
        if (key_is_down(key) != down)
            add_key_event(key, down);
    }

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
    u32 keyboard_codepage;
};

#ifdef FTB_WINDOW_IMPL

void clear_state_for_update(Window_State* state) {
    memset(&state->input.mouse.transition_count, 0, sizeof(state->input.mouse.transition_count));
    memset(&state->input.keyboard.transition_count, 0, sizeof(state->input.keyboard.transition_count));

    memset(&state->events, 0, sizeof(state->events));
    state->input.mouse.scroll_delta = 0;
    state->input.input_buffer.reset();
}

#  ifdef FTB_WINDOWS
#include "hashmap.hpp"
#include <windowsx.h>

Hash_Map<void*, Window_State> hwnd_to_state;
static const char* class_name = "FTB_WINDOW_CLASS";
static bool initialized = false;
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
        case VK_BACK: return Keyboard_Keys::Backspace;
        case VK_SPACE: return Keyboard_Keys::Space;
        case VK_RETURN: return Keyboard_Keys::Enter;
        case VK_ESCAPE: return Keyboard_Keys::Escape;

        case VK_OEM_7: return Keyboard_Keys::Apostrophe;
        case VK_OEM_COMMA: return Keyboard_Keys::Comma;
        case VK_OEM_MINUS: return Keyboard_Keys::Minus;
        case VK_OEM_PERIOD: return Keyboard_Keys::Period;
        case VK_OEM_1: return Keyboard_Keys::Semicolon;

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

// code taken from dear imgui
void update_codepage(Window_State* window) {
    // Retrieve keyboard code page, required for handling of non-Unicode Windows.
    HKL keyboard_layout = ::GetKeyboardLayout(0);
    LCID keyboard_lcid = MAKELCID(HIWORD(keyboard_layout), SORT_DEFAULT);
    if (GetLocaleInfoA(keyboard_lcid, (LOCALE_RETURN_NUMBER | LOCALE_IDEFAULTANSICODEPAGE), (LPSTR)&window->keyboard_codepage, sizeof(window->keyboard_codepage)) == 0)
        window->keyboard_codepage = CP_ACP; // Fallback to default ANSI code page when fails.
}

static bool is_vk_down(int vk) {
    return (::GetKeyState(vk) & 0x8000) != 0;
}

static void process_key_events_workarounds(Window_State* state) {
    // Left & right Shift keys: when both are pressed together, Windows tend to not generate the WM_KEYUP event for the first released one.
    if (state->input.key_is_down(Keyboard_Keys::Left_Shift) && !is_vk_down(VK_LSHIFT))
        state->input.add_key_event(Keyboard_Keys::Left_Shift, false);

    if (state->input.key_is_down(Keyboard_Keys::Right_Shift) && !is_vk_down(VK_RSHIFT))
        state->input.add_key_event(Keyboard_Keys::Right_Shift, false);
}

static void update_key_modifiers(Window_State* state) {
    state->input.set_key_state(Keyboard_Keys::Left_Shift,    is_vk_down(VK_LSHIFT));
    state->input.set_key_state(Keyboard_Keys::Left_Shift,    is_vk_down(VK_LSHIFT));
    state->input.set_key_state(Keyboard_Keys::Right_Shift,   is_vk_down(VK_RSHIFT));
    state->input.set_key_state(Keyboard_Keys::Left_Control,  is_vk_down(VK_LCONTROL));
    state->input.set_key_state(Keyboard_Keys::Right_Control, is_vk_down(VK_RCONTROL));
    state->input.set_key_state(Keyboard_Keys::Left_Alt, is_vk_down(VK_LMENU));
    state->input.set_key_state(Keyboard_Keys::Right_Alt, is_vk_down(VK_RMENU));
}

Window_State* update_window(Window_Type window) {
    Window_State* state = hwnd_to_state.get_object_ptr(window.window);

    clear_state_for_update(state);

    while (true) {
        MSG  message;
        BOOL result = PeekMessage(&message, NULL, 0, 0, PM_REMOVE);
        if (result == FALSE)
            break;

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    process_key_events_workarounds(state);

    return state;
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
                Mouse_Buttons button = Mouse_Buttons::Left;
                if (msg == WM_LBUTTONUP) { button = Mouse_Buttons::Left; }
                if (msg == WM_RBUTTONUP) { button = Mouse_Buttons::Right; }
                if (msg == WM_MBUTTONUP) { button = Mouse_Buttons::Middle; }
                if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? Mouse_Buttons::_4 : Mouse_Buttons::_5; }
                state->input.add_mouse_event(button, true);
                return 0;
            }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
            {
                Mouse_Buttons button = Mouse_Buttons::Left;
                if (msg == WM_LBUTTONUP) { button = Mouse_Buttons::Left; }
                if (msg == WM_RBUTTONUP) { button = Mouse_Buttons::Right; }
                if (msg == WM_MBUTTONUP) { button = Mouse_Buttons::Middle; }
                if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? Mouse_Buttons::_4 : Mouse_Buttons::_5; }
                state->input.add_mouse_event(button, false);
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
                update_key_modifiers(state);

                // Obtain virtual key code
                // (keypad enter doesn't have its own... VK_RETURN with KF_EXTENDED flag means keypad enter, see IM_VK_KEYPAD_ENTER definition for details, it is mapped to ImGuiKey_KeyPadEnter.)
                int vk = (int)wParam;

                Keyboard_Keys key = key_from_vk(vk);
                if (key != Keyboard_Keys::ENUM_SIZE) {
                    state->input.add_key_event(key, is_key_down);
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
        case WM_CHAR: {
            wchar_t wch = 0;
            MultiByteToWideChar(state->keyboard_codepage, MB_PRECOMPOSED, (char*)&wParam, 1, &wch, 1);
            char utf8_buffer[5];
            WideCharToMultiByte(CP_UTF8, 0 /* dwFlags */, &wch, 1, /*length = 1*/
                                (LPSTR)utf8_buffer, sizeof(utf8_buffer), 0, 0);

            if (utf8_buffer[0] < 128) {
                state->input.input_buffer.push(utf8_buffer[0]);
            }
            return 0;
        }
        case WM_INPUTLANGCHANGE: {
            update_codepage(state);
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

    Window_State win_state = {};
    update_codepage(&win_state);

    win_state.input.input_buffer.init(16);
    hwnd_to_state.set_object(result.window, win_state);

    return result;
}

void destroy_window(Window_Type window) {
    DestroyWindow(window.window);
}

#  else

Hash_Map<void*, Window_State> window_to_state;

static Atom wm_delete_window;
Window_Type create_window(IV2 size, const char* title) {
    Window_Type result;

    result.display = XOpenDisplay(NULL);
    if (!result.display) {
        return {};
    }

    if (window_to_state.data == nullptr) {
        window_to_state.init();
    }

    Screen* screen   = DefaultScreenOfDisplay(result.display);
    s32     screenId = DefaultScreen(result.display);


    XEvent ev;

    result.window = XCreateSimpleWindow(
        result.display, RootWindowOfScreen(screen), 0, 0,
        size.x, size.y, 1,
        BlackPixel(result.display, screenId),
        WhitePixel(result.display, screenId));

    XStoreName(result.display, result.window, title);

    XSelectInput(result.display, result.window,
                 KeyPressMask    | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask);

    XClearWindow(result.display, result.window);
    XMapRaised(result.display, result.window);

    wm_delete_window = XInternAtom(result.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(result.display, result.window, &wm_delete_window, 1);

    window_to_state.set_object((void*)result.window, Window_State{});

    return result;
}
#include <X11/keysymdef.h>
#include <X11/Xutil.h>
Window_State* update_window(Window_Type window) {
    Window_State* state = window_to_state.get_object_ptr((void*)window.window);
    clear_state_for_update(state);

    // Get Mouse pos
    {
        Window root_return, child_return;
        int root_x_return, root_y_return;
        int win_x_return, win_y_return;
        unsigned int mask_return;

        Bool success =
            XQueryPointer(window.display, window.window, &root_return, &child_return,
                          &root_x_return, &root_y_return, &win_x_return, &win_y_return,
                          &mask_return);

        if (success == True) {
            state->input.mouse.position = {
                (f32)win_x_return,
                (f32)win_y_return
            };
        }
    }

    while (XPending(window.display)) {
        XEvent ev;
        XNextEvent(window.display, &ev);

        if (ev.type == ClientMessage &&
            ev.xclient.data.l[0] == wm_delete_window)
        {
            state->events[(u32)Window_Events::Window_Wants_To_Close] = true;
            continue;
        }

        switch (ev.type) {
            case ButtonPress:
            case ButtonRelease: {
                bool is_down = ev.type == ButtonPress;

                if (ev.xbutton.button == 4) {
                    state->input.mouse.scroll_delta += 1;
                } else if (ev.xbutton.button == 5) {
                    state->input.mouse.scroll_delta -= 1;
                } else {
                    Mouse_Buttons button = Mouse_Buttons::ENUM_SIZE;

                    switch (ev.xbutton.button) {
                        case 1: button = Mouse_Buttons::Left;   break;
                        case 3: button = Mouse_Buttons::Right;  break;
                        case 2: button = Mouse_Buttons::Middle; break;
                    }

                    if (button != Mouse_Buttons::ENUM_SIZE) {
                        u32 button_as_int = (u32)button;
                        ++state->input.mouse.transition_count[button_as_int];
                        state->input.mouse.ended_down[button_as_int] = is_down;
                    }
                }

            } break;
            case KeyPress:
            case KeyRelease: {
                bool is_down = ev.type == KeyPress;
                int len = 0;
                KeySym keysym = 0;
                char str[25] = {0};
                len = XLookupString(&ev.xkey, str, 25, &keysym, NULL);

                Keyboard_Keys key = Keyboard_Keys::ENUM_SIZE;

                if (keysym >= XK_0 && keysym <= XK_9) {
                    key = (Keyboard_Keys)((u32)Keyboard_Keys::_0 + (keysym - XK_0));
                } else if (keysym >= XK_F1 && keysym <= XK_F11) {
                    // NOTE(Felix): XK_F12 is not the next number XK_F11, so
                    //   only handle until F11 and manually handle F12
                    //   separately
                    key = (Keyboard_Keys)((u32)Keyboard_Keys::F1 + (keysym - XK_F1));
                } else if (keysym >= XK_a && keysym <= XK_z) {
                    key = (Keyboard_Keys)((u32)Keyboard_Keys::A + (keysym - XK_a));
                } else {

                    switch (keysym) {
#define handle(x, my) case (x): key = Keyboard_Keys::my; break
                        handle(XK_space,     Space);
                        handle(XK_Escape,    Escape);
                        handle(XK_Return,    Enter);
                        handle(XK_Tab,       Tab);
                        handle(XK_BackSpace, Backspace);
                        handle(XK_Delete,    Delete);
                        handle(XK_Left,      Arrow_Left);
                        handle(XK_Right,     Arrow_Right);
                        handle(XK_Up,        Arrow_Up);
                        handle(XK_Down,      Arrow_Down);
                        handle(XK_F12,       F12);
                        handle(XK_Shift_L,   Left_Shift);
                        handle(XK_Shift_R,   Right_Shift);
                        handle(XK_Control_L, Left_Control);
                        handle(XK_Control_R, Right_Control);
                        handle(XK_Alt_L,     Left_Alt);
                        handle(XK_Alt_R,     Right_Alt);
#undef handle
                    }
                }

                if (key != Keyboard_Keys::ENUM_SIZE) {
                    u32 key_as_int = (u32)key;
                    ++state->input.keyboard.transition_count[key_as_int];
                    state->input.keyboard.ended_down[key_as_int] = is_down;
                }

            } break;
        }
    }

    return state;
}

void destroy_window(Window_Type window) {
    XDestroyWindow(window.display, window.window);
    XCloseDisplay(window.display);
}
#  endif
#endif
