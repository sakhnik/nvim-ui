#include "SdlLoop.hpp"
#include "Renderer.hpp"
#include "Logger.hpp"
#include <string_view>
#include <SDL2/SDL.h>


SdlLoop::SdlLoop(uv_loop_t *loop, Renderer *renderer)
    : _renderer{renderer}
    , _timer{loop}
{
}

SdlLoop::~SdlLoop()
{
    _timer.Stop();
}

void SdlLoop::Start()
{
    SDL_StartTextInput();
    _StartTimer();
}

void SdlLoop::_StartTimer()
{
    _timer.Start(10, 10, [this] {
        _PollEvents();
    });
}

void SdlLoop::_RawInput(const char *key)
{
    std::string input("<");
    if (_shift)
        input += "s-";
    if (_control)
        input += "c-";
    input += key;
    input += ">";
    _renderer->Input(input);
}

void SdlLoop::_PollEvents()
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
                    _renderer->Input("<lt>");
                else
                    _renderer->Input(event.text.text);
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
            case SDLK_LEFT:
                _RawInput("left");
                break;
            case SDLK_RIGHT:
                _RawInput("right");
                break;
            case SDLK_UP:
                _RawInput("up");
                break;
            case SDLK_DOWN:
                _RawInput("down");
                break;
            default:
                if (_control)
                {
                    const char *keyname = SDL_GetKeyName(event.key.keysym.sym);
                    if (1 == strlen(keyname))
                        _RawInput(keyname);
                    else
                        Logger().warn("Key {} not handled", keyname);
                }
                break;
            }
            break;
        }
    }
}
