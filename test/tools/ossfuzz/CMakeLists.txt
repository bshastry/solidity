add_custom_target(ossfuzz)
add_dependencies(ossfuzz solc_opt_ossfuzz solc_noopt_ossfuzz const_opt_ossfuzz yul_proto_ossfuzz)

#[[FuzzingEngine.a is provided by oss-fuzz's Dockerized build environment]]
add_executable(solc_opt_ossfuzz solc_opt_ossfuzz.cpp ../fuzzer_common.cpp)
target_link_libraries(solc_opt_ossfuzz PRIVATE libsolc evmasm FuzzingEngine.a)

add_executable(solc_noopt_ossfuzz solc_noopt_ossfuzz.cpp ../fuzzer_common.cpp)
target_link_libraries(solc_noopt_ossfuzz PRIVATE libsolc evmasm FuzzingEngine.a)

add_executable(const_opt_ossfuzz const_opt_ossfuzz.cpp ../fuzzer_common.cpp)
target_link_libraries(const_opt_ossfuzz PRIVATE libsolc evmasm FuzzingEngine.a)

add_executable(yul_proto_ossfuzz YulProtoFuzzer.cpp proto_to_yul.cpp yul_proto.pb.cc)
target_include_directories(yul_proto_ossfuzz ${SRC}/libprotobuf-mutator ${SRC}/LPM/external.protobuf/include)
link_directories(yul_proto_ossfuzz ${SRC}/LPM/src ${SRC}/LPM/src/libfuzzer ${SRC}/LPM/external.protobuf/lib)
target_link_libraries(yul_proto_ossfuzz PRIVATE libsolc evmasm
        protobuf-mutator-libfuzzer.a
        protobuf-mutator.a
        protobuf.a
        FuzzingEngine.a)