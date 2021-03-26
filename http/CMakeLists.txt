message("- [http]")
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost REQUIRED COMPONENTS coroutine system thread)

set(BOOST_BEAST_USE_STD_STRING_VIEW ON)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    add_library(httpserver STATIC
            server/HTTPServer.cpp server/HTTPServer.h
            server/HTTPListener.cpp server/HTTPListener.h
            server/HTTPSession.cpp server/HTTPSession.h
            server/HTTPServerUnit.h server/HTTPServerCertificate.h)

    if (BOOST_BEAST_USE_STD_STRING_VIEW)
        target_compile_definitions(httpserver PRIVATE BOOST_BEAST_USE_STD_STRING_VIEW)
    endif()

    target_link_libraries(httpserver absl::strings nlohmann_json ${Boost_LIBRARIES})
endif()
