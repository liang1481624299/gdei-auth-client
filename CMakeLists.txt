# CMakeLists.txt — gdei-connect 校园网自动认证客户端
# Single-source, single CMake build. Fixed Qt6, C++20, Qt Quick/QML.
# 单一源码、单一 CMake；固定 Qt6，不维护 Qt5 分支。
cmake_minimum_required(VERSION 3.16)
project(gdei-connect LANGUAGES CXX)

# ---- 标准 / Language standard ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ---- Qt6 固定依赖 / Fixed Qt6 dependencies ----
find_package(Qt6 REQUIRED COMPONENTS
    Core Quick QuickControls2 Network Qml Svg
)

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
    src/core/Crypto.cpp
    src/core/ConfigStore.cpp
    src/core/SessionCookieJar.cpp
    src/core/PortalClient.cpp
    src/core/KeepAlive.cpp
    src/core/I18n.cpp
    src/app/AuthController.cpp
    src/app/LogController.cpp
    src/app/AppController.cpp
)

set(APP_HEADERS
    src/core/Constants.hpp
    src/core/Types.hpp
    src/core/Logger.hpp
    src/core/Crypto.hpp
    src/core/ConfigStore.hpp
    src/core/SessionCookieJar.hpp
    src/core/PortalClient.hpp
    src/core/KeepAlive.hpp
    src/core/I18n.hpp
    src/app/AuthController.hpp
    src/app/LogController.hpp
    src/app/AppController.hpp
    src/core/msvc_stdext_compat.hpp
)

qt_add_executable(gdei-connect WIN32 MACOSX_BUNDLE ${APP_SOURCES} ${APP_HEADERS})

target_include_directories(gdei-connect PRIVATE src)

# ---- QML 模块 / QML module ----
qt_add_qml_module(gdei-connect
    URI GdeiConnect
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/components/Card.qml
        qml/components/TextField.qml
        qml/components/PrimaryButton.qml
        qml/components/Switch.qml
        qml/components/LangSelector.qml
        qml/components/LogPanel.qml
)

target_link_libraries(gdei-connect PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Qml
    Qt6::Svg
)

# ---- MSVC: force-include the stdext iterator polyfill ----
# New MSVC toolsets removed stdext::make_checked_array_iterator; Qt 6.5 still
# references it on MSVC, so pull in the compat header before any Qt header.
if(MSVC)
    target_compile_options(gdei-connect PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/core/msvc_stdext_compat.hpp")
endif()

# ---- 安装 / Install ----
install(TARGETS gdei-connect
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# ---- 打包 / Packaging (deb / rpm / Arch tar.zst / macOS dmg / Windows exe) ----
set(CPACK_PACKAGE_NAME "gdei-connect")
set(CPACK_PACKAGE_VERSION "0.1.1")
set(CPACK_PACKAGE_VENDOR "GDEI")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GDEI campus network auto-auth client")
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
