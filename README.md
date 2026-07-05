# Elastic wave FDM 2D

A C++17 finite-difference solver for displacement in a rectangular isotropic
elastic medium. The computational core is UI-independent; an observer callback
can stream states to a renderer or writer without retaining the full history.

## Mathematical model

The solver uses the two-dimensional Navier equations for a homogeneous,
isotropic elastic medium. The unknowns are the horizontal displacement
$u(x,y,t)$ and vertical displacement $v(x,y,t)$:

$$
\rho u_{tt}=(\lambda+2\mu)u_{xx}+\mu u_{yy}+(\lambda+\mu)v_{xy},
$$

$$
\rho v_{tt}=\mu v_{xx}+(\lambda+2\mu)v_{yy}+(\lambda+\mu)u_{xy}.
$$

The Lame parameters are calculated from Young's modulus $E$ and Poisson's
ratio $\nu$ as

$$
\lambda=\frac{\nu E}{(1+\nu)(1-2\nu)}, \qquad
\mu=\frac{E}{2(1+\nu)}.
$$

This form of $\lambda$ corresponds to a plane-strain model.

## Finite-difference scheme

The rectangular domain is sampled on a uniform grid
$x_i=i\,\Delta x$, $y_j=j\,\Delta y$, and $t^n=n\,\Delta t$. Central
differences are used in space:

$$
\delta_{xx}f_{i,j}=\frac{f_{i+1,j}-2f_{i,j}+f_{i-1,j}}{\Delta x^2},
\qquad
\delta_{yy}f_{i,j}=\frac{f_{i,j+1}-2f_{i,j}+f_{i,j-1}}{\Delta y^2},
$$

$$
\delta_{xy}f_{i,j}=
\frac{f_{i+1,j+1}-f_{i+1,j-1}-f_{i-1,j+1}+f_{i-1,j-1}}
     {4\Delta x\Delta y}.
$$

The explicit three-level time update is

$$
u_{i,j}^{n+1}=2u_{i,j}^{n}-u_{i,j}^{n-1}
+\frac{\Delta t^2}{\rho}\left[
(\lambda+2\mu)\delta_{xx}u_{i,j}^{n}
+\mu\delta_{yy}u_{i,j}^{n}
+(\lambda+\mu)\delta_{xy}v_{i,j}^{n}\right],
$$

$$
v_{i,j}^{n+1}=2v_{i,j}^{n}-v_{i,j}^{n-1}
+\frac{\Delta t^2}{\rho}\left[
\mu\delta_{xx}v_{i,j}^{n}
+(\lambda+2\mu)\delta_{yy}v_{i,j}^{n}
+(\lambda+\mu)\delta_{xy}u_{i,j}^{n}\right].
$$

Both displacement components are fixed to zero at every boundary. Initially
all values are zero except for a horizontal displacement at the centre node;
the previous and current layers are made equal to approximate zero initial
velocity.

Only three time layers are stored: previous, current, and next. At each step
the solver clears the next layer, updates all interior nodes, applies the fixed
boundary conditions, rotates the three buffers, and passes the resulting state
to the optional observer. Thus memory consumption is $O(N_xN_y)$ rather than
$O(N_xN_yN_t)$.

Before starting, the solver checks the conservative CFL condition

$$
c_p\Delta t\sqrt{\frac{1}{\Delta x^2}+\frac{1}{\Delta y^2}}\leq1,
\qquad
c_p=\sqrt{\frac{\lambda+2\mu}{\rho}}.
$$

The scheme is second-order accurate in space and, away from the initial-layer
approximation, second-order accurate in time.

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
