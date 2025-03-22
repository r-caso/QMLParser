#add_library(qml-parser STATIC)
#target_sources(qml-parser PRIVATE
#    src/parser.cpp
#    src/maps.cpp
#)
#target_include_directories(qml-parser PUBLIC 
#    include
#    QMLExpression::QMLExpression
#)
#target_link_libraries(qml-parser PUBLIC qml-lexer)

add_library(qml-parser STATIC)
target_sources(qml-parser PRIVATE
    src/parser.cpp
    src/maps.cpp
)
target_include_directories(qml-parser PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/QMLParser>
)
target_link_libraries(qml-parser PUBLIC qml-lexer)