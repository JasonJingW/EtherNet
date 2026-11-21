function(read_build_commit repository output_variable)
    set(build_commit "unknown")
    if(GIT_EXECUTABLE AND EXISTS "${repository}/.git")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
            WORKING_DIRECTORY "${repository}"
            OUTPUT_VARIABLE git_commit
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE git_result
            ERROR_QUIET)
        if(git_result STREQUAL "0" AND git_commit MATCHES "^[0-9a-f]+$")
            set(build_commit "${git_commit}")
            # Report submodule worktree edits on their own commit lines.
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" --no-optional-locks status --porcelain
                    --untracked-files=normal --ignore-submodules=dirty
                WORKING_DIRECTORY "${repository}"
                OUTPUT_VARIABLE git_status
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE git_result
                ERROR_QUIET)
            if(git_result STREQUAL "0" AND NOT git_status STREQUAL "")
                string(APPEND build_commit "-dirty")
            endif()
        endif()
    endif()
    set(${output_variable} "${build_commit}" PARENT_SCOPE)
endfunction()

read_build_commit("${PROJECT_ROOT}" project_commit)
read_build_commit("${PROJECT_ROOT}/platform_common" common_commit)
read_build_commit("${PROJECT_ROOT}/platform_application" application_commit)

string(TIMESTAMP build_timestamp "%Y-%m-%d %H:%M:%S")
string(SUBSTRING "${build_timestamp}" 0 10 build_date)
string(SUBSTRING "${build_timestamp}" 11 8 build_time)
file(CONFIGURE OUTPUT "${OUTPUT_FILE}" CONTENT [=[
#define PROJECT_BUILD_DATE "@build_date@"
#define PROJECT_BUILD_TIME "@build_time@"
#define PROJECT_BUILD_COMMIT "@project_commit@"
#define PROJECT_BUILD_COMMON_COMMIT "@common_commit@"
#define PROJECT_BUILD_APPLICATION_COMMIT "@application_commit@"
]=] @ONLY)
