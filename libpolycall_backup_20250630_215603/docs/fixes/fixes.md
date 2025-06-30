rm -rf build/ CMakeFiles/ CMakeCache.txt
cmake -DENABLE_INCLUDE_VALIDATION=ON .
make -j$(nproc)
python3 scripts/include_path_validator.py --project-root . --error-log build_errors.txt
