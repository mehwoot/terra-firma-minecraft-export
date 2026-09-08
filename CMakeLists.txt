cmake_minimum_required(VERSION 4.3)

set(PROJECT_NAME "tf2-mc-export")
set(TARGET_NAME "tf2-mc-export")
set(INSTALL_DIR "$CACHE{TERRA_FIRMA_2_PLUGINS_DIR}/${PROJECT_NAME}/")

# make sure compile commands is available to language server after configure step
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_language(DEFER CALL expose_current_compile_commands)

project(${PROJECT_NAME} C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)

#TODO replace the following 2 settings with a finder, or at least DRY and catch incorrect settings
set(TERRA_FIRMA_2_PLUGINS_DIR "${sourceDir}/../land/assets/plugins" CACHE PATH "Path to folder TF2 plugins directory")
message(STATUS "Using TERRA_FIRMA_2_PLUGINS_DIR: $CACHE{TERRA_FIRMA_2_PLUGINS_DIR}")
set(TERRA_FIRMA_2_API_DIR "${sourceDir}/../land/src/Api/include" CACHE PATH "Path to folder containing the TF2 API headers")
message(STATUS "Using TERRA_FIRMA_2_API_DIR: $CACHE{TERRA_FIRMA_2_API_DIR}")


file(GLOB_RECURSE TARGET_SOURCES CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS "assets/*")

add_library(${TARGET_NAME} SHARED ${TARGET_SOURCES} ${ASSET_FILES})
target_include_directories(${TARGET_NAME} 
    PUBLIC 
        "${CMAKE_SOURCE_DIR}/src"
        "${TERRA_FIRMA_2_API_DIR}"
)

install(TARGETS ${TARGET_NAME} 
    RUNTIME DESTINATION "${INSTALL_DIR}"
)

# Install the asset folder to the 'share/MyGame/assets' folder
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/"
        DESTINATION "${INSTALL_DIR}/assets"
)

function(expose_current_compile_commands)
    if(WIN32)
        file(COPY_FILE
            "${CMAKE_BINARY_DIR}/compile_commands.json" 
            "${CMAKE_BINARY_DIR}/../compile_commands.json" 
            ONLY_IF_DIFFERENT
        )
    else()
    # Create the symlink at configure time
        file(CREATE_LINK 
            "${CMAKE_BINARY_DIR}/compile_commands.json" 
            "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json" 
            SYMBOLIC
        )
    endif()
endfunction()
