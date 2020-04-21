
option(ASIO_RAFT "Build unit tests for raft core with ASIO" OFF)
option(NS3_RAFT "Build unit tests for raft core with NS3" ON)

set(SOURCE_FILES
        src/asio_service.cxx
        src/buffer.cxx
        src/cluster_config.cxx
        src/fs_log_store.cxx
        src/peer.cxx
        src/raft_server.cxx
        src/snapshot.cxx
        src/snapshot_sync_req.cxx
        src/srv_config.cxx
        src/in_memory_log_store.cpp
        )

set(TEST_SOURCE_FILES
        tests/src/test_async_result.cxx
        tests/src/test_buffer.cxx
        tests/src/test_everything_together.cxx
        tests/src/test_impls.cxx
        tests/src/test_log_store.cxx
        tests/src/test_logger.cxx
        tests/src/test_ptr.cxx
        tests/src/test_runner.cxx
        tests/src/test_scheduler.cxx
        tests/src/test_serialization.cxx
        tests/src/test_strfmt.cxx

        )

set(HEADER_FILES
        headers/asio_service.hxx
        headers/in_memory_log_store.h
        headers/async.hxx
        headers/basic_types.hxx
        headers/buffer.hxx
        headers/cluster_config.hxx
        headers/context.hxx
        headers/cornerstone.hxx
        headers/delayed_task.hxx
        headers/delayed_task_scheduler.hxx
        headers/fs_log_store.hxx
        headers/log_entry.hxx
        headers/log_store.hxx
        headers/log_val_type.hxx
        headers/logger.hxx
        headers/msg_base.hxx
        headers/msg_type.hxx
        headers/peer.hxx
        headers/pp_util.hxx
        headers/ptr.hxx
        headers/raft_params.hxx
        headers/raft_server.hxx
        headers/req_msg.hxx
        headers/resp_msg.hxx
        headers/rpc_cli.hxx
        headers/rpc_cli_factory.hxx
        headers/rpc_exception.hxx
        headers/rpc_listener.hxx
        headers/snapshot.hxx
        headers/snapshot_sync_ctx.hxx
        headers/snapshot_sync_req.hxx
        headers/srv_config.hxx
        headers/srv_role.hxx
        headers/srv_state.hxx
        headers/state_machine.hxx
        headers/state_mgr.hxx
        headers/strfmt.hxx
        headers/timer_task.hxx

        )



set(NS3_HAEDERS
        headers/ns3_service.h
        headers/raft_node.h
        headers/raft-topology-helper.h
        headers/client_node.h
        headers/base64.h
        )
set(NS3_SOURCEFILES
        src/ns3_service.cpp
        src/raft-topology-helper.cpp
        src/raft_node.cpp
        src/client_node.cpp
        src/base64.cpp
        )
set(NS3_TESTS
        tests/src/test_ns3_raft.cpp)



message(STATUS "${NS3_HEADERS}")

set(NS3_ALL_SOURCEFILES "${NS3_TESTS}" "${NS3_SOURCEFILES}" "${SOURCE_FILES}")
set(NS3_ALL_HEADERFILES "${NS3_HEADERS}" "${HEADER_FILES}")

message(STATUS ${NS3_ALL_SOURCEFILES})
message(STATUS ${NS3_ALL_HEADERFILES})

if(ASIO_RAFT)
    add_library(cornerstone ${SOURCE_FILES} ${HEADER_FILES})
endif()

#if(NS3_RAFT)
    add_library(cornerstone_ns3 ${NS3_ALL_SOURCEFILES} ${NS3_ALL_HEADERFILES})

#endif()

#message(STATUS "CUR DIR:" ${cornerstone})

include_directories(headers rapidjsonheaders)

add_executable(test_base64 tests/src/test_base64.cpp headers/base64.h src/base64.cpp)

add_executable(test_rapidjson tests/src/test_rapidjson.cpp)

add_executable(test_ns3_scheduler tests/src/test_ns3_scheduler.cpp)

target_link_libraries(test_ns3_scheduler ${libcore} ${libpoint-to-point} ${libinternet} ${libapplications})

if(ASIO_RAFT)
    add_definitions(-DASIO_STANDALONE)
    add_definitions(-DASIO_HAS_STD_CHRONO)

    # ASIO requires threads
    find_package(Threads REQUIRED)
    include_directories(asio/asio/include)
    set(NAME raftcore-test-asio)
    set(LIBS ${cornerstone})
    add_executable("${NAME}"
            ${TEST_SOURCE_FILES}
            ${HEADER_FILES})

    add_test(NAME ${NAME} COMMAND ${NAME})
    target_link_libraries(cornerstone
            # asio
            Threads::Threads
            )
    target_link_libraries(${NAME} cornerstone)
endif(ASIO_RAFT)



#if (NS3_RAFT)
    set(NAME test_ns3_raft)
    set(LIBS cornerstone_ns3 ${libcore} ${libcsma} ${libpoint-to-point} ${libinternet} ${libapplications})
    build_example("${NAME}"  "${NS3_ALL_SOURCEFILES}" "${NS3_ALL_HEADERFILES}" "${LIBS}")
#    add_executable(${NAME} ${NS3_TESTS} ${NS3_SOURCEFILES} ${SOURCE_FILES} ${NS3_HEADERS} ${HEADER_FILES})
#add_test(NAME ${NAME} COMMAND ${NAME})
#target_link_libraries(${NAME} ${LIBS})
#endif (NS3_RAFT)