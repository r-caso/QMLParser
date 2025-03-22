add_library(qml-lexer STATIC)
target_sources(qml-lexer PRIVATE 
    src/lexer.cpp
    src/token.cpp
)
target_include_directories(qml-lexer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/QMLParser>
)
target_link_libraries(qml-lexer PUBLIC QMLExpression::QMLExpression)