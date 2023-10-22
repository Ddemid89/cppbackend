
#################
###  BOOST
#################
set(CONAN_BOOST_ROOT_DEBUG "/home/aleksey/.conan/data/boost/1.78.0/_/_/package/8cc3305c27e5ff838d1c7590662e309638310dfc")
set(CONAN_INCLUDE_DIRS_BOOST_DEBUG "/home/aleksey/.conan/data/boost/1.78.0/_/_/package/8cc3305c27e5ff838d1c7590662e309638310dfc/include")
set(CONAN_LIB_DIRS_BOOST_DEBUG "/home/aleksey/.conan/data/boost/1.78.0/_/_/package/8cc3305c27e5ff838d1c7590662e309638310dfc/lib")
set(CONAN_BIN_DIRS_BOOST_DEBUG )
set(CONAN_RES_DIRS_BOOST_DEBUG )
set(CONAN_SRC_DIRS_BOOST_DEBUG )
set(CONAN_BUILD_DIRS_BOOST_DEBUG )
set(CONAN_FRAMEWORK_DIRS_BOOST_DEBUG )
set(CONAN_LIBS_BOOST_DEBUG boost_contract boost_coroutine boost_fiber_numa boost_fiber boost_context boost_graph boost_iostreams boost_json boost_locale boost_log_setup boost_log boost_math_c99 boost_math_c99f boost_math_c99l boost_math_tr1 boost_math_tr1f boost_math_tr1l boost_nowide boost_program_options boost_random boost_regex boost_stacktrace_addr2line boost_stacktrace_backtrace boost_stacktrace_basic boost_stacktrace_noop boost_timer boost_type_erasure boost_thread boost_chrono boost_container boost_date_time boost_unit_test_framework boost_prg_exec_monitor boost_test_exec_monitor boost_exception boost_wave boost_filesystem boost_atomic boost_wserialization boost_serialization)
set(CONAN_PKG_LIBS_BOOST_DEBUG boost_contract boost_coroutine boost_fiber_numa boost_fiber boost_context boost_graph boost_iostreams boost_json boost_locale boost_log_setup boost_log boost_math_c99 boost_math_c99f boost_math_c99l boost_math_tr1 boost_math_tr1f boost_math_tr1l boost_nowide boost_program_options boost_random boost_regex boost_stacktrace_addr2line boost_stacktrace_backtrace boost_stacktrace_basic boost_stacktrace_noop boost_timer boost_type_erasure boost_thread boost_chrono boost_container boost_date_time boost_unit_test_framework boost_prg_exec_monitor boost_test_exec_monitor boost_exception boost_wave boost_filesystem boost_atomic boost_wserialization boost_serialization)
set(CONAN_SYSTEM_LIBS_BOOST_DEBUG dl rt pthread)
set(CONAN_FRAMEWORKS_BOOST_DEBUG )
set(CONAN_FRAMEWORKS_FOUND_BOOST_DEBUG "")  # Will be filled later
set(CONAN_DEFINES_BOOST_DEBUG "-DBOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"-DBOOST_STACKTRACE_USE_ADDR2LINE"
			"-DBOOST_STACKTRACE_USE_BACKTRACE"
			"-DBOOST_STACKTRACE_USE_NOOP")
set(CONAN_BUILD_MODULES_PATHS_BOOST_DEBUG )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_BOOST_DEBUG "BOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"BOOST_STACKTRACE_USE_ADDR2LINE"
			"BOOST_STACKTRACE_USE_BACKTRACE"
			"BOOST_STACKTRACE_USE_NOOP")

set(CONAN_C_FLAGS_BOOST_DEBUG "")
set(CONAN_CXX_FLAGS_BOOST_DEBUG "")
set(CONAN_SHARED_LINKER_FLAGS_BOOST_DEBUG "")
set(CONAN_EXE_LINKER_FLAGS_BOOST_DEBUG "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_BOOST_DEBUG_LIST "")
set(CONAN_CXX_FLAGS_BOOST_DEBUG_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_BOOST_DEBUG_LIST "")
set(CONAN_EXE_LINKER_FLAGS_BOOST_DEBUG_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_BOOST_DEBUG "${CONAN_FRAMEWORKS_BOOST_DEBUG}" "_BOOST" "_DEBUG")
# Append to aggregated values variable
set(CONAN_LIBS_BOOST_DEBUG ${CONAN_PKG_LIBS_BOOST_DEBUG} ${CONAN_SYSTEM_LIBS_BOOST_DEBUG} ${CONAN_FRAMEWORKS_FOUND_BOOST_DEBUG})


#################
###  ZLIB
#################
set(CONAN_ZLIB_ROOT_DEBUG "/home/aleksey/.conan/data/zlib/1.2.13/_/_/package/be27726f9885116da1158027505be62e913cd585")
set(CONAN_INCLUDE_DIRS_ZLIB_DEBUG "/home/aleksey/.conan/data/zlib/1.2.13/_/_/package/be27726f9885116da1158027505be62e913cd585/include")
set(CONAN_LIB_DIRS_ZLIB_DEBUG "/home/aleksey/.conan/data/zlib/1.2.13/_/_/package/be27726f9885116da1158027505be62e913cd585/lib")
set(CONAN_BIN_DIRS_ZLIB_DEBUG )
set(CONAN_RES_DIRS_ZLIB_DEBUG )
set(CONAN_SRC_DIRS_ZLIB_DEBUG )
set(CONAN_BUILD_DIRS_ZLIB_DEBUG "/home/aleksey/.conan/data/zlib/1.2.13/_/_/package/be27726f9885116da1158027505be62e913cd585/")
set(CONAN_FRAMEWORK_DIRS_ZLIB_DEBUG )
set(CONAN_LIBS_ZLIB_DEBUG z)
set(CONAN_PKG_LIBS_ZLIB_DEBUG z)
set(CONAN_SYSTEM_LIBS_ZLIB_DEBUG )
set(CONAN_FRAMEWORKS_ZLIB_DEBUG )
set(CONAN_FRAMEWORKS_FOUND_ZLIB_DEBUG "")  # Will be filled later
set(CONAN_DEFINES_ZLIB_DEBUG )
set(CONAN_BUILD_MODULES_PATHS_ZLIB_DEBUG )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_ZLIB_DEBUG )

set(CONAN_C_FLAGS_ZLIB_DEBUG "")
set(CONAN_CXX_FLAGS_ZLIB_DEBUG "")
set(CONAN_SHARED_LINKER_FLAGS_ZLIB_DEBUG "")
set(CONAN_EXE_LINKER_FLAGS_ZLIB_DEBUG "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_ZLIB_DEBUG_LIST "")
set(CONAN_CXX_FLAGS_ZLIB_DEBUG_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_ZLIB_DEBUG_LIST "")
set(CONAN_EXE_LINKER_FLAGS_ZLIB_DEBUG_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_ZLIB_DEBUG "${CONAN_FRAMEWORKS_ZLIB_DEBUG}" "_ZLIB" "_DEBUG")
# Append to aggregated values variable
set(CONAN_LIBS_ZLIB_DEBUG ${CONAN_PKG_LIBS_ZLIB_DEBUG} ${CONAN_SYSTEM_LIBS_ZLIB_DEBUG} ${CONAN_FRAMEWORKS_FOUND_ZLIB_DEBUG})


#################
###  BZIP2
#################
set(CONAN_BZIP2_ROOT_DEBUG "/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515")
set(CONAN_INCLUDE_DIRS_BZIP2_DEBUG "/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515/include")
set(CONAN_LIB_DIRS_BZIP2_DEBUG "/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515/lib")
set(CONAN_BIN_DIRS_BZIP2_DEBUG "/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515/bin")
set(CONAN_RES_DIRS_BZIP2_DEBUG )
set(CONAN_SRC_DIRS_BZIP2_DEBUG )
set(CONAN_BUILD_DIRS_BZIP2_DEBUG "/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515/")
set(CONAN_FRAMEWORK_DIRS_BZIP2_DEBUG )
set(CONAN_LIBS_BZIP2_DEBUG bz2)
set(CONAN_PKG_LIBS_BZIP2_DEBUG bz2)
set(CONAN_SYSTEM_LIBS_BZIP2_DEBUG )
set(CONAN_FRAMEWORKS_BZIP2_DEBUG )
set(CONAN_FRAMEWORKS_FOUND_BZIP2_DEBUG "")  # Will be filled later
set(CONAN_DEFINES_BZIP2_DEBUG )
set(CONAN_BUILD_MODULES_PATHS_BZIP2_DEBUG )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_BZIP2_DEBUG )

set(CONAN_C_FLAGS_BZIP2_DEBUG "")
set(CONAN_CXX_FLAGS_BZIP2_DEBUG "")
set(CONAN_SHARED_LINKER_FLAGS_BZIP2_DEBUG "")
set(CONAN_EXE_LINKER_FLAGS_BZIP2_DEBUG "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_BZIP2_DEBUG_LIST "")
set(CONAN_CXX_FLAGS_BZIP2_DEBUG_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_BZIP2_DEBUG_LIST "")
set(CONAN_EXE_LINKER_FLAGS_BZIP2_DEBUG_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_BZIP2_DEBUG "${CONAN_FRAMEWORKS_BZIP2_DEBUG}" "_BZIP2" "_DEBUG")
# Append to aggregated values variable
set(CONAN_LIBS_BZIP2_DEBUG ${CONAN_PKG_LIBS_BZIP2_DEBUG} ${CONAN_SYSTEM_LIBS_BZIP2_DEBUG} ${CONAN_FRAMEWORKS_FOUND_BZIP2_DEBUG})


#################
###  LIBBACKTRACE
#################
set(CONAN_LIBBACKTRACE_ROOT_DEBUG "/home/aleksey/.conan/data/libbacktrace/cci.20210118/_/_/package/be27726f9885116da1158027505be62e913cd585")
set(CONAN_INCLUDE_DIRS_LIBBACKTRACE_DEBUG "/home/aleksey/.conan/data/libbacktrace/cci.20210118/_/_/package/be27726f9885116da1158027505be62e913cd585/include")
set(CONAN_LIB_DIRS_LIBBACKTRACE_DEBUG "/home/aleksey/.conan/data/libbacktrace/cci.20210118/_/_/package/be27726f9885116da1158027505be62e913cd585/lib")
set(CONAN_BIN_DIRS_LIBBACKTRACE_DEBUG )
set(CONAN_RES_DIRS_LIBBACKTRACE_DEBUG )
set(CONAN_SRC_DIRS_LIBBACKTRACE_DEBUG )
set(CONAN_BUILD_DIRS_LIBBACKTRACE_DEBUG "/home/aleksey/.conan/data/libbacktrace/cci.20210118/_/_/package/be27726f9885116da1158027505be62e913cd585/")
set(CONAN_FRAMEWORK_DIRS_LIBBACKTRACE_DEBUG )
set(CONAN_LIBS_LIBBACKTRACE_DEBUG backtrace)
set(CONAN_PKG_LIBS_LIBBACKTRACE_DEBUG backtrace)
set(CONAN_SYSTEM_LIBS_LIBBACKTRACE_DEBUG )
set(CONAN_FRAMEWORKS_LIBBACKTRACE_DEBUG )
set(CONAN_FRAMEWORKS_FOUND_LIBBACKTRACE_DEBUG "")  # Will be filled later
set(CONAN_DEFINES_LIBBACKTRACE_DEBUG )
set(CONAN_BUILD_MODULES_PATHS_LIBBACKTRACE_DEBUG )
# COMPILE_DEFINITIONS are equal to CONAN_DEFINES without -D, for targets
set(CONAN_COMPILE_DEFINITIONS_LIBBACKTRACE_DEBUG )

set(CONAN_C_FLAGS_LIBBACKTRACE_DEBUG "")
set(CONAN_CXX_FLAGS_LIBBACKTRACE_DEBUG "")
set(CONAN_SHARED_LINKER_FLAGS_LIBBACKTRACE_DEBUG "")
set(CONAN_EXE_LINKER_FLAGS_LIBBACKTRACE_DEBUG "")

# For modern cmake targets we use the list variables (separated with ;)
set(CONAN_C_FLAGS_LIBBACKTRACE_DEBUG_LIST "")
set(CONAN_CXX_FLAGS_LIBBACKTRACE_DEBUG_LIST "")
set(CONAN_SHARED_LINKER_FLAGS_LIBBACKTRACE_DEBUG_LIST "")
set(CONAN_EXE_LINKER_FLAGS_LIBBACKTRACE_DEBUG_LIST "")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_LIBBACKTRACE_DEBUG "${CONAN_FRAMEWORKS_LIBBACKTRACE_DEBUG}" "_LIBBACKTRACE" "_DEBUG")
# Append to aggregated values variable
set(CONAN_LIBS_LIBBACKTRACE_DEBUG ${CONAN_PKG_LIBS_LIBBACKTRACE_DEBUG} ${CONAN_SYSTEM_LIBS_LIBBACKTRACE_DEBUG} ${CONAN_FRAMEWORKS_FOUND_LIBBACKTRACE_DEBUG})


### Definition of global aggregated variables ###

set(CONAN_DEPENDENCIES_DEBUG boost zlib bzip2 libbacktrace)

set(CONAN_INCLUDE_DIRS_DEBUG "/home/aleksey/.conan/data/boost/1.78.0/_/_/package/8cc3305c27e5ff838d1c7590662e309638310dfc/include"
			"/home/aleksey/.conan/data/zlib/1.2.13/_/_/package/be27726f9885116da1158027505be62e913cd585/include"
			"/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515/include"
			"/home/aleksey/.conan/data/libbacktrace/cci.20210118/_/_/package/be27726f9885116da1158027505be62e913cd585/include" ${CONAN_INCLUDE_DIRS_DEBUG})
set(CONAN_LIB_DIRS_DEBUG "/home/aleksey/.conan/data/boost/1.78.0/_/_/package/8cc3305c27e5ff838d1c7590662e309638310dfc/lib"
			"/home/aleksey/.conan/data/zlib/1.2.13/_/_/package/be27726f9885116da1158027505be62e913cd585/lib"
			"/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515/lib"
			"/home/aleksey/.conan/data/libbacktrace/cci.20210118/_/_/package/be27726f9885116da1158027505be62e913cd585/lib" ${CONAN_LIB_DIRS_DEBUG})
set(CONAN_BIN_DIRS_DEBUG "/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515/bin" ${CONAN_BIN_DIRS_DEBUG})
set(CONAN_RES_DIRS_DEBUG  ${CONAN_RES_DIRS_DEBUG})
set(CONAN_FRAMEWORK_DIRS_DEBUG  ${CONAN_FRAMEWORK_DIRS_DEBUG})
set(CONAN_LIBS_DEBUG boost_contract boost_coroutine boost_fiber_numa boost_fiber boost_context boost_graph boost_iostreams boost_json boost_locale boost_log_setup boost_log boost_math_c99 boost_math_c99f boost_math_c99l boost_math_tr1 boost_math_tr1f boost_math_tr1l boost_nowide boost_program_options boost_random boost_regex boost_stacktrace_addr2line boost_stacktrace_backtrace boost_stacktrace_basic boost_stacktrace_noop boost_timer boost_type_erasure boost_thread boost_chrono boost_container boost_date_time boost_unit_test_framework boost_prg_exec_monitor boost_test_exec_monitor boost_exception boost_wave boost_filesystem boost_atomic boost_wserialization boost_serialization z bz2 backtrace ${CONAN_LIBS_DEBUG})
set(CONAN_PKG_LIBS_DEBUG boost_contract boost_coroutine boost_fiber_numa boost_fiber boost_context boost_graph boost_iostreams boost_json boost_locale boost_log_setup boost_log boost_math_c99 boost_math_c99f boost_math_c99l boost_math_tr1 boost_math_tr1f boost_math_tr1l boost_nowide boost_program_options boost_random boost_regex boost_stacktrace_addr2line boost_stacktrace_backtrace boost_stacktrace_basic boost_stacktrace_noop boost_timer boost_type_erasure boost_thread boost_chrono boost_container boost_date_time boost_unit_test_framework boost_prg_exec_monitor boost_test_exec_monitor boost_exception boost_wave boost_filesystem boost_atomic boost_wserialization boost_serialization z bz2 backtrace ${CONAN_PKG_LIBS_DEBUG})
set(CONAN_SYSTEM_LIBS_DEBUG dl rt pthread ${CONAN_SYSTEM_LIBS_DEBUG})
set(CONAN_FRAMEWORKS_DEBUG  ${CONAN_FRAMEWORKS_DEBUG})
set(CONAN_FRAMEWORKS_FOUND_DEBUG "")  # Will be filled later
set(CONAN_DEFINES_DEBUG "-DBOOST_STACKTRACE_ADDR2LINE_LOCATION=\"/usr/bin/addr2line\""
			"-DBOOST_STACKTRACE_USE_ADDR2LINE"
			"-DBOOST_STACKTRACE_USE_BACKTRACE"
			"-DBOOST_STACKTRACE_USE_NOOP" ${CONAN_DEFINES_DEBUG})
set(CONAN_BUILD_MODULES_PATHS_DEBUG  ${CONAN_BUILD_MODULES_PATHS_DEBUG})
set(CONAN_CMAKE_MODULE_PATH_DEBUG "/home/aleksey/.conan/data/zlib/1.2.13/_/_/package/be27726f9885116da1158027505be62e913cd585/"
			"/home/aleksey/.conan/data/bzip2/1.0.8/_/_/package/76bd63d0cd275bc555bda09b7f93740254ba3515/"
			"/home/aleksey/.conan/data/libbacktrace/cci.20210118/_/_/package/be27726f9885116da1158027505be62e913cd585/" ${CONAN_CMAKE_MODULE_PATH_DEBUG})

set(CONAN_CXX_FLAGS_DEBUG " ${CONAN_CXX_FLAGS_DEBUG}")
set(CONAN_SHARED_LINKER_FLAGS_DEBUG " ${CONAN_SHARED_LINKER_FLAGS_DEBUG}")
set(CONAN_EXE_LINKER_FLAGS_DEBUG " ${CONAN_EXE_LINKER_FLAGS_DEBUG}")
set(CONAN_C_FLAGS_DEBUG " ${CONAN_C_FLAGS_DEBUG}")

# Apple Frameworks
conan_find_apple_frameworks(CONAN_FRAMEWORKS_FOUND_DEBUG "${CONAN_FRAMEWORKS_DEBUG}" "" "_DEBUG")
# Append to aggregated values variable: Use CONAN_LIBS instead of CONAN_PKG_LIBS to include user appended vars
set(CONAN_LIBS_DEBUG ${CONAN_LIBS_DEBUG} ${CONAN_SYSTEM_LIBS_DEBUG} ${CONAN_FRAMEWORKS_FOUND_DEBUG})
