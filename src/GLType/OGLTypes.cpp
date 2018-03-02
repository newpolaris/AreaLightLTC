#include <GLType/OGLTypes.h>

namespace OGLTypes
{
    GLbitfield translate(GraphicsUsageFlags usage)
    {
        GLbitfield flags = GL_ZERO;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagReadBit)
            flags |= GL_MAP_READ_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagWriteBit)
            flags |= GL_MAP_WRITE_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagPersistentBit)
            flags |= GL_MAP_PERSISTENT_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagCoherentBit)
            flags |= GL_MAP_COHERENT_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagFlushExplicitBit)
            flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagDynamicStorageBit)
            flags |= GL_DYNAMIC_STORAGE_BIT;
        if (usage & GraphicsUsageFlagBits::GraphicsUsageFlagClientStorageBit)
            flags |= GL_CLIENT_STORAGE_BIT;
        return flags;
    }

    GLenum translate(GraphicsTarget target)
    {
		static GLenum const Table[] =
		{
            GL_TEXTURE_1D,
            GL_TEXTURE_1D_ARRAY,
            GL_TEXTURE_2D,
            GL_TEXTURE_2D_ARRAY,
            GL_TEXTURE_3D,
			GL_TEXTURE_RECTANGLE_EXT,
            GL_INVALID_ENUM, // RECT_ARRAY
			GL_TEXTURE_CUBE_MAP,
			GL_TEXTURE_CUBE_MAP_ARRAY
		};
		static_assert(sizeof(Table) / sizeof(Table[0]) == GraphicsTargetCount, "error: target descriptor list doesn't match number of supported targets");

		return Table[target];
	}
}
