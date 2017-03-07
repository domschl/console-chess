add_executable(chess chess.cpp)
target_link_libraries(chess LINK_PUBLIC Threads::Threads)
