set(generated "${CMAKE_CURRENT_BINARY_DIR}/jm.rc"
              "${CMAKE_CURRENT_BINARY_DIR}/jm.pc"
              "${CMAKE_CURRENT_BINARY_DIR}/jm.def"
              "${CMAKE_CURRENT_BINARY_DIR}/jm_config.h")

foreach(file ${generated})
  if(EXISTS ${file})
     file(REMOVE ${file})
  endif()
endforeach(file)
