# build service
set(SOURCE_FILES
    Simple.cpp
    Pointer.cpp
    Memory.cpp
    AllocUtils.cpp
)

add_library(Allocator ${SOURCE_FILES})
target_link_libraries(Allocator ${CMAKE_THREAD_LIBS_INIT})
