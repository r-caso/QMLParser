add_library(qml-lexer STATIC)
target_sources(qml-lexer PRIVATE 
    src/lexer.cpp
    src/token.cpp
)
target_include_directories(qml-lexer PUBLIC include)
target_link_libraries(qml-lexer PUBLIC qml-expression)