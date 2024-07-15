set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(ecap DAQCap_standalone.cpp)
target_link_libraries(ecap PRIVATE DAQCap Threads::Threads)