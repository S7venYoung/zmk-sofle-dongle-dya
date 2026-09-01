if(CONFIG_ZMK_KEY_STATS)
  target_include_directories(app PRIVATE include)
  target_sources(app PRIVATE src/key_stats.c)
endif()

# hitsmaxft/zmk-dongle-display still uses the pre-main+dya endpoint accessor.
# Keep the external module pinned and translate that symbol at compile time.
if(CONFIG_ZMK_DONGLE_DISPLAY_CUSTOM_ANIMATION_PROVIDER)
  zephyr_compile_definitions(zmk_endpoints_selected=zmk_endpoint_get_selected)
  # The upstream mini profile supplies four complete characters. Keep only Kyo
  # in this size-constrained receiver build; the animation engine remains upstream.
  zephyr_compile_definitions(
    FIGHTER_ENABLE_TERRY=0
    FIGHTER_ENABLE_MAI=0
    FIGHTER_ENABLE_MR_KARATE=0
  )
endif()
