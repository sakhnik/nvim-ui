#include "Input.hpp"
#include "MsgPackRpc.hpp"
#include "Renderer.hpp"
#include <iostream>
#include <SDL2/SDL.h>


Input::Input(uv_loop_t *loop, MsgPackRpc *rpc, Renderer *renderer)
    : _rpc{rpc}
    , _renderer{renderer}
{
    ::uv_timer_init(loop, &_timer);
    _timer.data = this;
}

Input::~Input()
{
    ::uv_timer_stop(&_timer);
}

void Input::Start()
{
    SDL_StartTextInput();

    auto on_timeout = [](uv_timer_t *handle) {
        Input *self = reinterpret_cast<Input*>(handle->data);
        self->_PollEvents();
    };

    ::uv_timer_start(&_timer, on_timeout, 10, 10);
}

void Input::_OnInput(std::string_view input)
{
    size_t input_size = input.size();
    _rpc->Request(
        [&](MsgPackRpc::PackerT &pk) {
            pk.pack("nvim_input");
            pk.pack_array(1);
            pk.pack(input);
        },
        [=](const msgpack::object &err, const msgpack::object &resp) {
            if (!err.is_nil())
            {
                std::ostringstream oss;
                oss << "Input error: " << err;
                throw std::runtime_error(oss.str());
            }
            size_t consumed = resp.as<size_t>();
            if (consumed < input_size)
            {
                std::cerr << "[input] Consumed " << consumed << "/" << input_size << " bytes" << std::endl;
            }
        }
    );
}

void Input::_RawInput(const char *key)
{
    std::string input("<");
    if (_shift)
        input += "s-";
    if (_control)
        input += "c-";
    input += key;
    input += ">";
    _OnInput(input);
}

void Input::_PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_WINDOWEVENT:
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                _renderer->OnResized(/*event.window.data1, event.window.data2*/);
                break;
            }
            break;
        case SDL_TEXTINPUT:
            if (!_control)
            {
                std::string_view lt("<");
                if (lt == event.text.text)
                    _OnInput("<lt>");
                else
                    _OnInput(event.text.text);
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                _shift = false;
                break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                _control = false;
                break;
            }
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                _shift = true;
                break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                _control = true;
                break;
            case SDLK_ESCAPE:
                _RawInput("esc");
                break;
            case SDLK_RETURN:
                _RawInput("cr");
                break;
            case SDLK_TAB:
                _RawInput("tab");
                break;
            case SDLK_BACKSPACE:
                _RawInput("bs");
                break;
            case SDLK_F1:
                _RawInput("f1");
                break;
            case SDLK_F2:
                _RawInput("f2");
                break;
            case SDLK_F3:
                _RawInput("f3");
                break;
            case SDLK_F4:
                _RawInput("f4");
                break;
            case SDLK_F5:
                _RawInput("f5");
                break;
            case SDLK_F6:
                _RawInput("f6");
                break;
            case SDLK_F7:
                _RawInput("f7");
                break;
            case SDLK_F8:
                _RawInput("f8");
                break;
            case SDLK_F9:
                _RawInput("f9");
                break;
            case SDLK_F10:
                _RawInput("f10");
                break;
            case SDLK_F11:
                _RawInput("f11");
                break;
            case SDLK_F12:
                _RawInput("f12");
                break;
            default:
                if (_control)
                {
                    const char *keyname = SDL_GetKeyName(event.key.keysym.sym);
                    if (1 == strlen(keyname))
                        _RawInput(keyname);
                    else
                        std::cerr << "Key " << keyname << " not handled" << std::endl;
                }
                break;
            }
            break;
        }
    }
}