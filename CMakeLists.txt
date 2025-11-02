cmake_minimum_required(VERSION 3.19)
cmake_policy(SET CMP0091 NEW)

string(TIMESTAMP VERSION_DAY "%m%d" UTC)
string(TIMESTAMP VERSION_YEAR "%Y" UTC)
set(VERSION_MAJOR "0.1" CACHE STRINGS "")
set(VERSION_MINOR "0" CACHE STRINGS "")
set(VERSION_DOT "${VERSION_MAJOR}.${VERSION_MINOR}")
set(I_DESCRIPTION "Renew of RIV+ based on SokuLib and AssistSoku" CACHE STRINGS "")
project(ReplayInputView++
    VERSION ${VERSION_MAJOR}.${VERSION_DAY}.${VERSION_YEAR}
    DESCRIPTION ${I_DESCRIPTION}
    LANGUAGES C CXX
)

option(SUIT_4_PLAYERS OFF)
set(SOKULIB_PATH src/SokuLib CACHE PATH "sokulib path")
set(I_INI_FILENAME ${PROJECT_NAME}.ini CACHE PATH "")


string(REPLACE . , VERSION_COMMA ${VERSION_DOT},0)
string(REPLACE . , IVERSION_COMMA ${PROJECT_VERSION})
message("build ${PROJECT_NAME} version ${VERSION_DOT} | ${IVERSION_COMMA}")

# RC
set(RC_FILES "")
file(GLOB RC_IN_FILES src/${PROJECT_NAME}/*.rc.in)
#set(RC_IN_FILES src/${PROJECT_NAME}/ReplayInputView.rc)
#message(${RC_FILES})

foreach(RC_IN_FILE ${RC_IN_FILES})
    get_filename_component(RC_FILENAME ${RC_IN_FILE} NAME)
    string(REGEX REPLACE "\\.in$" "" OUTPUT_FILENAME ${RC_FILENAME})
    set(OUTPUT_FILE "${OUTPUT_FILENAME}")
    list(APPEND RC_FILES ${OUTPUT_FILE})
    configure_file(${RC_IN_FILE} ${OUTPUT_FILE} @ONLY
    )
    message(STATUS "config: ${RC_IN_FILE} -> ${OUTPUT_FILE}")
endforeach()


set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

set(GAME_FOLDER_TH123 "th123" CACHE PATH "game folder ig")
set(CMAKE_INSTALL_PREFIX ${GAME_FOLDER_TH123})
message(Install prefix: ${CMAKE_INSTALL_PREFIX})
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(SUIT_4_PLAYERS)
    add_definitions(-DSUIT_4_PLAYERS)
    add_definitions(-DPLAYERS_NUMBER=4)
else()
	add_definitions(-DPLAYERS_NUMBER=2)
endif()
add_definitions(-DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -DINI_FILENAME=L"${I_INI_FILENAME}")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing -Wno-microsoft-cast")
endif ()
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Brepro")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /Brepro")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /Brepro")

# generate pdb file for release build (https://stackoverflow.com/a/31264946)
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

# SokuLib
add_subdirectory(
    ${SOKULIB_PATH}
    src/${PROJECT_NAME}
)
set(BOOST_ROOT "S:/boost_1_89_0" CACHE PATH "")
set(Boost_NO_SYSTEM_PATHS ON)
find_package(Boost REQUIRED)
if(Boost_FOUND)
    message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Boost not found!")
endif()

file(GLOB PROJ_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
file(GLOB PROJ_HDR_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)
# Module
add_library("${PROJECT_NAME}"
    MODULE
        ${RC_FILES}
        ${PROJ_SRC_FILES}
        ${PROJ_HDR_FILES}
)
target_compile_options("${PROJECT_NAME}" PRIVATE /ZI)
target_compile_definitions("${PROJECT_NAME}" PRIVATE DIRECTINPUT_VERSION=0x0800 CURL_STATICLIB _CRT_SECURE_NO_WARNINGS $<$<CONFIG:Debug>:_DEBUG>)
target_include_directories("${PROJECT_NAME}" PRIVATE include SokuLib/directx "src/${PROJECT_NAME}" ${Boost_INCLUDE_DIRS})
target_link_directories("${PROJECT_NAME}" PRIVATE ${SOKULIB_PATH}/lib)
target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
target_link_libraries(
        "${PROJECT_NAME}"
        SokuLib
        shlwapi
        d3d9
        d3dx9
)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION modules/${PROJECT_NAME}/)
install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_SOURCE_DIR}${PROJECT_NAME}.ini DESTINATION modules/${PROJECT_NAME}/ OPTIONAL)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/${I_INI_FILENAME} DESTINATION modules/${PROJECT_NAME}/ OPTIONAL)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dat DESTINATION modules/${PROJECT_NAME}/ OPTIONAL)

#note
#before "tasklist | findstr th123 && (taskkill /IM th123.exe /F) || (exit /b 0)"
#after "cmake --install . --config Debug"
set(SHADY_CLI_PATH "shady-cli.exe" CACHE FILEPATH "")
file(GLOB SRC_RIV_Pack ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/data/*)
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
	COMMAND "${SHADY_CLI_PATH}" pack -m data
		-o "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dat"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}/data"
	DEPENDS ${SRC_RIV_Pack}
	VERBATIM
)
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND tasklist | findstr th123 && (taskkill /IM th123.exe /F) || (exit /b 0))
add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD COMMAND
    mt.exe -manifest \"${CMAKE_CURRENT_SOURCE_DIR}/src/v6.manifest\" "-inputresource:\"${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}.dll;#2\"" "-outputresource:\"${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}.dll;#2\""
  COMMENT
    "Appending manifest for common-control-v6 dependency."
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND cmake --install . --config $<CONFIG>)

if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND "${GAME_FOLDER_TH123}/th123.exe")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${GAME_FOLDER_TH123}")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${PROJ_SRC_FILES} ${PROJ_HDR_FILES})
endif()