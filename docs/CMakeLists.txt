set(DOXYGEN_HTML_OUTPUT "${CMAKE_SOURCE_DIR}/docs/doxygen")
set(DOXYGEN_PROJECT_NAME "DAQCap")
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
set(DOXYGEN_EXCLUDE_PATTERNS "*.cpp")

# TODO: Add an option for building internal documentation

option(BUILD_INTERNAL_DOCS "Build internal documentation" OFF)
if(BUILD_INTERNAL_DOCS)

    doxygen_add_docs(
        docs 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md
        COMMENT "Generate documentation"
    )

else()

    doxygen_add_docs(
        docs 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md
        COMMENT "Generate documentation"
    )

endif()