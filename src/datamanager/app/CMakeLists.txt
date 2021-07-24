########################################################
# Files

set(QGIS_DATAMANAGER_APP_SRCS
  main.cpp
)

########################################################
# Build

add_executable (qgis_datamanager MACOSX_BUNDLE ${QGIS_DATAMANAGER_APP_SRCS})

# require c++17
target_compile_features(qgis_datamanager PRIVATE cxx_std_17)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(qgis_datamanager
  qgis_core
  qgis_gui
  qgis_datamanager_lib
)

# install

install (TARGETS qgis_datamanager
  BUNDLE DESTINATION ${QGIS_BIN_DIR}
  RUNTIME DESTINATION ${QGIS_BIN_DIR}
)

