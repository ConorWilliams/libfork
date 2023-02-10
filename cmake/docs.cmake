
# # ---- Declare documentation target ----

get_filename_component(ABSOLUTE_PATH ${CMAKE_SOURCE_DIR}/include ABSOLUTE)

# Add the cmake folder for the find_package calls
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(Doxygen REQUIRED)
find_package(Sphinx REQUIRED)

set(DOXYGEN_INPUT_DIR ${CMAKE_SOURCE_DIR}/include)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYGEN_STRIP_FROM_INC_PATH "${ABSOLUTE_PATH}/")
set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile)

# Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# Doxygen won't create these for us
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/latex)
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/html)
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/xml)


file(GLOB_RECURSE FORK_HEADERS CONFIGURE_DEPENDS "${DOXYGEN_INPUT_DIR}/*.hpp")

# Only regenerate Doxygen when the Doxyfile or public headers change
add_custom_command(
    OUTPUT ${DOXYGEN_INDEX_FILE}
    DEPENDS ${FORK_HEADERS}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
    MAIN_DEPENDENCY ${DOXYFILE_OUT}
    ${DOXYFILE_IN}
    COMMENT "Generating doxygen XML"
    VERBATIM
)

# Nice named target so we can run the job easily
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_SOURCE_DIR}/docs)
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

file(GLOB_RECURSE FORK_RSTS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/docs/*.rst")

# Only regenerate Sphinx when: - Doxygen has rerun - Our doc files have been updated - The Sphinx
# config has been updated
add_custom_command(
    OUTPUT ${SPHINX_INDEX_FILE}
    COMMAND ${SPHINX_EXECUTABLE} -b html -Dbreathe_projects.Forkpool=${DOXYGEN_OUTPUT_DIR}/xml
    ${SPHINX_SOURCE} ${SPHINX_BUILD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${FORK_RSTS} ${DOXYGEN_INDEX_FILE}
    MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
    COMMENT "Generating documentation with Sphinx"
)

# Nice named target so we can run the job easily
add_custom_target(docs ALL DEPENDS ${SPHINX_INDEX_FILE})

