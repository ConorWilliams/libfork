cmake_minimum_required(VERSION 3.16...3.22)

project(openFLY_DOCS)

get_filename_component(ABSOLUTE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../include ABSOLUTE)

# Add the cmake folder so custom
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" ${CMAKE_MODULE_PATH})

find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYGEN_STRIP_FROM_INC_PATH "${ABSOLUTE_PATH}/")
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# Doxygen won't create these for us
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/latex)
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/html)
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/xml)

file(GLOB_RECURSE FLY_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.hpp")

# Only regenerate Doxygen when the Doxyfile or public headers change
add_custom_command(
    OUTPUT ${DOXYGEN_INDEX_FILE}
    DEPENDS ${FLY_HEADERS}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
    MAIN_DEPENDENCY ${DOXYFILE_OUT}
    ${DOXYFILE_IN}
    COMMENT "Generating doxygen XML"
    VERBATIM
)

# Nice named target so we can run the job easily
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

file(GLOB_RECURSE FLY_EXAMPLES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../examples/*.cpp")

file(GLOB_RECURSE FLY_RSTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.rst")

# Only regenerate Sphinx when: - Doxygen has rerun - Our doc files have been updated - The Sphinx
# config has been updated
add_custom_command(
    OUTPUT ${SPHINX_INDEX_FILE}
    COMMAND ${SPHINX_EXECUTABLE} -b html -Dbreathe_projects.openFLY=${DOXYGEN_OUTPUT_DIR}/xml
    ${SPHINX_SOURCE} ${SPHINX_BUILD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${FLY_RSTS} ${DOXYGEN_INDEX_FILE} ${FLY_EXAMPLES}
    MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
    COMMENT "Generating documentation with Sphinx"
)

# Nice named target so we can run the job easily
add_custom_target(docs ALL DEPENDS ${SPHINX_INDEX_FILE})
