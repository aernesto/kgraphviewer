ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KGRAPHVIEWER
    SOVERSION ${KGRAPHVIEWERLIB_SOVERION}
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/config-kgraphviewer.h"
)

add_subdirectory( part )

include_directories(
    part
    ${graphviz_INCLUDE_DIRECTORIES}
)


########### kgraphviewer target ###############

set(kgraphviewer_SRCS main.cpp kgraphviewer.cpp kgraphviewerConfigDialog.cpp)

qt5_add_dbus_adaptor(kgraphviewer_SRCS org.kde.kgraphviewer.xml kgraphviewer.h KGraphViewerWindow)

kconfig_add_kcfg_files( kgraphviewer_SRCS kgraphviewersettings.kcfgc )

ki18n_wrap_ui(
  kgraphviewer_SRCS
  preferencesReload.ui
  preferencesParsing.ui
  preferencesOpenInExistingWindow.ui
  preferencesReopenPreviouslyOpenedFiles.ui
  preferencesAppearance.ui
)

ecm_qt_declare_logging_category(kgraphviewer_SRCS
    HEADER kgraphviewer_debug.h
    IDENTIFIER KGRAPHVIEWER_LOG
    CATEGORY_NAME org.kde.kgraphviewer
    DEFAULT_SEVERITY Warning
)

add_executable(kgraphviewer ${kgraphviewer_SRCS})

target_link_libraries(kgraphviewer LINK_PRIVATE
    KF5::CoreAddons
    KF5::Parts
)

install( TARGETS kgraphviewer ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install kgraphviewer files ###############

ecm_install_icons(
    ICONS
    ${CMAKE_CURRENT_SOURCE_DIR}/16-apps-kgraphviewer.png
    ${CMAKE_CURRENT_SOURCE_DIR}/32-apps-kgraphviewer.png
    DESTINATION
    ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

install( FILES kgraphviewersettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR} )
install( FILES kgraphviewerui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kgraphviewer )
install( PROGRAMS org.kde.kgraphviewer.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
install(FILES org.kde.kgraphviewer.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})


if(NOT BUILD_GRAPHEDITOR)
    return()
endif()

set(kgrapheditor_SRCS kgrapheditormain.cpp kgrapheditor.cpp kgrapheditorConfigDialog.cpp KGraphEditorNodesTreeWidget.cpp KGraphEditorElementTreeWidget.cpp)

qt5_add_dbus_adaptor(kgrapheditor_SRCS org.kde.kgrapheditor.xml kgrapheditor.h KGraphEditor)

kconfig_add_kcfg_files(kgrapheditor_SRCS kgrapheditorsettings.kcfgc)

ki18n_wrap_ui(kgrapheditor_SRCS preferencesReload.ui preferencesParsing.ui preferencesOpenInExistingWindow.ui preferencesReopenPreviouslyOpenedFiles.ui)

ecm_qt_declare_logging_category(kgrapheditor_SRCS
    HEADER kgrapheditor_debug.h
    IDENTIFIER KGRAPHEDITOR_LOG
    CATEGORY_NAME org.kde.kgrapheditor
    DEFAULT_SEVERITY Warning
)

add_executable(kgrapheditor ${kgrapheditor_SRCS})

target_link_libraries(kgrapheditor LINK_PRIVATE
    KF5::CoreAddons
    KF5::Parts
)

install( TARGETS kgrapheditor ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install kgrapheditor files ###############

install(FILES kgrapheditorsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(FILES kgrapheditorui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kgrapheditor)
install(PROGRAMS org.kde.kgrapheditor.desktop DESTINATION ${KDE_INSTALL_APPDIR} )


#add_subdirectory(plasmaapplet)

