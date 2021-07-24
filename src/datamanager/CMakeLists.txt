set(QGIS_DATAMANAGER_SRCS
  qgsdatamanagerdialog.cpp
)

set(QGIS_DATAMANAGER_HDRS
  qgsdatamanagerdialog.h
)

set(QGIS_DATAMANAGER_PRIVATE_HDRS
)

set_property(GLOBAL PROPERTY QGIS_DATAMANAGER_HDRS ${QGIS_DATAMANAGER_HDRS})

set(QGIS_DATAMANAGER_UI_HDRS
)

#############################################################
# qgis_datamanager library

add_library(qgis_datamanager ${LIBRARY_TYPE}
  ${QGIS_DATAMANAGER_SRCS} ${QGIS_DATAMANAGER_HDRS} ${QGIS_DATAMANAGER_PRIVATE_HDRS}
)

# require c++17
target_compile_features(qgis_datamanager PRIVATE cxx_std_17)

target_include_directories(qgis_datamanager SYSTEM PUBLIC
)

target_include_directories(qgis_datamanager SYSTEM PRIVATE
  ${QT_QTUITOOLS_INCLUDE_DIR}
)

target_include_directories(qgis_datamanager PUBLIC
  ${CMAKE_SOURCE_DIR}/src/datamanager

  ${CMAKE_BINARY_DIR}/src/datamanager
  ${CMAKE_BINARY_DIR}/src/ui
)

GENERATE_EXPORT_HEADER(
   qgis_datamanager
   BASE_NAME DATAMANAGER
   EXPORT_FILE_NAME qgis_datamanager.h
)

set(QGIS_DATAMANAGER_HDRS ${QGIS_DATAMANAGER_HDRS} ${CMAKE_CURRENT_BINARY_DIR}/qgis_datamanager.h)

# make sure that UI files will be processed first
add_dependencies(qgis_datamanager ui)

target_link_libraries(qgis_datamanager
  qgis_core
  qgis_gui
  qgis_native
)

target_compile_definitions(qgis_datamanager PRIVATE "-DQT_NO_FOREACH")

# clang-tidy
if(CLANG_TIDY_EXE)
  set_target_properties(
    qgis_datamanager PROPERTIES
    CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
  )
endif()

# install

install(TARGETS qgis_datamanager
  RUNTIME DESTINATION ${QGIS_BIN_DIR}
  LIBRARY DESTINATION ${QGIS_LIB_DIR}
  ARCHIVE DESTINATION ${QGIS_LIB_DIR}
  FRAMEWORK DESTINATION ${QGIS_FW_SUBDIR}
  PUBLIC_HEADER DESTINATION ${QGIS_INCLUDE_DIR})

# ui headers to install, don't exist at target definition time, install manually
if (APPLE AND QGIS_MACAPP_FRAMEWORK)
  install(FILES ${QGIS_DATAMANAGER_UI_HDRS} DESTINATION ${QGIS_FW_SUBDIR}/qgis_datamanager.framework/Headers)
else()
  install(FILES ${QGIS_DATAMANAGER_UI_HDRS} DESTINATION ${QGIS_INCLUDE_DIR})
endif()

# Mac dev frameworks

if (APPLE AND QGIS_MACAPP_INSTALL_DEV)
  install(TARGETS qgis_datamanager FRAMEWORK DESTINATION ${QGIS_MACAPP_DEV_PREFIX})
  install(FILES ${QGIS_DATAMANAGER_UI_HDRS} DESTINATION ${QGIS_MACAPP_DEV_PREFIX}/qgis_datamanager.framework/Headers)
  install(CODE "execute_process(COMMAND install_name_tool -id \"${QGIS_MACAPP_DEV_PREFIX}/qgis_datamanager.framework/Versions/${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}/qgis_datamanager\" \"$ENV{DESTDIR}${QGIS_MACAPP_DEV_PREFIX}/qgis_datamanager.framework/qgis_datamanager\")")
  install(CODE "execute_process(COMMAND install_name_tool -change \"${CMAKE_INSTALL_NAME_DIR}/qgis_core.framework/Versions/${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}/qgis_core\" \"${QGIS_MACAPP_DEV_PREFIX}/qgis_core.framework/Versions/${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}/qgis_core\" \"$ENV{DESTDIR}${QGIS_MACAPP_DEV_PREFIX}/qgis_datamanager.framework/qgis_datamanager\")")
endif()
