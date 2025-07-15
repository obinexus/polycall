# OBINexus Polycall Meson Build Configuration
project('polycall', 'c',
    version: '0.7.0',
    license: 'Apache-2.0',
    default_options: [
        'c_std=c11',
        'warning_level=3',
        'werror=true'
    ]
)

# Project configuration
polycall_version = meson.project_version()
polycall_name = meson.project_name()

# Dependencies
thread_dep = dependency('threads')

# Configuration options
enable_testing = get_option('enable_testing')
enable_nlink = get_option('enable_nlink')
security_level = get_option('security_level')

# Include directories
inc_dirs = include_directories('include', 'src')

# Subdirectories
subdir('src')

if enable_testing
    subdir('tests')
endif

# Summary
summary({
    'Version': polycall_version,
    'Testing': enable_testing,
    'NLink': enable_nlink,
    'Security Level': security_level,
}, bool_yn: true)
