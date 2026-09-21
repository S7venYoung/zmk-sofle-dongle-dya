if(CONFIG_ZMK_KEY_STATS OR CONFIG_ZMK_CUSTOM_SETTINGS)
  target_include_directories(app PRIVATE include)
endif()

if(CONFIG_ZMK_KEY_STATS)
  target_sources(app PRIVATE src/key_stats.c)
endif()

if(CONFIG_ZMK_CUSTOM_SETTINGS)
  target_sources(app PRIVATE src/display_settings.c)
endif()

if(CONFIG_ZMK_STUDIO_RPC)
  target_sources(app PRIVATE src/codex_metrics_rpc.c)

  list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
  include(nanopb)
  set(NANOPB_GENERATE_CPP_APPEND_PATH TRUE)
  set(NANOPB_GENERATE_CPP_STANDALONE OFF)

  zephyr_library()
  file(GLOB_RECURSE CODEX_METRICS_PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
  nanopb_generate_cpp(codex_metrics_proto_srcs codex_metrics_proto_hdrs
                       RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${CODEX_METRICS_PROTO_FILES})
  target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_sources(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${codex_metrics_proto_srcs}
                 ${codex_metrics_proto_hdrs})
  target_include_directories(app PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/proto)
  add_dependencies(app ${ZEPHYR_CURRENT_LIBRARY})
endif()

add_subdirectory(modules/zmk-feature-runtime-macro)
