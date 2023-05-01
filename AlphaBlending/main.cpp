#include <cstdio>

#include "Blender.hpp"

int main(const int argc, const char *argv[])
{
    Blender blender = {};
    GetBlendParams(argc, argv, &blender);
    
    return AlphaBlending(&blender);     
}