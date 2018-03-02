#pragma once

#include <tools/Rtti.h>

class GraphicsTexture : public rtti::Interface
{
    __DeclareSubInterface(GraphicsTexture, rtti::Interface)
public:

    GraphicsTexture() noexcept;
    virtual ~GraphicsTexture() noexcept;
};
