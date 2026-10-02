# Re-create an MSVC static library so its members are named by file name
# only. lib.exe records each object under the path it was given -- for a
# CMake build that is an absolute build-directory path, which would ship
# inside polycall_static.lib. GNU ar already stores base names.
#
#   cmake -DAR=<lib.exe> -DOUT=<library> -DOBJS=<obj;obj;...> -P relative_archive.cmake

if(NOT AR OR NOT OUT OR NOT OBJS)
  message(FATAL_ERROR "relative_archive.cmake needs AR, OUT and OBJS")
endif()

get_filename_component(_out_dir "${OUT}" DIRECTORY)
set(_stage "${_out_dir}/.relative_archive")
file(REMOVE_RECURSE "${_stage}")
file(MAKE_DIRECTORY "${_stage}")

set(_names "")
string(REPLACE "|" ";" _objs "${OBJS}")
foreach(_o IN LISTS _objs)
  get_filename_component(_n "${_o}" NAME)
  if("${_n}" IN_LIST _names)
    message(FATAL_ERROR "two objects share the name ${_n}; cannot flatten the archive")
  endif()
  list(APPEND _names "${_n}")
  file(COPY "${_o}" DESTINATION "${_stage}")
endforeach()

execute_process(
  COMMAND "${AR}" /NOLOGO "/OUT:${OUT}" ${_names}
  WORKING_DIRECTORY "${_stage}"
  RESULT_VARIABLE _rc
  OUTPUT_VARIABLE _log ERROR_VARIABLE _log)
file(REMOVE_RECURSE "${_stage}")
if(NOT _rc EQUAL 0)
  message(FATAL_ERROR "re-archiving ${OUT} failed: ${_log}")
endif()
