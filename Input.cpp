#include "Input.hpp"
#include "MsgPackRpc.hpp"
#include <iostream>
#include <SDL2/SDL.h>


Input::Input(uv_loop_t *loop, MsgPackRpc *rpc)
    : _rpc{rpc}
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

void Input::_OnInput(const std::string &input)
{
    std::cerr << "Input " << input << std::endl;

    _rpc->Request(
        [&](MsgPackRpc::PackerT &pk) {
            pk.pack(std::string{"nvim_input"});
            pk.pack_array(1);
            pk.pack(input);
        },
        [](const msgpack::object &err, const auto &resp) {
            if (!err.is_nil())
            {
                std::ostringstream oss;
                oss << "Input error: " << err;
                throw std::runtime_error(oss.str());
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
        case SDL_TEXTINPUT:
            if (!_control)
                _OnInput(event.text.text);
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
