set(SOURCES
    main.cpp
    Input.cpp
    App.cpp
    Messages.cpp
    Renderer.cpp
    util/String.cpp)

add_executable(trost ${SOURCES})
target_include_directories(trost PRIVATE ${CMAKE_CURRENT_LIST_DIR})
