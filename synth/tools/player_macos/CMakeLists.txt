if (APPLE)

  add_executable (player_macos MACOSX_BUNDLE)

  set (STORYBOARDS Base.lproj/Main.storyboard)
  set (IMAGES )
  set (ICONS )
  set (BUNDLE_ID "com.paul-bowen-huggett.player")

  set_target_properties(player_macos PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME          player_macos
    MACOSX_BUNDLE_BUNDLE_VERSION       "1.0"
    MACOSX_BUNDLE_COPYRIGHT            "Copyright © 2022 Paul Bowen-Huggett"
    MACOSX_BUNDLE_GUI_IDENTIFIER       "${BUNDLE_ID}"
    MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in"
    MACOSX_BUNDLE_INFO_STRING          "info string"
    MACOSX_BUNDLE_LONG_VERSION_STRING  "Version 1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"

    RESOURCE "${IMAGES};${STORYBOARDS};${ICONS}"

    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUNDLE_ID}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "player"
  )

  target_sources (player_macos PRIVATE
    AppDelegate.h
    AppDelegate.mm
    Assets.xcassets
    ViewController.h
    ViewController.mm
    player_macos.mm
    ${STORYBOARDS}
  )
  setup_target (player_macos)
  find_library (COCOA_LIBRARY Cocoa)
  find_library (AUDIOTOOLBOX_LIBRARY AudioToolbox)
  target_link_libraries (player_macos PRIVATE
    ${COCOA_LIBRARY}
    ${AUDIOTOOLBOX_LIBRARY}
    synth
  )
  target_compile_options (player_macos PRIVATE -Wno-direct-ivar-access)
endif (APPLE)
