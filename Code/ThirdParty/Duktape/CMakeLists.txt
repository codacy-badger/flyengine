pl_cmake_init()

pl_requires(PL_3RDPARTY_DUKTAPE_SUPPORT)

# Get the name of this folder as the project name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME_WE)

pl_create_target(LIBRARY ${PROJECT_NAME} NO_PL_PREFIX)

target_compile_definitions(${PROJECT_NAME} PUBLIC BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4702)
endif()
