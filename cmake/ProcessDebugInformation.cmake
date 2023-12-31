MACRO (PROCESS_DEBUG_INFO TARGET_NAME DESTINATION COMPONENT_NAME)
  IF ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    ADD_CUSTOM_COMMAND (TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --bold --blue "Separating debug information from ${TARGET_NAME}"
      COMMAND ${CMAKE_OBJCOPY} --only-keep-debug "$<TARGET_FILE:${TARGET_NAME}>" "$<TARGET_FILE:${TARGET_NAME}>.dbg"
      COMMAND ${CMAKE_OBJCOPY} --strip-debug "$<TARGET_FILE:${TARGET_NAME}>"
      COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="$<TARGET_FILE:${TARGET_NAME}>.dbg" "$<TARGET_FILE:${TARGET_NAME}>")

    GET_TARGET_PROPERTY (FULL_NAME ${TARGET_NAME} LOCATION)
    GET_FILENAME_COMPONENT (TARGET_FILE "${FULL_NAME}" NAME)

    INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_FILE}.dbg
      DESTINATION    ${DESTINATION}
      CONFIGURATIONS RelWithDebInfo
      COMPONENT      ${COMPONENT_NAME})

    SET (CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} ${COMPONENT_NAME})
  ENDIF ()

  IF ("${CMAKE_BUILD_TYPE}" STREQUAL "Release" OR "${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    GET_TARGET_PROPERTY (TARGET_TYPE ${TARGET_NAME} TYPE)

    IF (${TARGET_TYPE} STREQUAL "EXECUTABLE")
      ADD_CUSTOM_COMMAND (TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --bold --blue "Stripping information from ${TARGET_NAME}"
        COMMAND ${CMAKE_STRIP} --strip-all "$<TARGET_FILE:${TARGET_NAME}>")
    ELSE ()
      ADD_CUSTOM_COMMAND (TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --bold --blue "Stripping information from ${TARGET_NAME}"
        COMMAND ${CMAKE_STRIP} --strip-unneeded "$<TARGET_FILE:${TARGET_NAME}>")
    ENDIF ()
  ENDIF ()
ENDMACRO ()
