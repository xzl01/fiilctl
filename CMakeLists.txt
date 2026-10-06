cmake_minimum_required(VERSION 3.21)
project(fiilctl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.3 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup()

# QML 源放在 qml/ 下，但资源里要落到模块根（:/qt/qml/Fiil/Ui/），
# 这样同目录类型与 Theme 单例才对模块内其它文件可见。
set(FIIL_QML
    Main.qml OverviewPage.qml EqPage.qml KeysPage.qml PairPage.qml MorePage.qml
    Card.qml Toggle.qml ActionButton.qml Pill.qml Picker.qml Segmented.qml
    BatteryRing.qml Theme.qml
)
set(FIIL_QML_PATHS "")
foreach(f IN LISTS FIIL_QML)
    list(APPEND FIIL_QML_PATHS qml/${f})
    set_source_files_properties(qml/${f} PROPERTIES QT_RESOURCE_ALIAS ${f})
endforeach()
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(fiilctl
    src/main.cpp
    src/rfcomm.cpp
    src/protocol.cpp
    src/fiilview.cpp
    src/bluetooth_devices.cpp
)

target_include_directories(fiilctl PRIVATE src)
target_compile_definitions(fiilctl PRIVATE FIILCTL_VERSION="${PROJECT_VERSION}")
target_link_libraries(fiilctl PRIVATE Qt6::Quick Qt6::QuickControls2)

qt_add_qml_module(fiilctl
    URI Fiilctl.Ui
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml   # 显式固定，Qt 6.4 默认前缀不同
    QML_FILES ${FIIL_QML_PATHS}
)

install(TARGETS fiilctl RUNTIME DESTINATION bin)
install(FILES packaging/fiilctl.desktop DESTINATION share/applications)
install(FILES packaging/fiilctl.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES LICENSE DESTINATION share/licenses/fiilctl)
