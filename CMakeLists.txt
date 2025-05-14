link_directories(${PROJECT_SOURCE_DIR}/libs)

file(GLOB_RECURSE SOURCES "src/*.h" "src/*.cpp")

include(VersionGenerator.cmake)

generate_product_version(VersionInfo_ploader
        NAME "Plugin Loader"
        VERSION_MAJOR 1
        VERSION_MINOR 7
        VERSION_PATCH 0
        VERSION_REVISION 0
        COMPANY_NAME "PLUGINLOADER"
        ORIGINAL_FILENAME "PLUGINLOADER.DLL"
        FILE_DESCRIPTION "Plugin Libray Loader"
)

add_library(ploader MODULE ${SOURCES} ${VersionInfo_ploader})

target_link_libraries(ploader Common)

set_target_properties(ploader
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/isro/Loader"
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/isro/Loader"
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/isro/Loader"
        OUTPUT_NAME "ploader"
)

add_custom_command(
        TARGET ploader
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ploader> H:\\LASTROGUE_120_dev\\UNPACK\\PluginLoader.dll
)

add_compile_options(/EHa) # enable C++ EH (w/ SEH exceptions)