/* date = December 30th 2021 4:41 pm */
#ifndef MEMORY_H

// NOTE(nates): Depends on types.h

struct memory_arena
{
    void *Base;
    u32 Used;
    u32 Size;
};

internal void *
PushSize_(memory_arena *Arena, 
          u32 AllocSize)
{
    void *Ptr = 0;
    if((Arena->Used + AllocSize) < Arena->Size)
    {
        Ptr = (u8 *)(Arena->Base) + Arena->Used;
        Arena->Used += AllocSize;
    }
    
    return(Ptr);
}

#define PushStruct(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type *)PushSize_(arena, (sizeof(type)*count))

#define MEMORY_H
#endif //MEMORY_H
