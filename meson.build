project('polycall-v2', 'c', 'cpp',
    version : '2.0.0',
    default_options : [
        'warning_level=3',
        'c_std=c11',
        'cpp_std=c++17'
    ]
)

# Configuration
conf_data = configuration_data()
conf_data.set('VERSION', meson.project_version())
conf_data.set('PROJECT_NAME', meson.project_name())

# Include directories
inc_dirs = include_directories('include', 'lib/shared')

# Source files
core_sources = []
cli_sources = []
ffi_sources = []

# Find source files
if fs.is_dir('src/core')
    core_sources = run_command('find', 'src/core', '-name', '*.c', check: false).stdout().strip().split('\n')
endif

if fs.is_dir('src/cli')
    cli_sources = run_command('find', 'src/cli', '-name', '*.c', check: false).stdout().strip().split('\n')
endif

if fs.is_dir('src/ffi')
    ffi_sources = run_command('find', 'src/ffi', '-name', '*.c', check: false).stdout().strip().split('\n')
endif

# Dependencies
thread_dep = dependency('threads')

# Core library
if core_sources.length() > 0
    polycall_core = static_library('polycall-core',
        core_sources,
        include_directories : inc_dirs,
        dependencies : [thread_dep],
        install : true
    )
else
    # Fallback if no core sources found
    polycall_core = static_library('polycall-core',
        files('src/core/polycall_core.c'),
        include_directories : inc_dirs,
        dependencies : [thread_dep],
        install : true
    )
endif

# CLI executable
if cli_sources.length() > 0
    polycall_exe = executable('polycall',
        cli_sources,
        include_directories : inc_dirs,
        link_with : polycall_core,
        install : true
    )
endif

# FFI library
if ffi_sources.length() > 0
    polycall_ffi = shared_library('polycall-ffi',
        ffi_sources,
        include_directories : inc_dirs,
        link_with : polycall_core,
        install : true
    )
endif

# Tests
if get_option('enable_tests')
    test_dir = 'tests'
    if fs.is_dir(test_dir)
        subdir(test_dir)
    endif
endif
