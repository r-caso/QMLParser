cmake_minimum_required (VERSION 3.22)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (QMLParser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QMLExpression REQUIRED)

add_subdirectory(qml-lexer)
add_subdirectory(qml-parser)

add_library(QMLParser INTERFACE)
target_link_libraries(QMLParser INTERFACE qml-lexer qml-parser)
add_library(QMLParser::QMLParser ALIAS QMLParser)

# Set up installation paths
include(GNUInstallDirs)

# Export targets
install(TARGETS qml-lexer qml-parser
    EXPORT QMLParserTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the header files
install(DIRECTORY qml-lexer/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QMLParser
)
install(DIRECTORY qml-parser/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QMLParser
)

# Export targets for use by other projects
install(EXPORT QMLParserTargets
    FILE QMLParserTargets.cmake
    NAMESPACE QMLParser::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QMLParser
)

# Create and install package configuration files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QMLParserConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Create QMLParserConfig.cmake.in
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/QMLParserConfig.cmake.in"
"@PACKAGE_INIT@

include(CMakeFindDependencyMacro)
find_dependency(QMLExpression REQUIRED)

include(\"\${CMAKE_CURRENT_LIST_DIR}/QMLParserTargets.cmake\")
check_required_components(QMLParser)
")

# Configure the config file
configure_package_config_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QMLParserConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QMLParserConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QMLParser
)

# Install the config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QMLParserConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QMLParserConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QMLParser
)