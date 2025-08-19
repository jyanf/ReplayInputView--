cmake_minimum_required(VERSION 3.19)
cmake_policy(SET CMP0091 NEW)

string(TIMESTAMP VERSION_DAY "%m%d" UTC)
string(TIMESTAMP VERSION_YEAR "%Y" UTC)
set(VERSION_MAJOR "0.1" CACHE STRINGS "")
set(VERSION_MINOR "0" CACHE STRINGS "")
set(VERSION_DOT "${VERSION_MAJOR}.${VERSION_MINOR}")

project(ReplayInputView++
    VERSION ${VERSION_MAJOR}.${VERSION_DAY}.${VERSION_YEAR}
    DESCRIPTION "Renew of RIV+ based on SokuLib and AssistSoku"
    LANGUAGES C CXX
)

string(REPLACE . , VERSION_COMMA ${VERSION_DOT},0)
string(REPLACE . , IVERSION_COMMA ${PROJECT_VERSION})
message("build ${PROJECT_NAME} version ${VERSION_DOT} | ${IVERSION_COMMA}")

# RC
set(RC_FILES "")
file(GLOB RC_IN_FILES src/${PROJECT_NAME}/*.rc.in)
#set(RC_IN_FILES src/${PROJECT_NAME}/ReplayInputView.rc)
#message(${RC_FILES})
set(PNG_PATH "ReplayInputView.png")
foreach(RC_IN_FILE ${RC_IN_FILES})
    get_filename_component(RC_FILENAME ${RC_IN_FILE} NAME)
    string(REGEX REPLACE "\\.in$" "" OUTPUT_FILENAME ${RC_FILENAME})
    set(OUTPUT_FILE "${OUTPUT_FILENAME}")
    list(APPEND RC_FILES ${OUTPUT_FILE})
    configure_file(
        ${RC_IN_FILE}
        ${OUTPUT_FILE}
        @ONLY  # 只替换 @VAR@ 形式的变量
    )
    
    # 打印调试信息
    message(STATUS "config: ${RC_IN_FILE} -> ${OUTPUT_FILE}")
endforeach()

set(SOKULIB_PATH src/SokuLib)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH "game folder ig")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(SUIT_4_PLAYERS OFF)
if(SUIT_4_PLAYERS)
    add_definitions(-DSUIT_4_PLAYERS)
    add_definitions(-DPLAYERS_NUMBER=4)
else()
	add_definitions(-DPLAYERS_NUMBER=2)
endif()

add_definitions(-DWINVER=0x0501 -D_WIN32_WINNT=0x0501)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing -Wno-microsoft-cast")
endif ()
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /Brepro")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /Brepro")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /Brepro")

# generate pdb file for release build (https://stackoverflow.com/a/31264946)
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

# SokuLib
add_subdirectory(${SOKULIB_PATH} src/${PROJECT_NAME})


# Module
add_library("${PROJECT_NAME}"
        MODULE
        src/main.cpp
        ${RC_FILES}
        "src/${PROJECT_NAME}/riv.cpp"
        "src/${PROJECT_NAME}/box.cpp")
target_compile_options("${PROJECT_NAME}" PRIVATE /Zi)
target_compile_definitions("${PROJECT_NAME}" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG>)
target_include_directories("${PROJECT_NAME}" PRIVATE include SokuLib/directx "src/${PROJECT_NAME}")
target_link_directories("${PROJECT_NAME}" PRIVATE ${SOKULIB_PATH}/lib)
target_link_libraries(
        "${PROJECT_NAME}"
        SokuLib
        shlwapi
        d3d9
        d3dx9
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION modules/${PROJECT_NAME}/ CONFIGURATIONS ${CMAKE_BUILD_TYPE})
install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_SOURCE_DIR}${PROJECT_NAME}.ini DESTINATION modules/${PROJECT_NAME}/ OPTIONAL)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}.ini DESTINATION modules/${PROJECT_NAME}/ OPTIONAL)

#note
#before "tasklist | findstr th123 && (taskkill /IM th123.exe /F) || (exit /b 0)"
#after "cmake --install . --config Debug"