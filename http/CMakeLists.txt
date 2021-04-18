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
    include_directories(details)

    add_library(httpserver STATIC
            details/details.h
            server/HTTPServer.cpp server/HTTPServer.h
            server/HTTPListener.cpp server/HTTPListener.h
            server/HTTPSession.cpp server/HTTPSession.h
            server/HTTPServerCertificate.h
            server/HTTPRequestHandler.h)
    add_library(httpclient STATIC
            details/details.h
            client/HTTPClient.cpp client/HTTPClient.h )

    target_link_libraries(httpserver nlohmann_json ${Boost_LIBRARIES})
    target_link_libraries(httpclient ${Boost_LIBRARIES})
endif()
