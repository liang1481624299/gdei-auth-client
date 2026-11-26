# CMakeLists.txt — gdei-auth-client 广东第二师范学院网络客户端
# Single-source, single CMake build. Fixed Qt6, C++20, Qt Quick/QML.
# 单一源码、单一 CMake；固定 Qt6，不维护 Qt5 分支。
cmake_minimum_required(VERSION 3.16)
project(gdei-auth-client LANGUAGES CXX)

# ---- 标准 / Language standard ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---- Qt6 固定依赖 / Fixed Qt6 dependencies ----
find_package(Qt6 REQUIRED COMPONENTS
    Core Quick QuickControls2 Network Qml Svg Core5Compat
)
if(WIN32)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
endif()

# ---- macOS: drop the removed AGL.framework from Qt's link interface ----
# AGL.framework was removed from modern macOS SDKs. Qt 6.5's FindWrapOpenGL still
# references it via WrapOpenGL::WrapOpenGL, transitively linked by Qt6::Gui.
# Recursively strip any AGL reference from the imported targets' link interface.
if(APPLE)
    set_property(GLOBAL PROPERTY _gc_agl_seen "")
    function(_gc_strip_agl _tgt)
        if(NOT TARGET "${_tgt}")
            return()
        endif()
        get_property(_seen GLOBAL PROPERTY _gc_agl_seen)
        if("${_tgt}" IN_LIST _seen)
            return()
        endif()
        set_property(GLOBAL APPEND PROPERTY _gc_agl_seen "${_tgt}")
        get_target_property(_libs "${_tgt}" INTERFACE_LINK_LIBRARIES)
        if(_libs)
            set(_filtered ${_libs})
            list(FILTER _filtered EXCLUDE REGEX "AGL")
            if(NOT "${_filtered}" STREQUAL "${_libs}")
                set_target_properties("${_tgt}" PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${_filtered}")
            endif()
            foreach(_dep IN LISTS _libs)
                _gc_strip_agl("${_dep}")
            endforeach()
        endif()
    endfunction()
    get_property(_gc_imported DIRECTORY PROPERTY IMPORTED_TARGETS)
    foreach(_gc_t IN LISTS _gc_imported)
        _gc_strip_agl("${_gc_t}")
    endforeach()
    foreach(_gc_qt_mod Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Svg Qt6::Network WrapOpenGL::WrapOpenGL)
        _gc_strip_agl("${_gc_qt_mod}")
    endforeach()
endif()

# ---- 源码 / Sources ----
set(APP_SOURCES
    src/main.cpp
    src/core/Logger.cpp
    src/core/SecretStore.cpp
    src/core/secretstore_win.cpp
    src/core/secretstore_mac.cpp
    src/core/secretstore_fallback.cpp
    src/core/ConfigStore.cpp
    src/core/LocalDeviceStore.cpp
    src/core/BodyDecoder.cpp
    src/core/CertificateStore.cpp
    src/core/FormEncoding.cpp
    src/core/SessionCookieJar.cpp
    src/core/PortalClient.cpp
    src/core/DevicePortalClient.cpp
    src/core/AccountPortalClient.cpp
    src/core/NicSelector.cpp
    src/platform/RouteBinder.cpp
    src/core/SelfServiceCodec.cpp
    src/core/KeepAlive.cpp
    src/core/I18n.cpp
    src/app/AuthController.cpp
    src/app/DeviceController.cpp
    src/app/AccountController.cpp
    src/app/LocalDeviceController.cpp
    src/app/NetworkInfoController.cpp
    src/app/DiagnosticsController.cpp
    src/app/SessionController.cpp
    src/app/EventBus.cpp
    src/app/EventModel.cpp
    src/app/EventBridge.cpp
    src/app/LogController.cpp
    src/app/AppController.cpp
)

set(APP_HEADERS
    src/core/Constants.hpp
    src/core/Types.hpp
    src/core/Logger.hpp
    src/core/SecretStore.hpp
    src/core/ConfigStore.hpp
    src/core/LocalDeviceStore.hpp
    src/core/BodyDecoder.hpp
    src/core/CertificateStore.hpp
    src/core/FormEncoding.hpp
    src/core/SessionCookieJar.hpp
    src/core/PortalClient.hpp
    src/core/DevicePortalClient.hpp
    src/core/AccountPortalClient.hpp
    src/core/NicSelector.hpp
    src/platform/RouteBinder.hpp
    src/core/SelfServiceCodec.hpp
    src/core/KeepAlive.hpp
    src/core/I18n.hpp
    src/app/AuthController.hpp
    src/app/DeviceController.hpp
    src/app/AccountController.hpp
    src/app/LocalDeviceController.hpp
    src/app/NetworkInfoController.hpp
    src/app/DiagnosticsController.hpp
    src/app/SessionController.hpp
    src/app/AppEvent.hpp
    src/app/EventBus.hpp
    src/app/EventModel.hpp
    src/app/EventBridge.hpp
    src/app/LogController.hpp
    src/app/AppController.hpp
    src/core/msvc_stdext_compat.hpp
)
if(WIN32)
    list(APPEND APP_SOURCES src/platform/TrayController.cpp)
    list(APPEND APP_HEADERS src/platform/TrayController.hpp)
endif()

qt_add_executable(gdei-auth-client WIN32 MACOSX_BUNDLE ${APP_SOURCES} ${APP_HEADERS})

target_include_directories(gdei-auth-client PRIVATE src)

# ---- 应用图标 / Application icons ----
# Windows: 多尺寸 ico 经资源脚本编译进 exe。
if(WIN32)
    target_sources(gdei-auth-client PRIVATE packaging/windows/app.rc)
endif()
# macOS: bundle 图标名；app.icns 由 packaging/macos/deploy.sh 从
# assets/app.iconset 用 iconutil 生成并放入 Contents/Resources。
if(APPLE)
    set_target_properties(gdei-auth-client PROPERTIES
        MACOSX_BUNDLE_ICON_FILE app.icns)
endif()

# ---- QML 模块 / QML module ----
qt_add_qml_module(gdei-auth-client
    URI GdeiConnect
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/pages/LoginPage.qml
        qml/pages/OverviewPage.qml
        qml/pages/DevicesPage.qml
        qml/pages/DiscoverPage.qml
        qml/pages/MinePage.qml
        qml/pages/AdvancedSettingsPage.qml
        qml/pages/EventLogPage.qml
        qml/pages/LogPage.qml
        qml/pages/AboutPage.qml
        qml/pages/RemoteDevicesPage.qml
        qml/pages/DiagnosticsPage.qml
        qml/theme/Theme.qml
        qml/components/BaseCard.qml
        qml/components/StatusListCard.qml
        qml/components/BaseButton.qml
        qml/components/BaseTextField.qml
        qml/components/BaseSwitch.qml
        qml/components/BaseComboBox.qml
        qml/components/BaseCollapseGroup.qml
        qml/components/BaseBadge.qml
        qml/components/Banner.qml
        qml/components/BaseSkeleton.qml
        qml/components/DiagnosticToolCard.qml
        qml/components/LangSelector.qml
        qml/components/ThemeSelector.qml
        qml/components/DeviceItem.qml
        qml/components/DevicePanel.qml
        qml/components/DeviceCard.qml
        qml/components/LogPanel.qml
        qml/components/Icon.qml
        qml/components/BrandIcon.qml
        qml/components/AppDialog.qml
        qml/components/SettingRow.qml
        qml/components/PageScaffold.qml
        qml/components/AppTabBar.qml
        qml/components/AppTabButton.qml
        qml/components/NavEntryRow.qml
        qml/components/TextLink.qml
        qml/components/IconGrid.qml
        qml/components/StatStrip.qml
        qml/components/KeyValueRow.qml
        qml/components/AvatarBlock.qml
        qml/components/LocalDeviceEditor.qml
        qml/components/NetworkInfoCard.qml
        qml/components/LocalProfilesCard.qml
        qml/components/ToastHost.qml
        qml/components/EventListItem.qml
        qml/components/UnreadBadge.qml
    RESOURCES
        qml/icons/person.svg
        qml/icons/lock.svg
        qml/icons/globe.svg
        qml/icons/power.svg
        qml/icons/refresh.svg
        qml/icons/check.svg
        qml/icons/download.svg
        qml/icons/chevronDown.svg
        qml/icons/chevronLeft.svg
        qml/icons/chevronRight.svg
        qml/icons/home.svg
        qml/icons/wifi.svg
        qml/icons/wifiOff.svg
        qml/icons/trash.svg
        qml/icons/list.svg
        qml/icons/server.svg
        qml/icons/bolt.svg
        qml/icons/plus.svg
        qml/icons/info.svg
        qml/icons/clock.svg
        qml/icons/shield.svg
        qml/icons/ellipsis.svg
        qml/icons/xmark.svg
        qml/icons/slider.svg
        qml/icons/eye.svg
        qml/icons/eyeOff.svg
        qml/icons/trayOnline.svg
        qml/icons/trayOffline.svg
        qml/icons/bell.svg
        qml/icons/sun.svg
        qml/icons/cog.svg
        qml/icons/chart.svg
        qml/fonts/SimSun.ttc
        qml/fonts/SegoeUI.ttf
        qml/fonts/SegoeUI-Bold.ttf
        qml/fonts/Meiryo.ttc
        qml/fonts/MicrosoftJhengHei.ttc
        qml/icons/app.png
        qml/icons/brand/windows.svg
        qml/icons/brand/macos.svg
        qml/icons/brand/android.svg
        qml/icons/brand/linux.svg
        qml/icons/brand/debian.svg
        qml/icons/brand/arch.svg
        qml/icons/brand/ubuntu.svg
)

target_link_libraries(gdei-auth-client PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Qml
    Qt6::Svg
    Qt6::Core5Compat
)

# ---- QML 编译产物 / QML compiled output ----
# 本项目大量通过 rootContext 对象 `app` 暴露 C++ 属性（i18n.lang、config.themeMode
# 等）。qmlcachegen 的 AOT 原生代码无法解析这类上下文对象，会把相关绑定跳过原生编译，
# 进而在部分路径上丢失对 langChanged / themeModeChanged 的重算，表现为切换语言不刷新、
# 跟随系统主题失效。这里关闭 AOT 原生代码生成，仅保留字节码缓存（解释执行），
# 让绑定回到 QML 引擎的常规依赖追踪；性能损失在本项目 UI 规模下可忽略。
# AOT native code cannot resolve context object `app`; keep bytecode-only so the
# QML engine tracks lang/theme property dependencies normally.
set_target_properties(gdei-auth-client PROPERTIES
    QT_QMLCACHEGEN_ARGUMENTS "--only-bytecode")

# ---- Platform secure-storage libraries ----
# Windows DPAPI (Crypt32) / macOS Keychain (Security framework).
# Windows 另需 iphlpapi：NetworkInfoController 用 GetIpNetTable 读取邻接表获取网关 MAC。
if(WIN32)
    target_link_libraries(gdei-auth-client PRIVATE crypt32 iphlpapi Qt6::Widgets)
elseif(APPLE)
    target_link_libraries(gdei-auth-client PRIVATE "-framework Security")
endif()

# ---- MSVC: force-include the stdext iterator polyfill ----
# New MSVC toolsets removed stdext::make_checked_array_iterator; Qt 6.5 still
# references it on MSVC, so pull in the compat header before any Qt header.
if(MSVC)
    target_compile_options(gdei-auth-client PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/core/msvc_stdext_compat.hpp"
        /utf-8)
elseif(MINGW)
    target_compile_options(gdei-auth-client PRIVATE
        -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# ---- Android 包与自适应配置 / Android package and rotation support ----
if(ANDROID)
    set_target_properties(gdei-auth-client PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_PACKAGE_NAME "org.gdei.connect"
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_TARGET_SDK_VERSION 35
        QT_ANDROID_VERSION_CODE 2
        QT_ANDROID_VERSION_NAME "0.3.0")
endif()

# ---- Qt 运行时部署 / Qt runtime deployment ----
# 注意：不要在 POST_BUILD 阶段对编译树内的 exe 直接跑 windeployqt。那样会把
# Qt DLL / 插件 / QML 模块倾倒进 out-of-source 编译目录，污染后续编译器子进程
# 的 DLL 加载（MinGW g++ 子进程会在该目录加载到不符版本 DLL 而静默退出）。
# 运行时捆绑统一交给打包阶段：packaging/windows/deploy.ps1 先把 exe 复制到
# 暂存目录，再对暂存副本执行 windeployqt，编译树保持干净。
# NOTE: do NOT run windeployqt against the in-tree exe in POST_BUILD. That dumps Qt
# DLLs/plugins/QML into the out-of-source build dir and breaks later compiler
# subprocess DLL resolution (MinGW g++ silently exits when loading mismatched DLLs
# from that cwd). Bundling is deferred to packaging/windows/deploy.ps1, which copies
# the exe to a staging dir and runs windeployqt on that staging copy.

# CPack / `cmake --install` 路径的桌面部署（Windows / macOS / Linux 通用）。
# Android 由 apk target 调用 androiddeployqt，不支持此桌面部署函数。
set(_gc_deploy_script "")
# 该宏的参数集合随 Qt 版本变化：6.4 旧宏不接受 OUTPUT_SCRIPT，6.5 尚不识别
# MACOS_BUNDLE_POST_PROCESSING；且生成的脚本只在 cpack/install 阶段使用，CI 的
# Configure/Build 并不依赖。旧版 Qt 直接跳过，运行时部署交由各平台 packaging/ 脚本。
if(COMMAND qt_generate_deploy_qml_app_script AND NOT ANDROID
   AND QT_VERSION_MAJOR EQUAL 6 AND QT_VERSION_MINOR GREATER_EQUAL 5)
    qt_generate_deploy_qml_app_script(
        TARGET gdei-auth-client
        OUTPUT_SCRIPT _gc_deploy_script)
endif()

# ---- 安装 / Install ----
install(TARGETS gdei-auth-client
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
if(_gc_deploy_script)
    install(SCRIPT "${_gc_deploy_script}")
endif()

# ---- 打包 / Packaging (deb / rpm / Arch tar.zst / macOS dmg / Windows exe) ----
set(CPACK_PACKAGE_NAME "gdei-auth-client")
set(CPACK_PACKAGE_VERSION "0.3.0")
set(CPACK_PACKAGE_VENDOR "GDEI")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GDEI Network Client, campus network auto-authentication")
set(CPACK_PACKAGE_CONTACT "liang1481624299 <liang1481624299@users.noreply.github.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "liang1481624299 <liang1481624299@users.noreply.github.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    # Debian / RPM / 压缩包；Arch pkg.tar.zst 由打包脚本基于 tar 产物生成。
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

include(CPack)
