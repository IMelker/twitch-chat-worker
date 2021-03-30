add_executable(buffer_test BufferStaticTest.cpp ../BufferStatic.cpp ../BufferStatic.h)

set(CMAKE_CXX_STANDARD 17)

if(APPLE OR CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_library (GTEST_LIBRARY
            NAMES gtest
            PATHS /usr/lib /usr/local/lib
            )
    if (GTEST_LIBRARY)
        target_link_libraries(buffer_test LINK_PUBLIC ${GTEST_LIBRARY})
    endif ()
endif()
