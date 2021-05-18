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
            details/Details.h
            server/HTTPServer.cpp server/HTTPServer.h
            server/HTTPListener.cpp server/HTTPListener.h
            server/HTTPServerSession.cpp server/HTTPServerSession.h
            details/RootCertificates.h
            server/HTTPRequestHandler.h)

    add_library(httpclient STATIC
            client/HTTPClient.cpp client/HTTPClient.h
            client/HTTPClientSession.cpp client/HTTPClientSession.h
            client/HTTPRequestFactory.h
            client/HTTPRequest.h)

    target_link_libraries(httpserver nlohmann_json ${Boost_LIBRARIES})
    target_link_libraries(httpclient ${Boost_LIBRARIES})
endif()
