# ======================================================================================================================================
#  CMakeLists.txt 
# ======================================================================================================================================
cmake_minimum_required(VERSION 3.16)
project(suxing_2d)
#--------------------------------------------------------------------------------------------------
# 1. 项目基础和路径设置
#--------------------------------------------------------------------------------------------------
message(STATUS "Setting up project paths and basic configurations...")
option(CORE_CRL_MD "Use Cubism Core that is multithread-specific and DLL-specific version" OFF)
option(CSM_MINIMUM_DEMO "Build the minimum demo" OFF)
set(SDK_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(CORE_PATH ${SDK_ROOT_PATH}/Core)
set(FRAMEWORK_PATH ${SDK_ROOT_PATH}/Framework)
set(THIRD_PARTY_PATH ${SDK_ROOT_PATH}/Samples/OpenGL/thirdParty)
set(RES_PATH ${SDK_ROOT_PATH}/Samples/Resources)
set(COMMON_SRC_PATH ${SDK_ROOT_PATH}/Samples/Common)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME})
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "Configurations" FORCE)
set(CMAKE_SUPPRESS_REGENERATION ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(/utf-8)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force all libraries to be built as static" FORCE)

#--------------------------------------------------------------------------------------------------
# 2. 编译器和架构检测 (这部分不变)
#--------------------------------------------------------------------------------------------------
message(STATUS "Detecting compiler, architecture, and CRT...")
# ... (你的编译器检测代码，原样保留)
if(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:x64")
  set(ARCH x86_64)
elseif(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:X86")
  set(ARCH x86)
else()
  message(FATAL_ERROR "[${PROJECT_NAME}] Invalid linker flag ${CMAKE_EXE_LINKER_FLAGS}.")
endif()
if(MSVC_VERSION MATCHES 1900)
  set(COMPILER 140)
elseif(MSVC_VERSION GREATER_EQUAL 1910 AND MSVC_VERSION LESS 1920)
  set(COMPILER 141)
elseif(MSVC_VERSION GREATER_EQUAL 1920 AND MSVC_VERSION LESS 1930)
  set(COMPILER 142)
elseif(MSVC_VERSION GREATER_EQUAL 1930 AND MSVC_VERSION LESS 1950)
  set(COMPILER 143)
elseif(MSVC)
  message(FATAL_ERROR "[${PROJECT_NAME}] Unsupported Visual C++ compiler used.")
else()
  message(FATAL_ERROR "[${PROJECT_NAME}] Unsupported compiler used.")
endif()
if(CORE_CRL_MD)
  set(CRT MD)
else()
  set(CRT MT)
endif()
#--------------------------------------------------------------------------------------------------
# 3. 核心依赖项配置 (Live2D, GLEW, GLFW) (这部分不变)
#--------------------------------------------------------------------------------------------------
message(STATUS "Configuring core dependencies: Live2D, GLEW, GLFW...")
add_library(Live2DCubismCore STATIC IMPORTED)
set(CORE_LIB_SUFFIX ${CORE_PATH}/lib/windows/${ARCH}/${COMPILER})
set_target_properties(Live2DCubismCore PROPERTIES IMPORTED_LOCATION_DEBUG ${CORE_LIB_SUFFIX}/Live2DCubismCore_${CRT}d.lib IMPORTED_LOCATION_RELEASE ${CORE_LIB_SUFFIX}/Live2DCubismCore_${CRT}.lib INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include)
add_subdirectory(${THIRD_PARTY_PATH}/glew/build/cmake ${CMAKE_CURRENT_BINARY_DIR}/glew)
add_subdirectory(${THIRD_PARTY_PATH}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw)
set(FRAMEWORK_SOURCE OpenGL)
add_subdirectory(${FRAMEWORK_PATH} ${CMAKE_CURRENT_BINARY_DIR}/Framework)
target_compile_definitions(Framework PUBLIC CSM_TARGET_WIN_GL)
target_include_directories(Framework PUBLIC ${THIRD_PARTY_PATH}/glew/include)
target_link_libraries(Framework Live2DCubismCore glew_s)
#--------------------------------------------------------------------------------------------------
# 4. 第三方库配置 (ImGui, nlohmann_json, cpr) (这部分不变)
#--------------------------------------------------------------------------------------------------
message(STATUS "Configuring third-party libraries: ImGui, nlohmann_json, cpr...")
add_library(imgui STATIC libs/imgui/imgui.cpp libs/imgui/imgui_draw.cpp libs/imgui/imgui_tables.cpp libs/imgui/imgui_widgets.cpp libs/imgui/imgui_impl_glfw.cpp libs/imgui/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC libs/imgui ${THIRD_PARTY_PATH}/glfw/include)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE libs/nlohmann_json/single_include)
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
add_subdirectory(libs/zlib-ng)
set(CURL_DISABLE_TESTS ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/curl)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/cpr)
target_link_libraries(cpr PRIVATE libcurl zlibstatic)

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE libs/miniaudio) # 告诉编译器去 libs/miniaudio 找头文件
target_link_libraries(miniaudio INTERFACE ole32) 

set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg)
find_path(FFMPEG_INCLUDE_DIR libavformat/avformat.h PATHS ${FFMPEG_DIR}/include)
find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_DIR}/lib)
find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_DIR}/lib)
find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_DIR}/lib)
find_library(SWRESAMPLE_LIBRARY swresample PATHS ${FFMPEG_DIR}/lib)

if(NOT FFMPEG_INCLUDE_DIR OR NOT AVFORMAT_LIBRARY OR NOT AVCODEC_LIBRARY OR NOT AVUTIL_LIBRARY OR NOT SWRESAMPLE_LIBRARY)
    message(FATAL_ERROR "FFmpeg libraries not found in ${FFMPEG_DIR}")
endif()

# 创建一个 INTERFACE 库来代表 FFmpeg 的所有依赖
add_library(ffmpeg INTERFACE)
target_include_directories(ffmpeg INTERFACE ${FFMPEG_INCLUDE_DIR})
target_link_libraries(ffmpeg INTERFACE 
    ${AVFORMAT_LIBRARY}
    ${AVCODEC_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${SWRESAMPLE_LIBRARY}
)

#--------------------------------------------------------------------------------------------------
# 5. 构建主程序
#--------------------------------------------------------------------------------------------------
message(STATUS "Configuring main application: ${PROJECT_NAME}...")
add_executable(${PROJECT_NAME})

# 抓取源文件
file(GLOB COMMON_SOURCES "${COMMON_SRC_PATH}/*.cpp" "${COMMON_SRC_PATH}/*.hpp")
file(GLOB SRC_SOURCES "src/*.cpp" "src/*.hpp")
# 选择性编译
if (CSM_MINIMUM_DEMO)
    # ...
else ()
  message(STATUS "Building FULL demo version.")
  set(ALL_SOURCES ${COMMON_SOURCES} ${SRC_SOURCES})
  list(REMOVE_ITEM ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/mainMinimum.cpp")
  target_sources(${PROJECT_NAME} PRIVATE ${ALL_SOURCES})
endif ()

# ======================= 这是操翻所有 "No such file or directory" 的最终手段 =======================
# 递归地把 Framework/src 下的所有子目录都加到 include 路径里
file(GLOB FRAMEWORK_INCLUDE_DIRS LIST_DIRECTORIES true "${FRAMEWORK_PATH}/src/*")
foreach(dir ${FRAMEWORK_INCLUDE_DIRS})
    if(IS_DIRECTORY "${dir}")
        list(APPEND ALL_FRAMEWORK_INCLUDE_DIRS "${dir}")
    endif()
endforeach()
# ====================================================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${COMMON_SRC_PATH}
    ${FRAMEWORK_PATH}/src  # 根目录也要
    ${ALL_FRAMEWORK_INCLUDE_DIRS} # 所有子目录！
    ${CORE_PATH}/include
    ${THIRD_PARTY_PATH}/glew/include
    ${THIRD_PARTY_PATH}/glfw/include
    ${THIRD_PARTY_PATH}/stb
    libs/cpr/include
    libs/curl/include
)
#--------------------------------------------------------------------------------------------------
# 6. 链接所有库到主程序 (这部分不变)
#--------------------------------------------------------------------------------------------------
message(STATUS "Linking libraries to ${PROJECT_NAME}...")
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Framework 
    glfw 
    glew_s 
    imgui 
    nlohmann_json 
    cpr 
    ffmpeg 
    ws2_32 
    miniaudio 
    ${OPENGL_LIBRARIES}
)
target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-NODEFAULTLIB:libcmtd.lib> $<$<CONFIG:Release>:-NODEFAULTLIB:libcmt.lib>)
target_compile_options(${PROJECT_NAME} PRIVATE /MP)
#--------------------------------------------------------------------------------------------------
# 7. 构建后任务 (这部分不变)
#--------------------------------------------------------------------------------------------------
message(STATUS "Setting up post-build commands...")
set(SAMPLE_SHADER_PATH ${SDK_ROOT_PATH}/Samples/OpenGL/Shaders/Standard)
set(FRAMEWORK_SHADER_PATH ${FRAMEWORK_PATH}/src/Rendering/OpenGL/Shaders/Standard)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_PATH} $<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources COMMAND ${CMAKE_COMMAND} -E copy_directory ${SAMPLE_SHADER_PATH} $<TARGET_FILE_DIR:${PROJECT_NAME}>/SampleShaders COMMAND ${CMAKE_COMMAND} -E copy_directory ${FRAMEWORK_SHADER_PATH} $<TARGET_FILE_DIR:${PROJECT_NAME}>/FrameworkShaders)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${FFMPEG_DIR}/bin $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR})
message(STATUS "CMake configuration finished for ${PROJECT_NAME}. You can now generate the project files.")