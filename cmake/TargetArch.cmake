SET (ARCHDETECT_C_CODE "
#if defined(__arm__) || defined(__TARGET_ARCH_ARM)
    #if defined(__ARM_ARCH_7__) \\
        || defined(__ARM_ARCH_7A__) \\
        || defined(__ARM_ARCH_7R__) \\
        || defined(__ARM_ARCH_7M__) \\
        || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 7)
        #error cmake_ARCH armhf
    #elif defined(__ARM_ARCH_6__) \\
        || defined(__ARM_ARCH_6J__) \\
        || defined(__ARM_ARCH_6T2__) \\
        || defined(__ARM_ARCH_6Z__) \\
        || defined(__ARM_ARCH_6K__) \\
        || defined(__ARM_ARCH_6ZK__) \\
        || defined(__ARM_ARCH_6M__) \\
        || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 6)
        #error cmake_ARCH armv6
    #elif defined(__ARM_ARCH_5TEJ__) \\
        || (defined(__TARGET_ARCH_ARM) && __TARGET_ARCH_ARM-0 >= 5)
        #error cmake_ARCH armv5
    #else
        #error cmake_ARCH arm
    #endif
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
    #error cmake_ARCH i386
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
    #error cmake_ARCH amd64
#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)
    #error cmake_ARCH ia64
#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__) \\
      || defined(_ARCH_COM) || defined(_ARCH_PWR) || defined(_ARCH_PPC)  \\
      || defined(_M_MPPC) || defined(_M_PPC)
    #if defined(__ppc64__) || defined(__powerpc64__) || defined(__64BIT__)
        #error cmake_ARCH ppc64
    #else
        #error cmake_ARCH ppc
    #endif
#endif
#error cmake_ARCH unknown
")

FUNCTION (TARGET_ARCHITECTURE output_var alt_output_var)
  IF (APPLE AND CMAKE_OSX_ARCHITECTURES)
    FOREACH (osx_arch ${CMAKE_OSX_ARCHITECTURES})
      IF ("${osx_arch}" STREQUAL "ppc" AND ppc_support)
        SET (osx_arch_ppc TRUE)
      ELSEIF ("${osx_arch}" STREQUAL "i386")
        SET (osx_arch_i386 TRUE)
      ELSEIF ("${osx_arch}" STREQUAL "x86_64")
        SET (osx_arch_x86_64 TRUE)
      ELSEIF ("${osx_arch}" STREQUAL "ppc64" AND ppc_support)
        SET (osx_arch_ppc64 TRUE)
      ELSE ()
        MESSAGE(FATAL_ERROR "Invalid OS X arch name: ${osx_arch}")
      ENDIF ()
    ENDFOREACH ()

    IF (osx_arch_ppc)
      LIST (APPEND ARCH ppc)
    ENDIF ()

    IF (osx_arch_i386)
      LIST (APPEND ARCH i386)
    ENDIF ()

    IF (osx_arch_x86_64)
      LIST (APPEND ARCH x86_64)
    ENDIF ()

    IF (osx_arch_ppc64)
      LIST (APPEND ARCH ppc64)
    ENDIF ()
  ELSE ()
    FILE (WRITE "${CMAKE_BINARY_DIR}/arch.c" "${ARCHDETECT_C_CODE}")

    ENABLE_LANGUAGE (C)

    # Detect the architecture in a rather creative way...
    # This compiles a small C program which is a series of ifdefs that selects a
    # particular #error preprocessor directive whose message string contains the
    # target architecture. The program will always fail to compile (both because
    # file is not a valid C program, and obviously because of the presence of the
    # #error preprocessor directives... but by exploiting the preprocessor in this
    # way, we can detect the correct target architecture even when cross-compiling,
    # since the program itself never needs to be run (only the compiler/preprocessor)
    TRY_RUN (
      run_result_unused
      compile_result_unused
      "${CMAKE_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}/arch.c"
      COMPILE_OUTPUT_VARIABLE ARCH
      CMAKE_FLAGS CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    )

    # Parse the architecture name from the compiler output
    STRING (REGEX MATCH "cmake_ARCH ([a-zA-Z0-9_]+)" ARCH "${ARCH}")

    # Get rid of the value marker leaving just the architecture name
    STRING (REPLACE "cmake_ARCH " "" ARCH "${ARCH}")
    
    IF (${ARCH} STREQUAL "amd64")
      SET (ALT_ARCH "x86_64")
    ELSE ()
      SET (ALT_ARCH ${ARCH})
    ENDIF ()

    # If we are compiling with an unknown architecture this variable should
    # already be set to "unknown" but in the case that it's empty (i.e. due
    # to a typo in the code), then set it to unknown
    IF (NOT ARCH)
      SET (ARCH unknown)
      SET (ALT_ARCH unknown)
    ENDIF ()
  ENDIF ()

  SET (${output_var} "${ARCH}" PARENT_SCOPE)
  SET (${alt_output_var} "${ALT_ARCH}" PARENT_SCOPE)
ENDFUNCTION ()
