if (APPLE)
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    add_executable (player_macos MACOSX_BUNDLE)

    set (STORYBOARDS macOS/Base.lproj/Main.storyboard)
    set (IMAGES )
    set (ICONS )
    set (BUNDLE_ID "com.paul-bowen-huggett.player")

    set_target_properties (player_macos PROPERTIES
      MACOSX_BUNDLE_BUNDLE_NAME          player_macos
      MACOSX_BUNDLE_BUNDLE_VERSION       "1.0"
      MACOSX_BUNDLE_COPYRIGHT            "Copyright © 2022 Paul Bowen-Huggett"
      MACOSX_BUNDLE_GUI_IDENTIFIER       "${BUNDLE_ID}"
      MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in"
      MACOSX_BUNDLE_INFO_STRING          "info string"
      MACOSX_BUNDLE_LONG_VERSION_STRING  "Version 1.0"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"

      RESOURCE "${IMAGES};${STORYBOARDS};${ICONS}"

      XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC Yes
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
      macOS/player_macos.entitlements
      ${STORYBOARDS}
    )
    setup_target (player_macos)
    find_library (COCOA_LIBRARY Cocoa)
    find_library (AUDIOTOOLBOX_LIBRARY AudioToolbox)
    find_library (COREMIDI_LIBRARY CoreMIDI)
    target_link_libraries (player_macos PRIVATE
      ${COCOA_LIBRARY}
      ${AUDIOTOOLBOX_LIBRARY}
      ${COREMIDI_LIBRARY}
      synth
    )
    target_compile_options (player_macos PRIVATE
      -fobjc-arc
      -Wno-direct-ivar-access
    )

  endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")



  if (SYSTEM_IS_IOS)
    add_executable (player_ios MACOSX_BUNDLE)

    set (STORYBOARDS
      iOS/Base.lproj/LaunchScreen.storyboard
      iOS/Base.lproj/Main.storyboard
    )
    set (IMAGES )
    set (ICONS )
    set (BUNDLE_ID "com.paul-bowen-huggett.player")

    set_target_properties (player_ios PROPERTIES
      MACOSX_BUNDLE_BUNDLE_NAME          player_ios
      MACOSX_BUNDLE_BUNDLE_VERSION       "1.0"
      MACOSX_BUNDLE_COPYRIGHT            "Copyright © 2022 Paul Bowen-Huggett"
      MACOSX_BUNDLE_GUI_IDENTIFIER       "${BUNDLE_ID}"
      MACOSX_BUNDLE_INFO_PLIST           "${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in"
      MACOSX_BUNDLE_INFO_STRING          "info string"
      MACOSX_BUNDLE_LONG_VERSION_STRING  "Version 1.0"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"

      RESOURCE "${IMAGES};${STORYBOARDS};${ICONS}"

      XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC Yes
      XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUNDLE_ID}"
      XCODE_ATTRIBUTE_PRODUCT_NAME "player"
    )

    target_sources (player_ios PRIVATE
      AppDelegate.h
      AppDelegate.mm
      Assets.xcassets
      ViewController.h
      ViewController.mm
      player_macos.mm
      ${STORYBOARDS}
    )
    target_sources (player_ios PRIVATE
      iOS/SceneDelegate.m iOS/SceneDelegate.h
    )
    setup_target (player_ios)
    find_library (UIKIT_LIBRARY UIKit)
    find_library (AUDIOTOOLBOX_LIBRARY AudioToolbox)
    find_library (COREMIDI_LIBRARY CoreMIDI)
    target_link_libraries (player_ios PRIVATE
      ${UIKIT_LIBRARY}
      ${AUDIOTOOLBOX_LIBRARY}
      ${COREMIDI_LIBRARY}
      synth
    )
    target_compile_options (player_ios PRIVATE
      -fobjc-arc
      -Wno-direct-ivar-access
    )

  endif (SYSTEM_IS_IOS)

endif (APPLE)
