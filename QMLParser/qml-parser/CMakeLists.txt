add_library(qml-parser STATIC)
target_sources(qml-parser PRIVATE
    src/parser.cpp
    src/maps.cpp
)
target_include_directories(qml-parser PUBLIC include)
target_link_libraries(qml-parser PUBLIC qml-lexer)