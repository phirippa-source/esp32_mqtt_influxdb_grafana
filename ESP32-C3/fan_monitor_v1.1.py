import serial
import matplotlib.pyplot as plt

PORT = "COM48"       # 실제 ESP32-C3의 COM 포트 번호로 변경
BAUDRATE = 230400

ser = serial.Serial(PORT, BAUDRATE, timeout=1)

plt.ion()

fig, ax = plt.subplots()

while True:

    line = ser.readline().decode(errors="ignore").strip()

    if line != "FFT_BEGIN":
        continue

    frequencies = []
    magnitudes = []

    while True:

        line = ser.readline().decode(errors="ignore").strip()

        if line == "FFT_END":
            break

        try:
            freq, mag = line.split(",")

            frequencies.append(float(freq))
            magnitudes.append(float(mag))

        except ValueError:
            pass

    ax.clear()

    ax.plot(frequencies, magnitudes)

    ax.set_xlabel("Frequency [Hz]")
    ax.set_ylabel("Magnitude")
    ax.set_title("LIS3DH X-axis FFT")

    ax.set_xlim(0, 200)
    ax.grid(True)

    plt.pause(0.01)
