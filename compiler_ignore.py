# extra_script.py
# Import("env")

# This script runs after PlatformIO has prepared the build environment.
# We can use it to make precise modifications.

# 1. Get the build environment for the esp-dsp library.
#    PlatformIO creates a separate environment for each library.
lib_env = env.GetLibBuildenv("esp-dsp")

# 2. Define the path to the file we want to exclude.
#    The path is relative to the root of the esp-dsp library folder.
bad_file = "modules/support/cplx_gen/dsps_cplx_gen.c"

# 3. Use the Filter out a source file feature
#    This command tells the build environment for this specific library
#    to explicitly ignore the file we specify.
lib_env.SRC_FILTER = f"-<{bad_file}>"

print("-----------------------------------------------------------------------------")
print(f"Custom script: Forcing esp-dsp library to ignore conflicting file: {bad_file}")
print("-----------------------------------------------------------------------------")