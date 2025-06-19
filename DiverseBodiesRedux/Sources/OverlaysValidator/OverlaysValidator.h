#pragma once
#include <set>
#include <string>

class OverlaysValidator
{
public:
    std::set<std::string> operator()() const;
};



