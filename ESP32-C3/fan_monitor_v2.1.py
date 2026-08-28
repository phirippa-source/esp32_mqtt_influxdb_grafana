import serial
import matplotlib.pyplot as plt

# -------------------------------------------------------
# Serial 설정
# -------------------------------------------------------

PORT = "COM48"       # 실제 포트에 맞게 수정
BAUDRATE = 230400

ser = serial.Serial(
    PORT,
    BAUDRATE,
    timeout=1
)


# -------------------------------------------------------
# 그래프 설정
# -------------------------------------------------------

plt.ion()

fig, axes = plt.subplots(
    3, 1,
    figsize=(10, 8),
    sharex=True
)

fig.suptitle("LIS3DH 3-Axis FFT Spectrum")

axis_names = ["X-axis", "Y-axis", "Z-axis"]

lines = []

for ax, name in zip(axes, axis_names):

    line, = ax.plot([], [])

    lines.append(line)

    ax.set_title(name)
    ax.set_ylabel("Magnitude")
    ax.grid(True)

axes[2].set_xlabel("Frequency (Hz)")

plt.tight_layout()


# -------------------------------------------------------
# 한 축의 FFT 데이터 읽기
# -------------------------------------------------------

def read_fft_data(end_marker):

    frequencies = []
    magnitudes = []

    while True:

        line = ser.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()

        if not line:
            continue

        # FFT_X_END, FFT_Y_END, FFT_Z_END
        if line == end_marker:
            break

        try:
            freq, mag = line.split(",")

            frequencies.append(float(freq))
            magnitudes.append(float(mag))

        except ValueError:
            # 형식이 맞지 않는 데이터는 무시
            continue

    return frequencies, magnitudes


# -------------------------------------------------------
# Main Loop
# -------------------------------------------------------

print("Waiting for FFT data...")

try:

    while True:

        line = ser.readline().decode(
            "utf-8",
            errors="ignore"
        ).strip()


        # ------------------------------------------------
        # 하나의 FFT Frame 시작
        # ------------------------------------------------

        if line != "FRAME_BEGIN":
            continue


        x_freq = []
        x_mag = []

        y_freq = []
        y_mag = []

        z_freq = []
        z_mag = []


        # ------------------------------------------------
        # FRAME_END가 나올 때까지 데이터 수신
        # ------------------------------------------------

        while True:

            line = ser.readline().decode(
                "utf-8",
                errors="ignore"
            ).strip()


            # X축
            if line == "FFT_X_BEGIN":

                x_freq, x_mag = read_fft_data(
                    "FFT_X_END"
                )


            # Y축
            elif line == "FFT_Y_BEGIN":

                y_freq, y_mag = read_fft_data(
                    "FFT_Y_END"
                )


            # Z축
            elif line == "FFT_Z_BEGIN":

                z_freq, z_mag = read_fft_data(
                    "FFT_Z_END"
                )


            # 한 Frame 종료
            elif line == "FRAME_END":

                break


        # ------------------------------------------------
        # X축 Spectrum
        # ------------------------------------------------

        lines[0].set_data(
            x_freq,
            x_mag
        )

        axes[0].relim()
        axes[0].autoscale_view()


        # ------------------------------------------------
        # Y축 Spectrum
        # ------------------------------------------------

        lines[1].set_data(
            y_freq,
            y_mag
        )

        axes[1].relim()
        axes[1].autoscale_view()


        # ------------------------------------------------
        # Z축 Spectrum
        # ------------------------------------------------

        lines[2].set_data(
            z_freq,
            z_mag
        )

        axes[2].relim()
        axes[2].autoscale_view()


        # ------------------------------------------------
        # X축 주파수 범위 고정
        #
        # Fs = 400 Hz
        # Nyquist = 200 Hz
        # ------------------------------------------------

        for ax in axes:
            ax.set_xlim(0, 200)


        # ------------------------------------------------
        # 그래프 갱신
        # ------------------------------------------------

        fig.canvas.draw()
        fig.canvas.flush_events()

        plt.pause(0.001)


except KeyboardInterrupt:

    print("\nProgram stopped.")


finally:

    ser.close()
    plt.ioff()
    plt.show()
