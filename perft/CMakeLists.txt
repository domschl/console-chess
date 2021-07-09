add_executable(perft perft.cpp)
target_link_libraries(perft LINK_PUBLIC Threads::Threads)
