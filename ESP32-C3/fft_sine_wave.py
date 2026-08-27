import numpy as np
import matplotlib.pyplot as plt

# --------------------------------------------------
# Signal settings
# --------------------------------------------------
fs = 200          # Sampling frequency [Hz]
duration = 2.0    # Signal duration [s]

f1 = 10           # Frequency component 1 [Hz]
f2 = 25           # Frequency component 2 [Hz]

A1 = 1.0          # Amplitude of f1
A2 = 0.5          # Amplitude of f2

# --------------------------------------------------
# Generate time-domain signal
# --------------------------------------------------
t = np.arange(0, duration, 1 / fs)

y1 = A1 * np.sin(2 * np.pi * f1 * t)
y2 = A2 * np.sin(2 * np.pi * f2 * t)

y = y1 + y2

# --------------------------------------------------
# FFT
# --------------------------------------------------
N = len(y)

fft_result = np.fft.rfft(y)
freq = np.fft.rfftfreq(N, d=1 / fs)

amplitude = np.abs(fft_result) * 2 / N

# DC and Nyquist components should not be doubled
amplitude[0] /= 2
if N % 2 == 0:
    amplitude[-1] /= 2

# --------------------------------------------------
# Plot
# --------------------------------------------------
fig, ax = plt.subplots(2, 1, figsize=(10, 7))

# Time-domain signal
ax[0].plot(t, y)
ax[0].set_title(f"Time Domain: {f1} Hz + {f2} Hz")
ax[0].set_xlabel("Time [s]")
ax[0].set_ylabel("Amplitude")
ax[0].grid(True)

# Frequency-domain signal
ax[1].stem(freq, amplitude, basefmt=" ")
ax[1].set_title("Frequency Domain (FFT)")
ax[1].set_xlabel("Frequency [Hz]")
ax[1].set_ylabel("Amplitude")
ax[1].set_xlim(0, 50)
ax[1].grid(True)

plt.tight_layout()
plt.show()
