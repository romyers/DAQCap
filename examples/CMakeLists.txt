

# TODO: We need to do setcap stuff. See:
# https://stackoverflow.com/questions/41254902/cmake-run-script-for-install-target
# https://stackoverflow.com/questions/31540189/is-it-possible-to-execute-a-command-as-a-super-user-while-using-cmake
# https://stackoverflow.com/questions/72012474/integrate-granting-of-capabilities-into-the-build-process

# TODO: Read this on linux capabilities:
# https://book.hacktricks.xyz/linux-hardening/privilege-escalation/linux-capabilities
# Also potentially a good read on cybersecurity

add_executable(ecap DAQCap_standalone.cpp)
target_link_libraries(ecap PRIVATE DAQCap)