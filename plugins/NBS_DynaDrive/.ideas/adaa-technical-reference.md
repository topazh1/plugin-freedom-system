# ADAA Technical Reference: NBS DynaDrive

**Purpose:** Deep technical reference for implementing Antiderivative Anti-Aliasing in the DynaDrive saturation engine.
**Date:** 2026-02-27
**Status:** Research document -- guides Phase 4.1 and 4.2 DSP implementation.
**References:** Parker et al. (2016), Bilbao et al. (2017), Chowdhury (2020)

---

## 1. ADAA Theory -- Mathematical Foundation

### The Aliasing Problem

When a memoryless nonlinear function `f(x)` is applied to a discrete-time signal `x[n]`, the output `y[n] = f(x[n])` generates harmonics. If the input signal has energy up to frequency `B`, the output can have energy at multiples `2B, 3B, 4B, ...` without bound. Any harmonic energy above the Nyquist frequency `fs/2` folds back into the audible band as aliasing -- inharmonic spectral content that sounds metallic and harsh.

Traditional mitigation: oversample the signal (process at 2x-16x the sample rate, lowpass filter, decimate). This works but costs CPU, adds latency from the anti-aliasing filter, and introduces phase artifacts from the steep lowpass.

### The Continuous-Time Convolution Insight

ADAA starts from a simple observation in continuous-time signal processing. Consider a continuous-time signal `x(t)` passed through a nonlinear function `f` to produce `y(t) = f(x(t))`. If we want to bandwidth-limit this output, we convolve it with a sinc function (ideal lowpass filter):

```
y_filtered(t) = integral from -inf to +inf of f(x(tau)) * sinc((t - tau) / T) d_tau
```

where `T = 1/fs` is the sample period. This integral is generally intractable. Parker et al.'s insight was to approximate it using a simpler kernel -- a rectangular (boxcar) function of width T, centered at each sample instant:

```
y_approx[n] = (1/T) * integral from x((n-1)T) to x(nT) of f(x(tau)) d_tau
```

This is equivalent to averaging the continuous-time output over one sample period. The rectangular kernel is not ideal (it is a sinc in the frequency domain, not a brick wall), but it provides meaningful aliasing suppression with zero latency and minimal computation.

### Why the Antiderivative Formula Works

If we assume the input `x(t)` is piecewise-linear between samples (i.e., linearly interpolated), then within the interval `[(n-1)T, nT]`, we can write:

```
x(tau) = x[n-1] + (x[n] - x[n-1]) * (tau - (n-1)T) / T
```

Under this linear interpolation, the variable substitution `u = x(tau)` transforms the integral:

```
y[n] = (1 / (x[n] - x[n-1])) * integral from x[n-1] to x[n] of f(u) du
```

By the Fundamental Theorem of Calculus, if `F1(x)` is the antiderivative of `f(x)` (i.e., `F1'(x) = f(x)`), then:

```
integral from x[n-1] to x[n] of f(u) du = F1(x[n]) - F1(x[n-1])
```

Therefore:

```
y[n] = (F1(x[n]) - F1(x[n-1])) / (x[n] - x[n-1])
```

**This is the first-order ADAA formula.** It computes the average value of the waveshaping function between consecutive input samples, weighted by the input difference. The result is a bandwidth-limited approximation of the nonlinear output.

### Frequency-Domain Interpretation

The rectangular averaging acts as a first-order sinc lowpass in the frequency domain. The magnitude response of this implicit filter is:

```
|H(f)| = |sinc(f / fs)| = |sin(pi * f / fs) / (pi * f / fs)|
```

At the Nyquist frequency (f = fs/2), this gives approximately -3.9 dB of attenuation. At 2x Nyquist (the first aliased harmonic), attenuation is approximately -13.5 dB. This is a significant improvement over no anti-aliasing (0 dB aliased components), though not as aggressive as multi-order oversampling.

### The Half-Sample Delay

First-order ADAA introduces a delay of exactly 0.5 samples. This is because the rectangular integration window spans from sample `n-1` to sample `n`, centering the output at `n - 0.5`. In practice, for a zero-latency plugin, this half-sample delay is negligible (10.4 microseconds at 48kHz) and does not require latency compensation.

---

## 2. First-Order vs. Second-Order ADAA

### Second-Order ADAA Formula

Second-order ADAA applies the same averaging principle twice, using the second antiderivative `F2(x)` where `F2'(x) = F1(x)` and `F2''(x) = f(x)`:

```
D2(x[n], x[n-1]) = (F2(x[n]) - F2(x[n-1])) / (x[n] - x[n-1])
```

Then:

```
y[n] = (2 / (x[n] - x[n-2])) * (D2(x[n], x[n-1]) - D2(x[n-1], x[n-2]))
```

Note that second-order ADAA uses a three-point formula involving `x[n]`, `x[n-1]`, and `x[n-2]`.

### Aliasing Suppression Comparison

| Method                   | Aliasing suppression at Nyquist | Equivalent oversampling |
|--------------------------|---------------------------------|-------------------------|
| No anti-aliasing         | 0 dB                            | 1x                      |
| First-order ADAA         | ~13.5 dB at 2x Nyquist         | ~2x oversampling        |
| Second-order ADAA        | ~27 dB at 2x Nyquist           | ~4x-6x oversampling     |
| First-order ADAA + 2x OS | ~27 dB                          | ~4x oversampling        |
| 4x oversampling alone    | ~24 dB                          | 4x oversampling         |

The second-order method squares the sinc envelope, giving `sinc^2` rolloff -- substantially better suppression of aliased components, especially at high frequencies.

### CPU Cost Comparison

| Method          | Operations per sample                    | Relative cost |
|-----------------|------------------------------------------|---------------|
| No AA           | 1x `f(x)` evaluation                    | 1.0x          |
| First-order     | 1x `F1(x)` evaluation + 1 division      | 1.2-1.5x      |
| Second-order    | 1x `F2(x)` evaluation + 2 divisions     | 1.5-2.5x      |
| 2x oversampling | 2x `f(x)` + lowpass filter (FIR/IIR)    | 3-5x          |
| 4x oversampling | 4x `f(x)` + lowpass filter              | 6-10x         |

First-order ADAA is remarkably efficient -- it costs barely more than direct evaluation. The `F1` evaluation for tanh (which is `log(cosh(x))`) is slightly more expensive than `tanh(x)` itself, but the overall cost is far less than any oversampling approach.

### When First-Order Is Sufficient

First-order ADAA is sufficient when:
- The waveshaper is not extremely aggressive (drive levels producing up to ~10-12th harmonics)
- The sample rate is 44.1kHz or higher
- The application is saturation/warmth rather than hard distortion
- CPU budget is tight (mastering plugin running on many tracks)

For DynaDrive specifically, first-order ADAA is the correct choice because:
1. The tanh waveshaper is inherently bandlimited (its harmonics decay rapidly)
2. The cubic polynomial `1.5x - 0.5x^3` generates only up to 3rd harmonics from the polynomial itself
3. Mastering-grade use cases involve moderate drive (the sweet spot is subtle saturation)
4. Zero-latency requirement precludes the 1-sample delay of second-order ADAA

### When Second-Order Would Be Necessary

Second-order ADAA becomes beneficial when:
- Implementing hard clipping (generates theoretically infinite harmonics)
- Drive levels are extreme (guitar amp distortion, fuzz effects)
- Operating at low sample rates (e.g., 22.05kHz legacy)
- The application is a dedicated distortion effect where quality at extreme settings matters more than CPU

The second-order formula for tanh requires the dilogarithm function `Li2(-e^(-2x))`, which is expensive to compute and lacks a simple SIMD-friendly implementation. This further supports using first-order for DynaDrive.

---

## 3. Waveshaper Function Design for ADAA Compatibility

### What Makes a Function "ADAA-Friendly"

A waveshaping function `f(x)` is ADAA-compatible if and only if:

1. **Closed-form antiderivative exists.** `F1(x) = integral of f(x) dx` must be expressible in terms of elementary or efficiently computable functions. If only numerical integration is available, ADAA loses its performance advantage.

2. **The antiderivative is numerically stable.** Even if a closed form exists, it must be computable without catastrophic cancellation, overflow, or precision loss across the full input range.

3. **The function is continuous.** Discontinuities in `f(x)` create impulses in the ADAA output at transition points, which can be worse than aliasing. True hard clipping `f(x) = clamp(x, -1, 1)` is continuous but not differentiable at the clip points -- this is acceptable for ADAA but requires piecewise antiderivative computation.

4. **The antiderivative is smooth.** `F1(x)` should be C1-continuous (continuously differentiable) for first-order ADAA to work correctly. Since `F1'(x) = f(x)`, this means `f(x)` itself must be continuous (satisfied by requirement 3).

### The tanh Waveshaper

**Function:** `f(x) = tanh(x)`

**First antiderivative:** `F1(x) = ln(cosh(x))`

**Verification:** `d/dx [ln(cosh(x))] = sinh(x)/cosh(x) = tanh(x)` -- correct.

**Numerical stability of `ln(cosh(x))`:**

Direct computation of `std::log(std::cosh(x))` overflows for `|x| > ~89` (float) or `|x| > ~710` (double) because `cosh(x)` grows as `e^|x|/2`.

The stable reformulation uses the identity:

```
ln(cosh(x)) = |x| + ln(1 + e^(-2|x|)) - ln(2)
```

**Derivation:**
```
cosh(x) = (e^x + e^(-x)) / 2
         = e^|x| * (1 + e^(-2|x|)) / 2

ln(cosh(x)) = |x| + ln(1 + e^(-2|x|)) - ln(2)
```

For `|x| > 10`, the term `e^(-2|x|)` is negligibly small (< 2e-9), so `ln(cosh(x)) ~= |x| - ln(2)`. Use `std::log1p(std::exp(-2.0 * std::abs(x)))` for the middle term to preserve precision when the argument is near zero.

**Implementation (C++):**

```cpp
inline double logcosh(double x)
{
    const double ax = std::abs(x);
    if (ax > 10.0)
        return ax - 0.6931471805599453; // ln(2)
    return std::log(std::cosh(x)); // safe for |x| <= 10
}
```

Or the fully stable version that is accurate everywhere:

```cpp
inline double logcosh_stable(double x)
{
    const double ax = std::abs(x);
    return ax + std::log1p(std::exp(-2.0 * ax)) - 0.6931471805599453;
}
```

### The Cubic Polynomial Waveshaper

**Function:** `f_hard(x) = 1.5x - 0.5x^3` for `|x| <= 1`, `sign(x)` for `|x| > 1`

This is a smooth soft-clipper that reaches +/-1 at `x = +/-1` and stays there. The derivative at the clip point is zero (smooth transition).

**First antiderivative:**

For `|x| <= 1`:
```
F1_hard(x) = 0.75*x^2 - 0.125*x^4
```

For `|x| > 1`:
```
F1_hard(x) = sign(x) * |x| + C
```

where `C` is chosen for continuity at `x = +/-1`:
```
F1_hard(1) = 0.75 - 0.125 = 0.625
F1_hard(1+) = 1 + C = 0.625  =>  C = -0.375
```

So: `F1_hard(x) = sign(x) * (|x| - 0.375)` for `|x| > 1`.

**Verification:** `d/dx [0.75x^2 - 0.125x^4] = 1.5x - 0.5x^3` -- correct.

**Why this polynomial?** The function `f(x) = 1.5x - 0.5x^3` is the unique cubic of the form `ax - bx^3` that satisfies:
- `f(1) = 1` (clips to unity)
- `f'(1) = 0` (zero derivative at clip point -- smooth knee)

These constraints give `a = 3/2, b = 1/2`.

### The tanh with Drive Gain

For unity-gain normalization at various drive levels:

**Function:** `f_soft(x) = tanh(k * x) / tanh(k)` where `k` is the drive gain.

**First antiderivative:**
```
F1_soft(x) = ln(cosh(k * x)) / (k * tanh(k))
```

**Verification:**
```
d/dx [ln(cosh(k*x)) / (k * tanh(k))]
= k * tanh(k*x) / (k * tanh(k))
= tanh(k*x) / tanh(k)
= f_soft(x)  -- correct.
```

Note the normalization factor `1 / (k * tanh(k))` in the antiderivative. For large `k`, `tanh(k) -> 1`, so this simplifies to `ln(cosh(k*x)) / k`.

---

## 4. The h_curve Morphing Problem

### The Question

If `h_curve` morphs between `f_soft(x) = tanh(k*x)/tanh(k)` and `f_hard(x) = 1.5x - 0.5x^3` via:

```
f_blend(x) = (1 - alpha) * f_soft(x) + alpha * f_hard(x)
```

Is it true that `F1_blend(x) = (1 - alpha) * F1_soft(x) + alpha * F1_hard(x)`?

### Proof: Yes, Antiderivative of a Linear Blend Equals the Blend of Antiderivatives

**Theorem:** If `F1_a(x)` is an antiderivative of `f_a(x)` and `F1_b(x)` is an antiderivative of `f_b(x)`, and `alpha` is a constant, then `(1-alpha)*F1_a(x) + alpha*F1_b(x)` is an antiderivative of `(1-alpha)*f_a(x) + alpha*f_b(x)`.

**Proof:**
```
d/dx [(1-alpha)*F1_a(x) + alpha*F1_b(x)]
= (1-alpha)*F1_a'(x) + alpha*F1_b'(x)       [linearity of differentiation]
= (1-alpha)*f_a(x) + alpha*f_b(x)            [definition of antiderivative]
= f_blend(x)                                  QED
```

This works because differentiation (and hence integration) is a linear operator. The key requirement is that **alpha is constant during the sample-to-sample ADAA computation**. Since `h_curve` is a user parameter that changes at audio rate (but is constant within a single ADAA step), this condition is satisfied.

### The Correct Implementation

```cpp
// Per-sample ADAA with morphing
float alpha = hCurve / 100.0f;

// Compute blended antiderivatives
double F1_now  = (1.0 - alpha) * F1_soft(x_now)  + alpha * F1_hard(x_now);
double F1_prev = (1.0 - alpha) * F1_soft(x_prev) + alpha * F1_hard(x_prev);

// Standard ADAA formula
double delta = x_now - x_prev;
double y;
if (std::abs(delta) > EPSILON)
    y = (F1_now - F1_prev) / delta;
else
    y = (1.0 - alpha) * f_soft(x_now) + alpha * f_hard(x_now);
```

### Warning: Alpha Must Not Change Between x[n-1] and x[n]

If `alpha` changes between samples (e.g., due to parameter smoothing updating between each sample pair), the blended antiderivatives `F1_blend(x[n])` and `F1_blend(x[n-1])` would have been computed with different alpha values, breaking the ADAA formula.

**Solution:** Hold `alpha` constant for each ADAA step. Read the smoothed parameter value once at the start of each sample computation, not independently for `F1(x[n])` and `F1(x[n-1])`.

In practice, since `F1(x[n-1])` was computed on the previous sample with the previous alpha, you have two options:

1. **Recompute F1(x[n-1]) with current alpha.** This means storing `x_prev` (not `F1_prev`) and recomputing `F1_blend(x_prev)` with the current alpha each sample. This is the correct approach.

2. **Store F1_prev directly.** This is incorrect if alpha changes between samples -- the stored `F1_prev` was computed with a different alpha, creating a discontinuity.

**Recommendation for DynaDrive: Store `x_prev`, not `F1_prev`.** Recompute both `F1_blend(x_now)` and `F1_blend(x_prev)` with the current alpha on every sample. This costs one extra `F1_soft` + `F1_hard` evaluation per sample but ensures correctness during parameter changes.

---

## 5. Near-Singularity Handling

### The Problem

The ADAA formula:

```
y[n] = (F1(x[n]) - F1(x[n-1])) / (x[n] - x[n-1])
```

When `x[n] ~= x[n-1]`, both numerator and denominator approach zero, creating a 0/0 indeterminate form. By L'Hopital's rule:

```
lim_{x[n] -> x[n-1]} (F1(x[n]) - F1(x[n-1])) / (x[n] - x[n-1])
= F1'(x[n-1])
= f(x[n-1])
```

So the limit is simply the direct application of the waveshaping function. The question is: at what threshold do we switch to this fallback, and how do we do it?

### Epsilon Threshold Analysis

The numerical error in the ADAA division comes from catastrophic cancellation in the numerator. When `F1(x[n])` and `F1(x[n-1])` are nearly equal, their difference loses significant digits.

For IEEE 754 single precision (float, 24-bit mantissa, ~7.2 decimal digits):
- If `|F1(x[n]) - F1(x[n-1])| < epsilon_F1 * |F1(x[n])|` where `epsilon_F1 ~= 1e-7`, the difference has zero significant bits.
- Since `F1(x[n]) - F1(x[n-1]) ~= f(x[n]) * (x[n] - x[n-1])` by Taylor expansion, catastrophic cancellation occurs when `|x[n] - x[n-1]| < epsilon * |x[n]|` approximately.

For IEEE 754 double precision (double, 53-bit mantissa, ~15.9 decimal digits):
- The cancellation threshold is much smaller: `|x[n] - x[n-1]| ~< 1e-15 * |x[n]|`.

**Practical thresholds from the literature and implementations:**

| Source                  | Precision | Epsilon threshold |
|-------------------------|-----------|-------------------|
| Faust AANL library      | float     | 1e-3 (0.001)      |
| Chowdhury (CCRMA)       | float     | 1e-5              |
| JUCE forum consensus    | float     | 1e-3 to 1e-5      |
| Architecture.md spec    | float     | 1e-5              |
| Robbert vdh (Rust)      | float     | exact 0 (too tight)|

**Recommendation for DynaDrive:**

Use **double precision** for all ADAA math internally (the `logcosh` and division), with an epsilon of **1e-5**. This provides:
- Adequate precision headroom (double gives ~10 extra digits beyond the threshold)
- Safety margin for accumulated errors from the `log`/`cosh` computation chain
- No audible artifacts from switching (the fallback matches the limit perfectly)

The 1e-5 threshold means the fallback triggers when consecutive samples differ by less than ~0.001% of a normalized signal. This corresponds to signal changes smaller than -100 dBFS at the sample level -- effectively DC or silence.

### Implementation Strategies

**Strategy 1: Tolerance-Based Branching (Recommended for DynaDrive)**

```cpp
double processSampleADAA(double x_now, double x_prev, double alpha)
{
    double F1_now  = blendedF1(x_now, alpha);
    double F1_prev = blendedF1(x_prev, alpha);
    double delta   = x_now - x_prev;

    if (std::abs(delta) < 1e-5)
        return blendedF(x_now, alpha);  // L'Hopital fallback

    return (F1_now - F1_prev) / delta;
}
```

Pros: Simple, predictable, no extra computation in the common case.
Cons: Discontinuity at the switching threshold (but at 1e-5, the jump is ~1e-10, far below audibility).

**Strategy 2: Taylor Series Approximation**

Near the singularity, expand `F1(x[n])` around `x[n-1]`:

```
F1(x[n]) = F1(x[n-1]) + f(x[n-1]) * delta + (1/2) * f'(x[n-1]) * delta^2 + ...
```

Therefore:

```
(F1(x[n]) - F1(x[n-1])) / delta = f(x[n-1]) + (1/2) * f'(x[n-1]) * delta + ...
```

The zeroth-order Taylor approximation is just `f(x[n-1])` (same as L'Hopital). The first-order correction adds `(1/2) * f'(x[n-1]) * delta`. For tanh:

```
f'(x) = 1 - tanh^2(x) = sech^2(x)
```

So the first-order Taylor fallback is:

```
y ~= tanh(x_prev) + 0.5 * sech^2(x_prev) * delta
```

This provides a smoother transition across the epsilon boundary, but requires computing `f'(x)` -- an extra transcendental evaluation per sample in the fallback region.

**Recommendation:** Use Strategy 1 (simple branching) for DynaDrive. The transition artifact from branching at epsilon=1e-5 is on the order of 1e-10, which is approximately -200 dBFS -- far below any noise floor. The added complexity of Taylor series is not justified.

**Strategy 3: Blend between ADAA and direct (used by some implementations)**

```cpp
double blend = std::min(1.0, std::abs(delta) / epsilon);
double y_adaa = (F1_now - F1_prev) / delta;
double y_direct = blendedF(x_now, alpha);
return blend * y_adaa + (1.0 - blend) * y_direct;
```

This creates a smooth crossfade, but the `y_adaa` value at small `delta` is noisy/imprecise, so blending it in does not help. Not recommended.

---

## 6. Even/Odd Harmonic Generation with ADAA

### Even Harmonics via DC Bias

**Principle:** A symmetric waveshaping function `f(x) = f(-x)` (like `tanh`) produces only odd harmonics from a sinusoidal input. To generate even harmonics, we break the symmetry by adding a DC offset before the waveshaper.

**Mathematical basis:** Consider input `x(t) = A*sin(wt) + B` (sinusoid with DC bias `B`). The waveshaper expands as a Taylor series:

```
f(x) = f(B + A*sin(wt))
     = f(B) + f'(B)*A*sin(wt) + (1/2)*f''(B)*A^2*sin^2(wt) + ...
```

The `sin^2(wt)` term equals `(1 - cos(2wt))/2`, producing 2nd harmonic content. The `sin^3(wt)` term produces 3rd harmonic, etc. Crucially, the even-order Taylor coefficients `f''(B), f''''(B)` generate even harmonics only if `B != 0` (for a symmetric function, these terms vanish at `B = 0`).

**ADAA compatibility:** The DC bias is applied *before* the ADAA waveshaper. The ADAA formula does not care about the absolute value of the input -- it only requires `F1(x)` to be computable. Since `F1(x) = ln(cosh(k*x))` is defined for all real x, the biased input works perfectly:

```
x_biased[n] = x[n] + bias
y[n] = (F1(x_biased[n]) - F1(x_biased[n-1])) / (x_biased[n] - x_biased[n-1])
```

Note that `x_biased[n] - x_biased[n-1] = x[n] - x[n-1]` (the bias cancels in the denominator). So the ADAA division uses the same `delta` as without bias. The state variable `x_prev` must store the biased value:

```cpp
// Per-sample even harmonic processing
double x_biased = x + bias;       // bias = (even / 100.0) * 0.15
double x_odd    = x_biased;       // (odd pre-distortion applied before bias, see below)
double F1_now   = blendedF1(x_odd, alpha);
double F1_prev  = blendedF1(xPrev, alpha);  // xPrev stores previous biased+odd value
double delta    = x_odd - xPrev;
// ... standard ADAA division ...
xPrev = x_odd;
```

**DC removal:** The bias creates a DC component in the output. The architecture specifies a 5Hz highpass filter after the waveshaper to remove this. This is essential -- without it, the DC offset would accumulate through downstream processing and damage speakers/headphones.

### Odd Harmonics via Cubic Pre-Distortion

**Principle:** Adding `k * x^3` to the input before the waveshaper increases odd harmonic content. This works because `x^3` is an odd function, and when passed through a symmetric waveshaper, it produces only odd harmonics (3rd, 5th, 7th...).

**Mathematical basis:**

```
x_odd = x + k * x^3
```

The cubic term `k * x^3` can be viewed as a first-order Chebyshev-like distortion. When this modified signal is passed through `tanh`, the result is:

```
tanh(x + k*x^3) ≈ tanh(x) + k*x^3 * sech^2(x) + ...
```

The `x^3 * sech^2(x)` term contributes energy at the 3rd harmonic and higher odd harmonics.

**ADAA compatibility:** The cubic pre-distortion is applied *before* the ADAA waveshaper, as a transformation of the input signal. The ADAA engine sees the modified signal `x_odd` and applies the standard formula. The antiderivative `F1` is of the waveshaping function `f`, not of `f` composed with the cubic -- this is important.

The ADAA formula becomes:

```
y[n] = (F1(x_odd[n]) - F1(x_odd[n-1])) / (x_odd[n] - x_odd[n-1])
```

Note that `delta` is now `x_odd[n] - x_odd[n-1]`, NOT `x[n] - x[n-1]`. The pre-distortion changes the effective input signal that ADAA operates on. This is correct -- ADAA anti-aliases whatever function operates on whatever input it receives.

**Does pre-distortion preserve anti-aliasing?**

Yes, with a caveat. The ADAA anti-aliases the waveshaper `f(x)`. The cubic pre-distortion itself (`x -> x + k*x^3`) is a nonlinear operation that can generate aliasing on its own. However, with `k = 0.05` (the maximum in the architecture spec), the cubic term is very small, and its aliasing contribution is negligible. The dominant aliasing source is the waveshaper `f`, which is fully anti-aliased by ADAA.

For extreme values of `k`, the pre-distortion could itself become a significant aliasing source. At `k = 0.05`, a full-scale sinusoid at 10kHz would produce a 3rd harmonic at 30kHz with amplitude `0.05 * 1^3 = 0.05` (-26 dB). At 48kHz sample rate, this 30kHz component aliases to 18kHz, but at -26 dB it is well below audibility in the context of an already-saturated signal. For DynaDrive's mastering use case, this is acceptable.

### Processing Order

The complete pre-processing chain before ADAA:

```
1. x_raw = input sample
2. x_odd = x_raw + (odd/100.0) * 0.05 * x_raw^3     // odd harmonic pre-distortion
3. x_full = x_odd + (even/100.0) * 0.15              // even harmonic DC bias
4. x_driven = x_full * driveGain                      // drive scaling
5. y = ADAA(x_driven, x_driven_prev)                  // anti-aliased waveshaping
6. y_compensated = y / (tanh(driveGain) / driveGain)  // output compensation
7. y_dc_blocked = HPF_5Hz(y_compensated)              // DC removal
```

The order matters: odd pre-distortion first (operates on the raw signal shape), then bias (shifts the operating point), then drive (scales into the nonlinear region).

---

## 7. Practical Implementation in C++/JUCE

### Complete ADAA Processing Class

```cpp
class ADAASaturator
{
public:
    void prepare(double sampleRate, int maxBlockSize)
    {
        this->sampleRate = sampleRate;

        // Reset state
        xPrev[0] = 0.0;
        xPrev[1] = 0.0;

        // Configure DC-blocking HPF at 5Hz
        auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate, 5.0f);
        for (int ch = 0; ch < 2; ++ch)
        {
            dcBlocker[ch].coefficients = hpfCoeffs;
            dcBlocker[ch].reset();
        }
    }

    void processSample(float& sample, int channel,
                       float driveGain, float alpha, float bias, float oddGain)
    {
        // Pre-processing: odd harmonics, then bias, then drive
        double x = static_cast<double>(sample);
        double x_odd  = x + oddGain * x * x * x;     // cubic pre-distortion
        double x_full = x_odd + bias;                  // DC bias for even harmonics
        double x_driven = x_full * driveGain;          // drive into nonlinear region

        // ADAA computation (double precision)
        double F1_now  = blendedF1(x_driven, alpha);
        double F1_prev = blendedF1(xPrev[channel], alpha);
        double delta   = x_driven - xPrev[channel];

        double y;
        if (std::abs(delta) < 1e-5)
        {
            // Near-singularity: use direct waveshaper evaluation (L'Hopital limit)
            y = blendedF(x_driven, alpha);
        }
        else
        {
            y = (F1_now - F1_prev) / delta;
        }

        // Store state (the driven, pre-processed value)
        xPrev[channel] = x_driven;

        // Output gain compensation
        double normFactor = std::tanh(driveGain) / driveGain;
        y = y / normFactor;

        // Convert back to float for DC blocker
        sample = static_cast<float>(y);

        // DC blocking (removes bias-induced DC)
        sample = dcBlocker[channel].processSample(sample);
    }

private:
    // --- Waveshaper functions ---

    // Soft: tanh(k*x) -- normalized by tanh(k) happens at output
    static double f_soft(double x)
    {
        return std::tanh(x);
    }

    // Hard: cubic soft clipper
    static double f_hard(double x)
    {
        if (x > 1.0)  return 1.0;
        if (x < -1.0) return -1.0;
        return 1.5 * x - 0.5 * x * x * x;
    }

    // Blended waveshaper
    static double blendedF(double x, double alpha)
    {
        return (1.0 - alpha) * f_soft(x) + alpha * f_hard(x);
    }

    // --- Antiderivatives ---

    // F1 of tanh(x) = ln(cosh(x)), numerically stable
    static double F1_soft(double x)
    {
        const double ax = std::abs(x);
        if (ax > 10.0)
            return ax - 0.6931471805599453;  // |x| - ln(2)
        return ax + std::log1p(std::exp(-2.0 * ax)) - 0.6931471805599453;
    }

    // F1 of cubic soft clipper (piecewise)
    static double F1_hard(double x)
    {
        if (x > 1.0)
            return x - 0.375;          // x - 3/8
        if (x < -1.0)
            return -x - 0.375;         // -x - 3/8 = |x| - 3/8
        return 0.75 * x * x - 0.125 * x * x * x * x;
    }

    // Blended antiderivative (linear blend is valid -- see Section 4)
    static double blendedF1(double x, double alpha)
    {
        return (1.0 - alpha) * F1_soft(x) + alpha * F1_hard(x);
    }

    // --- State ---
    double xPrev[2] = {0.0, 0.0};
    double sampleRate = 48000.0;
    juce::dsp::IIR::Filter<float> dcBlocker[2];
};
```

### processBlock Structure

```cpp
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Read parameters (atomic, lock-free)
    const float drive   = driveParam->load();
    const float even    = evenParam->load();
    const float odd     = oddParam->load();
    const float hCurve  = hCurveParam->load();

    // Map parameter ranges
    const float driveGain = 1.0f + (drive / 100.0f) * 15.0f;  // 1x to 16x
    const float alpha     = hCurve / 100.0f;                    // 0.0 to 1.0
    const float bias      = (even / 100.0f) * 0.15f;           // 0.0 to 0.15
    const float oddGain   = (odd / 100.0f) * 0.05f;            // 0.0 to 0.05

    // Process sample-by-sample (ADAA is inherently sample-by-sample)
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);

        for (int n = 0; n < numSamples; ++n)
        {
            saturator.processSample(channelData[n], ch,
                                    driveGain, alpha, bias, oddGain);
        }
    }
}
```

### Handling the First Sample

When the plugin starts or after `prepareToPlay` is called, `xPrev` is initialized to `0.0`. On the very first sample:

- If the first input sample is `x[0] = 0.0`: `delta = 0.0 - 0.0 = 0.0`, the near-singularity fallback triggers, returning `f(0.0) = tanh(0) = 0.0`. Correct.
- If the first input sample is non-zero (e.g., audio starts abruptly): `delta = x[0] - 0.0 = x[0]`, and ADAA computes `(F1(x[0]) - F1(0)) / x[0]`. Since `F1_soft(0) = ln(cosh(0)) = ln(1) = 0` and `F1_hard(0) = 0`, this simplifies to `F1(x[0]) / x[0]`, which is the average slope of `F1` from 0 to `x[0]` -- a reasonable startup value.

No special handling is needed beyond initializing `xPrev` to zero.

### State Reset

```cpp
void prepareToPlay(double sampleRate, int maxBlockSize)
{
    saturator.prepare(sampleRate, maxBlockSize);
    // xPrev automatically reset to 0 inside prepare()
}
```

State must be reset when:
- `prepareToPlay` is called (new session, sample rate change)
- NOT between processBlock calls (state persists across buffer boundaries)

### Thread Safety

- `xPrev[]` is accessed only from the audio thread -- no locking needed.
- Parameter values are read via `std::atomic<float>::load()` -- lock-free, thread-safe.
- DC blocker filter state is audio-thread-only -- no locking needed.
- The `alpha` (h_curve) parameter is read once per processBlock and held constant for the entire block. This prevents mid-block alpha changes from causing ADAA discontinuities.

### SIMD Optimization Potential

First-order ADAA has limited SIMD potential because it is inherently sequential within a single channel -- each sample depends on the previous sample's `xPrev`. However:

1. **Cross-channel parallelism:** Process left and right channels simultaneously using SIMD pairs. Pack `{xPrev_L, xPrev_R}` into a SIMD register and compute both channels in parallel.

2. **Vectorized `logcosh`:** The `F1_soft` computation (logcosh) can use SIMD transcendental approximations. Libraries like XSIMD or Intel's SVML provide vectorized `log`, `exp`, `cosh`.

3. **Block-based parameter interpolation:** If parameters are smoothed per-block rather than per-sample, the `driveGain`, `alpha`, `bias`, `oddGain` values can be precomputed once per block.

4. **Lookup tables:** For extreme optimization, `logcosh(x)` and `tanh(x)` can be replaced with lookup tables + linear interpolation. The accuracy requirement for ADAA is approximately 1e-5 relative error in `F1`, which requires a table of ~4096 entries over the useful input range [-10, 10]. This eliminates all transcendental function calls.

**Recommendation:** Start with scalar double-precision math. Profile. If CPU exceeds 15% at 48kHz stereo, consider lookup tables for `logcosh` as the first optimization step.

---

## 8. Known Pitfalls and Edge Cases

### Denormals

**Problem:** After heavy waveshaping, very small output values (< ~1e-38 for float) can become denormalized numbers, which are extremely slow to process on x86 CPUs (100-300x slower per operation).

**Solution:** `juce::ScopedNoDenormals` at the top of processBlock sets the CPU's flush-to-zero (FTZ) and denormals-are-zero (DAZ) flags. This must be done every processBlock call (the flags are thread-local and may be reset by other code).

**Specific ADAA risk:** The `F1(x[n]) - F1(x[n-1])` difference can produce denormals when consecutive samples are nearly equal and the function value is near zero. The near-singularity fallback (epsilon = 1e-5) catches most of these, but the FTZ flag is still essential as a safety net.

### Numerical Precision at High Drive

**Problem:** At `driveGain = 16x`, a full-scale input (+1.0) becomes `x_driven = 16.0`. At this level:
- `tanh(16.0) = 0.99999999...` (essentially 1.0)
- `logcosh(16.0) = 16.0 - ln(2) = 15.3069...`

The ADAA formula becomes:

```
y = (logcosh(16.0 * x[n]) - logcosh(16.0 * x[n-1])) / (16.0 * x[n] - 16.0 * x[n-1])
```

When the input is near full scale, the tanh is saturated and changes very slowly -- the numerator and denominator both become small, approaching the near-singularity condition. The output is essentially `tanh(16 * x) ~= sign(x)`, which is correct behavior (hard clipping).

**Risk:** At maximum drive with high-frequency content (e.g., a 15kHz sinusoid at full scale at 48kHz), there are only ~3 samples per cycle. The ADAA averaging over such coarse sampling may lose effectiveness. This is inherent to all anti-aliasing at low sample rates with extreme nonlinearities.

**Mitigation:** The architecture already limits the polynomial hard clipper to `1.5x - 0.5x^3`, which has a gentler saturation curve than `tanh(16x)`. The `h_curve` blend naturally limits the most extreme drive behavior. For users who need extreme distortion at high frequencies, recommend increasing the DAW sample rate to 96kHz.

### Behavior with DC Input

**Problem:** A constant DC input `x[n] = C` for all `n` means `delta = 0` every sample. The near-singularity fallback always triggers, returning `f(C)`.

**Correctness:** This is mathematically correct -- the ADAA of a constant input is `f(C)`, since there is no aliasing from a DC signal.

**Practical concern:** If the DC input has a small amount of noise (quantization noise, dither), the signal will hover near the epsilon boundary, rapidly switching between ADAA and direct evaluation. With epsilon = 1e-5, the two paths produce values that differ by less than 1e-10, so this rapid switching is inaudible.

### Behavior at Extreme Sample Rates (192kHz)

At 192kHz, the sample period is 5.2 microseconds. The ADAA half-sample delay is 2.6 microseconds (vs. 10.4 microseconds at 48kHz). The anti-aliasing quality *improves* at higher sample rates because:

1. The Nyquist frequency is 96kHz, so most aliased harmonics are above the audible range even without anti-aliasing.
2. The sinc envelope of the ADAA filter stretches to 96kHz, providing more attenuation per octave in the audible band.
3. Sample-to-sample differences are smaller (smoother signal), so the near-singularity case triggers more frequently for quiet signals -- but this is handled correctly by the fallback.

**At 192kHz, ADAA becomes almost unnecessary** -- but it still improves quality slightly and costs almost nothing, so it should remain active at all sample rates.

### Potential for NaN/Inf

**NaN sources:**
1. `0.0 / 0.0` in the ADAA formula -- prevented by the epsilon check.
2. `log(0.0)` in `logcosh` -- prevented because `cosh(x) >= 1.0` for all real x, so `logcosh(x) >= 0.0`. However, in the stable formulation, `log1p(exp(-2|x|))` could produce `log1p(0)` = 0 for very large `|x|`, which is fine.
3. `log(negative)` -- impossible since `cosh(x) > 0` always.
4. Accumulated NaN from previous NaN input -- if a NaN enters the input buffer (corrupt audio), it propagates through all math and poisons `xPrev`. Reset `xPrev` to 0 if NaN is detected.

**Inf sources:**
1. `exp(x)` for very large `x` -- in `logcosh_stable`, the `exp(-2|x|)` term underflows to 0 for `|x| > 350` (double), which is safe.
2. Division by a very small `delta` just above epsilon -- the result can be large but finite. With epsilon = 1e-5 and double precision, the worst case is `~1e5 * F1'(x) ~= 1e5`, which is large but not infinite.

**Inf prevention:**

```cpp
// After ADAA computation, clamp output to prevent downstream issues
y = std::clamp(y, -10.0, 10.0);  // No physical signal exceeds this after waveshaping
```

**NaN detection (paranoid safety):**

```cpp
if (std::isnan(y) || std::isinf(y))
{
    y = 0.0;
    xPrev[channel] = 0.0;  // Reset state to prevent NaN propagation
}
```

### Zipper Noise from Parameter Changes

**Problem:** If `driveGain` or `alpha` changes abruptly between processBlock calls, the discontinuity in the waveshaper's response can create audible clicks.

**Solution:** Use `juce::LinearSmoothedValue` for all parameters that affect the ADAA computation (drive, h_curve, even, odd). Apply smoothing at the processBlock level (not per-sample, to keep alpha constant within the ADAA step -- see Section 4 warning).

**Recommended approach:** Smooth parameters once at the start of each processBlock. This means parameter changes take effect with block-sized granularity (e.g., 512 samples = ~10.7ms at 48kHz), which is smooth enough for knob turns.

For sample-accurate smoothing of drive/shape:

```cpp
// Per-block approach: read smoothed value once
float smoothedDrive = driveSmoother.getNextValue();
float smoothedAlpha = alphaSmoother.getNextValue();

// Use these constant values for the entire block
for (int n = 0; n < numSamples; ++n)
    saturator.processSample(data[n], ch, smoothedDrive, smoothedAlpha, bias, oddGain);
```

---

## 9. Reference Implementations and Literature

### Key Papers

**[1] Parker, Zavalishin, Le Bivic -- "Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution" (DAFx 2016)**
The foundational paper. Introduces the continuous-time convolution framework and derives the first-order ADAA formula. Demonstrates the technique on hard clipping and tanh waveshaping. Includes MATLAB implementations.
- Paper: https://www.dafx.de/paper-archive/details/vem_XXF5qBbfiWOH2RVVAA
- Code: https://github.com/julian-parker/DAFX-AntiAliasing

**[2] Bilbao, Esqueda, Parker, Valimaki -- "Antiderivative Antialiasing for Memoryless Nonlinearities" (IEEE Signal Processing Letters, 2017)**
Extends ADAA to second and third order using higher antiderivatives. Provides the rigorous mathematical framework and aliasing suppression analysis. Shows second-order ADAA achieves ~6x oversampling equivalent quality.
- Paper: https://ieeexplore.ieee.org/document/7865908/

**[3] Esqueda, Bilbao, Valimaki -- "Aliasing Reduction in Nonlinear Audio Signal Processing" (Aalto University, 2018)**
Comprehensive doctoral thesis covering ADAA theory, implementation, and comparison with oversampling. Essential reading for understanding the theoretical limits.
- Thesis: https://aaltodoc.aalto.fi/handle/123456789/30723

**[4] Chowdhury -- "Practical Considerations for Antiderivative Anti-Aliasing" (CCRMA, 2020)**
Practical engineering guide. Covers the near-singularity problem, tolerance selection, lookup table optimization, and real-world implementation in C++/JUCE. Source code available.
- Article: https://ccrma.stanford.edu/~jatin/Notebooks/adaa.html
- Code: https://github.com/jatinchowdhury18/ADAA

### Open-Source Implementations Worth Studying

**Jatin Chowdhury's ADAA repository**
C++/JUCE plugin with first-order, second-order, and lookup-table variants for hard clip and tanh. The reference implementation for audio plugin developers.
- https://github.com/jatinchowdhury18/ADAA

**Faust AANL (Antialiased Nonlinearities) Library**
Production-ready ADAA implementations in the Faust DSP language. Includes `hardclip`, `cubic`, `tanh1`, `arctan`, `sin` with first and second-order ADAA. Uses epsilon = 0.001 for near-singularity detection.
- https://faustlibraries.grame.fr/libs/aanl/

**Julian Parker's DAFX-AntiAliasing**
MATLAB implementation companion to the original paper. Useful for verification and algorithm prototyping before C++ implementation.
- https://github.com/julian-parker/DAFX-AntiAliasing

**apulsoft blog: tanh Antiderivatives**
Detailed analysis of numerical stability issues with `logcosh` and the second antiderivative of tanh (involving the dilogarithm). Essential reading for understanding why second-order ADAA with tanh is impractical.
- https://apulsoft.ch/blog/tanh-antiderivatives/

### Commercial Plugin Approaches

**FabFilter Saturn 2:** Uses oversampling (up to 8x) for anti-aliasing, applied only to the saturation section to minimize CPU cost. Does NOT use ADAA. This is the industry standard approach but incurs significant CPU overhead at high oversampling rates.

**Soundtoys Decapitator:** Uses analog circuit modeling but has minimal anti-aliasing. Independent measurements show aliasing artifacts, though at levels generally considered inaudible in mixing contexts. This demonstrates that commercial success does not require perfect anti-aliasing -- but DynaDrive's ADAA approach gives it a measurable technical advantage over Decapitator.

**Klanghelm SDRR / IVGI:** Known to use polynomial waveshapers with gentle curves that naturally limit harmonic generation. Effective for subtle saturation but does not address aliasing mathematically.

**Airwindows:** Many plugins use simple waveshapers with no anti-aliasing or minimal oversampling. The developer's philosophy prioritizes simplicity and character over measurement. DynaDrive takes the opposite approach -- measured perfection as a feature.

---

## Summary: Implementation Checklist for DynaDrive

### Phase 4.1 (Core ADAA) -- Must Get Right

- [ ] Implement `logcosh_stable()` using the `|x| + log1p(exp(-2|x|)) - ln(2)` identity
- [ ] Use double precision for all ADAA math (cast input to double, compute, cast output to float)
- [ ] Epsilon threshold = 1e-5 with simple branching fallback to direct `f(x)` evaluation
- [ ] Store `xPrev` as the driven/processed input value (not raw input)
- [ ] Initialize `xPrev = 0.0` in `prepareToPlay`
- [ ] Add `juce::ScopedNoDenormals` at top of processBlock
- [ ] Add NaN/Inf safety check on output (clamp to [-10, 10])
- [ ] Verify `F1_soft(0) = 0` and `F1_hard(0) = 0` (ensures clean startup)

### Phase 4.2 (Morphing + Harmonics) -- Must Get Right

- [ ] Blend antiderivatives linearly: `F1_blend = (1-alpha)*F1_soft + alpha*F1_hard`
- [ ] Store `x_prev` (not `F1_prev`) to allow recomputation with current alpha
- [ ] Apply processing order: odd pre-distortion -> bias -> drive -> ADAA
- [ ] Verify `F1_hard` continuity at `|x| = 1` boundary: `F1_hard(1) = 0.625` from both sides
- [ ] DC blocker at 5Hz after waveshaper for even harmonic bias removal
- [ ] Smooth drive, h_curve, even, odd parameters (per-block, not per-sample)
- [ ] Hold alpha constant within each ADAA sample step

### Performance Targets

| Metric                          | Target           |
|---------------------------------|------------------|
| CPU at 48kHz stereo, 512 buf    | < 15%            |
| CPU at 96kHz stereo, 512 buf    | < 25%            |
| Aliasing level (1kHz sine, 50% drive) | < -60 dB    |
| THD+N at 0% drive               | < -120 dB (unity)|
| Latency                         | 0 samples        |

---

*End of technical reference.*
