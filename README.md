# Elastic wave FDM 2D

A C++17 finite-difference solver for displacement in a rectangular isotropic
elastic medium. The computational core is UI-independent; an observer callback
can stream states to a renderer or writer without retaining the full history.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

For an optimized build with native SIMD instructions:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

Native SIMD is enabled by default and can be disabled with
`-DELASTIC_WAVE_NATIVE_SIMD=OFF` when building binaries for other CPUs.

Run `build/elastic-wave` (or `build/Debug/elastic-wave.exe` with a multi-config
generator). Parameters are collected in `elastic_wave::Configuration`.

On Windows the build produces two applications:

- `elastic-wave.exe` runs the simulation in the console without graphics.
- `elastic-wave-visualizer.exe` opens a window and animates the wave field.

For a MinGW installation in `C:\msys64`, build and run the visualizer from
PowerShell with:

```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cmake -S . -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw
.\build-mingw\elastic-wave-visualizer.exe
```
