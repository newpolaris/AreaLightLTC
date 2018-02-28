#include <GLType/OGLUtil.h>

GLbitfield GetOGLUsageFlag(GraphicsUsageFlags usage)
{
    GLbitfield flags = GL_ZERO;
    if(usage & GraphicsUsageFlagBits::GraphicsUsageFlagReadBit)
        flags |= GL_MAP_READ_BIT;
    if(usage & GraphicsUsageFlagBits::GraphicsUsageFlagWriteBit)
        flags |= GL_MAP_WRITE_BIT;
    if(usage & GraphicsUsageFlagBits::GraphicsUsageFlagPersistentBit)
        flags |= GL_MAP_PERSISTENT_BIT;
    if(usage & GraphicsUsageFlagBits::GraphicsUsageFlagCoherentBit)
        flags |= GL_MAP_COHERENT_BIT;
    if(usage & GraphicsUsageFlagBits::GraphicsUsageFlagFlushExplicitBit)
        flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
    if(usage & GraphicsUsageFlagBits::GraphicsUsageFlagDynamicStorageBit)
        flags |= GL_DYNAMIC_STORAGE_BIT;
    if(usage & GraphicsUsageFlagBits::GraphicsUsageFlagClientStorageBit)
        flags |= GL_CLIENT_STORAGE_BIT;
    return flags;
}
