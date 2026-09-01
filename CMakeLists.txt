if(CONFIG_ZMK_KEY_STATS)
  target_include_directories(app PRIVATE include)
  target_sources(app PRIVATE src/key_stats.c)
endif()

# hitsmaxft/zmk-dongle-display still uses the pre-main+dya endpoint accessor.
# Keep the external module pinned and translate that symbol at compile time.
if(CONFIG_ZMK_DONGLE_DISPLAY_CUSTOM_ANIMATION_PROVIDER)
  zephyr_compile_definitions(zmk_endpoints_selected=zmk_endpoint_get_selected)
endif()
