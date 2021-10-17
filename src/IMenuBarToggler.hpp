#pragma once

struct IMenuBarToggler
{
    virtual ~IMenuBarToggler() { }
    virtual void MenuBarToggle() = 0;
    virtual void MenuBarHide() = 0;
};
