# - Try to find dlite
#
# Once done this will define:
#
#   DLITE_FOUND - system has DLITE
#   DLITE_INCLUDE_DIRS - the DLITE include directory
#   DLITE_LIBRARIES - Link these to use DLITE
#   DLITE_LIBRARY_DIR - Add to link_directories
#   DLITE_RUNTIME_DIR - Directory with executables (and DLLs on Windows)
#   DLITE_TEMPLATE_DIR - Directory where templates are stored
#   DLITE_STORAGE_PLUGINS - Directory path for storage plugins
#   DLITE_TRANSLATOR_PLUGINS - Directory path for translator plugins
#   DLITE_ROOT - Root of install directory
#
# Additionally the following variables will be defined, used for setting
# up the environment for DLite
#
#   dlite_PATH
#   dlite_LD_LIBRARY_PATH
#   dlite_STORAGE_PLUGINS
#   dlite_TRANSLATOR_PLUGINS
#
# DLITE_ROOT may be set at call time.
#
#

if(DLITE_LIBRARIES AND DLITE_INCLUDE_DIRS)
  # in cache already
  set(DLITE_FOUND TRUE)
else()

  # DLITE_INCLUDE_DIRS
  find_path(DLITE_INCLUDE_DIR
    NAMES
      dlite.h
    PATHS
      ${DLITE_ROOT}/include/dlite
      $ENV{HOME}/.local/include/dlite
      /usr/include/dlite
      /usr/local/include/dlite
      /opt/local/include/dlite
      /sw/include/dlite
    )
  list(APPEND DLITE_INCLUDE_DIRS ${DLITE_INCLUDE_DIR})

  # DLITE_ROOT
  get_filename_component(INCLUDES ${DLITE_INCLUDE_DIR} DIRECTORY)
  get_filename_component(DLITE_ROOT ${INCLUDES} DIRECTORY)

  # DLITE_LIBRARY_DIR
  set(DLITE_LIBRARY_DIR ${DLITE_ROOT}/lib)

  # DLITE_RUNTIME_DIR
  set(DLITE_RUNTIME_DIR ${DLITE_ROOT}/bin)

  # DLITE_TEMPLATE_DIR
  set(DLITE_TEMPLATE_DIR ${DLITE_ROOT}/share/dlite/templates)

  # DLITE_STORAGE_PLUGINS
  set(DLITE_STORAGE_PLUGINS ${DLITE_ROOT}/share/dlite/storage-plugins)

  # DLITE_TRANSLATOR_PLUGINS
  set(DLITE_TRANSLATOR_PLUGINS ${DLITE_ROOT}/share/dlite/translator-plugins)

  # DLITE_LIBRARIES
  find_library(DLITE_LIBRARY
    NAMES dlite
    PATHS ${DLITE_LIBRARY_DIR}
    )
  find_library(DLITE_UTILS_LIBRARY
    NAMES dlite-utils
    PATHS ${DLITE_LIBRARY_DIR}
    )
  list(APPEND DLITE_LIBRARIES ${DLITE_LIBRARY} ${DLITE_UTILS_LIBRARY})

  # Find HDF5 and Jansson
  # FIXME - when dlite is using dynamic loaded plugins we should:
  #     Windows: append Jansson and HDF5 runtime library paths to dlite_PATH
  #     Linux:   append Jansson and HDF5 library paths to dlite_LD_LIBRARY_PATH
  find_library(JANSSON_LIBRARY
    NAMES
      jansson
    PATHS
      ${JANSSON_ROOT}/lib
      $ENV{HOME}/.local/lib
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
    )
  list(APPEND JANSSON_LIBRARIES ${JANSSON_LIBRARY})

  find_package(HDF5 COMPONENTS C)

  list(APPEND DLITE_LIBRARIES
    ${JANSSON_LIBRARIES}
    ${HDF5_LIBRARIES}
    )


  # Define variables for setting up environment needed by dlite
  if(WINDOWS)
    set(dlite_PATH
      "${DLITE_RUNTIME_DIR};${DLITE_LIBRARY_DIR};$ENV{PATH}")
    set(dlite_LD_LIBRARY_PATH
      "$ENV{LD_LIBRARY_PATH}")
    set(dlite_STORAGE_PLUGINS
      "${DLITE_STORAGE_PLUGINS};$ENV{DLITE_STORAGE_PLUGINS}")
    set(dlite_TRANSLATOR_PLUGINS
      "${DLITE_TRANSLATOR_PLUGINS};$ENV{DLITE_TRANSLATOR_PLUGINS}")
  else()
    set(dlite_PATH
      "${DLITE_RUNTIME_DIR}:$ENV{PATH}")
    set(dlite_LD_LIBRARY_PATH
      "${DLITE_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}")
    set(dlite_STORAGE_PLUGINS
      "${DLITE_STORAGE_PLUGINS}:$ENV{DLITE_STORAGE_PLUGINS}")
    set(dlite_TRANSLATOR_PLUGINS
      "${DLITE_TRANSLATOR_PLUGINS}:$ENV{DLITE_TRANSLATOR_PLUGINS}")
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(DLITE DEFAULT_MSG
    DLITE_INCLUDE_DIRS
    DLITE_LIBRARIES
    DLITE_LIBRARY_DIR
    DLITE_RUNTIME_DIR
    DLITE_TEMPLATE_DIR
    DLITE_STORAGE_PLUGINS
    DLITE_TRANSLATOR_PLUGINS
    DLITE_ROOT

    dlite_PATH
    dlite_LD_LIBRARY_PATH
    dlite_STORAGE_PLUGINS
    dlite_TRANSLATOR_PLUGINS
    )

  # Show the DLITE_INCLUDE_DIRS and DLITE_LIBRARIES variables only in
  # the advanced view
  mark_as_advanced(
    DLITE_INCLUDE_DIRS
    DLITE_LIBRARIES
    DLITE_LIBRARY_DIR
    DLITE_RUNTIME_DIR
    DLITE_TEMPLATE_DIR
    DLITE_STORAGE_PLUGINS
    DLITE_TRANSLATOR_PLUGINS
    DLITE_ROOT

    dlite_PATH
    dlite_LD_LIBRARY_PATH
    dlite_STORAGE_PLUGINS
    dlite_TRANSLATOR_PLUGINS
    )

endif()
