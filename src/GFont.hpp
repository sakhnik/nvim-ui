#pragma once

#include <string>

class GFont
{
public:
    const std::string& GetFace() const { return _face; }
    double GetSizePt() const { return _size_pt; }
    void SetSizePt(double size_pt) { _size_pt = size_pt; }

private:
    std::string _face = "Fira Code";
    double _size_pt = 14;
};
