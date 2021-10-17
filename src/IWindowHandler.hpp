#pragma once

struct IWindowHandler
{
    virtual ~IWindowHandler() { }
    virtual void MenuBarToggle() = 0;
    virtual void MenuBarHide() = 0;
    virtual void CheckSizeAsync() = 0;
};
