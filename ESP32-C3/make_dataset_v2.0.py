import csv
import os
import queue
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox

import serial
import serial.tools.list_ports

from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


# ============================================================
# 기본 설정
# ============================================================

BAUDRATE = 230400
CSV_FILE = "fan_dataset.csv"

HEADER = [
    "Voltage",
    "Current",
    "X_Freq",
    "X_Energy",
    "Y_Freq",
    "Y_Energy",
    "Z_Freq",
    "Z_Energy",
    "Label"
]


# ============================================================
# GUI Application
# ============================================================

class DatasetCollectorApp:

    def __init__(self, root):

        self.root = root

        self.root.title("Fan Dataset Collector")

        # 그래프가 추가되므로 창을 조금 크게 사용
        self.root.geometry("1200x820")
        self.root.minsize(1050, 750)

        # ----------------------------------------------------
        # Serial
        # ----------------------------------------------------

        self.ser = None
        self.running = False
        self.serial_thread = None

        # Serial Thread → GUI Thread
        self.data_queue = queue.Queue()

        # ----------------------------------------------------
        # Recording
        # ----------------------------------------------------

        self.recording = False
        self.current_label = None

        self.counts = {
            "NORMAL": 0,
            "CONTACT": 0,
            "AIRFLOW_RESTRICTED": 0
        }

        # ----------------------------------------------------
        # GUI
        # ----------------------------------------------------

        self.create_widgets()

        self.refresh_ports()

        # Queue 확인
        self.root.after(
            50,
            self.process_queue
        )

        self.root.protocol(
            "WM_DELETE_WINDOW",
            self.on_close
        )


    # ========================================================
    # GUI 생성
    # ========================================================

    def create_widgets(self):

        # ----------------------------------------------------
        # 제목
        # ----------------------------------------------------

        title = ttk.Label(
            self.root,
            text="Fan AI Dataset Collector",
            font=("Arial", 20, "bold")
        )

        title.pack(
            pady=(10, 5)
        )


        # ====================================================
        # 상단 제어 영역
        # ====================================================

        top_frame = ttk.Frame(
            self.root
        )

        top_frame.pack(
            fill="x",
            padx=15,
            pady=5
        )


        # ----------------------------------------------------
        # Serial Connection
        # ----------------------------------------------------

        connection_frame = ttk.LabelFrame(
            top_frame,
            text="Serial Connection",
            padding=8
        )

        connection_frame.pack(
            fill="x"
        )


        ttk.Label(
            connection_frame,
            text="Port:"
        ).grid(
            row=0,
            column=0,
            padx=5
        )


        self.port_combo = ttk.Combobox(
            connection_frame,
            width=12,
            state="readonly"
        )

        self.port_combo.grid(
            row=0,
            column=1,
            padx=5
        )


        ttk.Button(
            connection_frame,
            text="Refresh",
            command=self.refresh_ports,
            width=10
        ).grid(
            row=0,
            column=2,
            padx=5
        )


        self.connect_button = ttk.Button(
            connection_frame,
            text="Connect",
            command=self.toggle_connection,
            width=10
        )

        self.connect_button.grid(
            row=0,
            column=3,
            padx=5
        )


        self.connection_status = ttk.Label(
            connection_frame,
            text="Disconnected"
        )

        self.connection_status.grid(
            row=0,
            column=4,
            padx=15
        )


        # ====================================================
        # Feature + Control 영역
        # ====================================================

        info_frame = ttk.Frame(
            self.root
        )

        info_frame.pack(
            fill="x",
            padx=15,
            pady=3
        )


        # ----------------------------------------------------
        # Current Feature
        # ----------------------------------------------------

        feature_frame = ttk.LabelFrame(
            info_frame,
            text="Current Feature",
            padding=8
        )

        feature_frame.pack(
            side="left",
            fill="both",
            expand=True,
            padx=(0, 5)
        )


        self.value_labels = {}

        feature_names = [
            "Voltage",
            "Current",
            "X_Freq",
            "X_Energy",
            "Y_Freq",
            "Y_Energy",
            "Z_Freq",
            "Z_Energy"
        ]


        for i, name in enumerate(feature_names):

            row = i // 2
            col = (i % 2) * 2

            ttk.Label(
                feature_frame,
                text=name + ":",
                width=11
            ).grid(
                row=row,
                column=col,
                padx=(8, 3),
                pady=3,
                sticky="e"
            )


            value_label = ttk.Label(
                feature_frame,
                text="---",
                width=18,
                font=("Consolas", 10, "bold")
            )

            value_label.grid(
                row=row,
                column=col + 1,
                padx=(0, 10),
                pady=3,
                sticky="w"
            )

            self.value_labels[name] = value_label


        # ----------------------------------------------------
        # Recording Status
        # ----------------------------------------------------

        status_frame = ttk.LabelFrame(
            info_frame,
            text="Recording Status",
            padding=8
        )

        status_frame.pack(
            side="right",
            fill="both",
            padx=(5, 0)
        )


        self.recording_status = ttk.Label(
            status_frame,
            text="PAUSED",
            font=("Arial", 16, "bold"),
            width=28,
            anchor="center"
        )

        self.recording_status.pack(
            padx=10,
            pady=18
        )


        # ====================================================
        # Dataset Collection
        # ====================================================

        collect_frame = ttk.LabelFrame(
            self.root,
            text="Dataset Collection",
            padding=8
        )

        collect_frame.pack(
            fill="x",
            padx=15,
            pady=4
        )


        buttons = [
            (
                "NORMAL",
                lambda: self.start_recording("NORMAL"),
                16
            ),
            (
                "CONTACT",
                lambda: self.start_recording("CONTACT"),
                16
            ),
            (
                "AIRFLOW_RESTRICTED",
                lambda: self.start_recording(
                    "AIRFLOW_RESTRICTED"
                ),
                20
            ),
            (
                "PAUSE",
                self.pause_recording,
                14
            ),
            (
                "QUIT",
                self.on_close,
                14
            )
        ]


        for i, (text, command, width) in enumerate(buttons):

            button = ttk.Button(
                collect_frame,
                text=text,
                command=command,
                width=width
            )

            button.grid(
                row=0,
                column=i,
                padx=6,
                pady=2
            )


        # ====================================================
        # FFT Spectrum
        # ====================================================

        spectrum_frame = ttk.LabelFrame(
            self.root,
            text="Live 3-Axis FFT Spectrum",
            padding=4
        )

        spectrum_frame.pack(
            fill="both",
            expand=True,
            padx=15,
            pady=4
        )


        # Matplotlib Figure
        self.fig = Figure(
            figsize=(10, 5.2),
            dpi=100
        )


        self.ax_x = self.fig.add_subplot(311)
        self.ax_y = self.fig.add_subplot(
            312,
            sharex=self.ax_x
        )
        self.ax_z = self.fig.add_subplot(
            313,
            sharex=self.ax_x
        )


        # Line 객체
        self.line_x, = self.ax_x.plot([], [])
        self.line_y, = self.ax_y.plot([], [])
        self.line_z, = self.ax_z.plot([], [])


        # 그래프 설정
        axes = [
            (self.ax_x, "X-axis"),
            (self.ax_y, "Y-axis"),
            (self.ax_z, "Z-axis")
        ]


        for ax, title_text in axes:

            ax.set_title(
                title_text,
                fontsize=10
            )

            ax.set_ylabel(
                "Magnitude"
            )

            ax.grid(True)

            ax.set_xlim(
                0,
                200
            )


        self.ax_z.set_xlabel(
            "Frequency (Hz)"
        )


        self.fig.tight_layout(
            pad=1.0
        )


        self.canvas = FigureCanvasTkAgg(
            self.fig,
            master=spectrum_frame
        )

        self.canvas.get_tk_widget().pack(
            fill="both",
            expand=True
        )


        # ====================================================
        # Collected Samples
        # ====================================================

        count_frame = ttk.LabelFrame(
            self.root,
            text="Collected Samples",
            padding=6
        )

        count_frame.pack(
            fill="x",
            padx=15,
            pady=4
        )


        self.count_labels = {}


        labels = [
            "NORMAL",
            "CONTACT",
            "AIRFLOW_RESTRICTED"
        ]


        for i, label in enumerate(labels):

            count_frame.columnconfigure(
                i,
                weight=1
            )


            ttk.Label(
                count_frame,
                text=label,
                font=("Arial", 10, "bold")
            ).grid(
                row=0,
                column=i,
                pady=1
            )


            count_label = ttk.Label(
                count_frame,
                text="0",
                font=("Arial", 14)
            )

            count_label.grid(
                row=1,
                column=i,
                pady=1
            )


            self.count_labels[label] = count_label


        # ====================================================
        # CSV 정보
        # ====================================================

        file_frame = ttk.Frame(
            self.root
        )

        file_frame.pack(
            fill="x",
            padx=18,
            pady=(2, 6)
        )


        ttk.Label(
            file_frame,
            text="CSV File:"
        ).pack(
            side="left"
        )


        ttk.Label(
            file_frame,
            text=CSV_FILE,
            font=("Consolas", 10, "bold")
        ).pack(
            side="left",
            padx=8
        )


    # ========================================================
    # COM Port 검색
    # ========================================================

    def refresh_ports(self):

        ports = [
            port.device
            for port in serial.tools.list_ports.comports()
        ]

        self.port_combo["values"] = ports

        if ports:

            self.port_combo.current(0)


    # ========================================================
    # Connect / Disconnect
    # ========================================================

    def toggle_connection(self):

        if self.ser and self.ser.is_open:

            self.disconnect_serial()
            return


        port = self.port_combo.get()


        if not port:

            messagebox.showwarning(
                "Warning",
                "COM Port를 선택하세요."
            )

            return


        try:

            self.ser = serial.Serial(
                port,
                BAUDRATE,
                timeout=1
            )

            time.sleep(2)

            self.ser.reset_input_buffer()

            self.running = True


            self.serial_thread = threading.Thread(
                target=self.serial_reader,
                daemon=True
            )

            self.serial_thread.start()


            self.connection_status.config(
                text=f"Connected : {port}"
            )

            self.connect_button.config(
                text="Disconnect"
            )


        except Exception as e:

            messagebox.showerror(
                "Serial Error",
                str(e)
            )


    # ========================================================
    # Disconnect
    # ========================================================

    def disconnect_serial(self):

        self.recording = False
        self.current_label = None
        self.running = False


        if self.ser:

            try:

                if self.ser.is_open:
                    self.ser.close()

            except Exception:
                pass


        self.connection_status.config(
            text="Disconnected"
        )

        self.connect_button.config(
            text="Connect"
        )

        self.recording_status.config(
            text="PAUSED"
        )


    # ========================================================
    # Serial Reader Thread
    #
    # 보드 데이터 구조
    #
    # FRAME_BEGIN
    # FEATURE,...
    #
    # FFT_X_BEGIN
    # freq,mag
    # ...
    # FFT_X_END
    #
    # FFT_Y_BEGIN
    # ...
    #
    # FFT_Z_BEGIN
    # ...
    #
    # FRAME_END
    # ========================================================

    def serial_reader(self):

        current_fft_axis = None

        spectrum_freq = []
        spectrum_mag = []


        while self.running:

            try:

                line = self.ser.readline().decode(
                    "utf-8",
                    errors="ignore"
                ).strip()


                if not line:
                    continue


                # ------------------------------------------------
                # Feature
                # ------------------------------------------------

                if line.startswith("FEATURE,"):

                    parts = line.split(",")

                    # FEATURE + 8 values
                    if len(parts) != 9:
                        continue


                    try:

                        feature_data = [
                            float(value)
                            for value in parts[1:]
                        ]

                    except ValueError:
                        continue


                    self.data_queue.put(
                        (
                            "FEATURE",
                            feature_data
                        )
                    )

                    continue


                # ------------------------------------------------
                # FFT 시작
                # ------------------------------------------------

                if line == "FFT_X_BEGIN":

                    current_fft_axis = "X"
                    spectrum_freq = []
                    spectrum_mag = []

                    continue


                if line == "FFT_Y_BEGIN":

                    current_fft_axis = "Y"
                    spectrum_freq = []
                    spectrum_mag = []

                    continue


                if line == "FFT_Z_BEGIN":

                    current_fft_axis = "Z"
                    spectrum_freq = []
                    spectrum_mag = []

                    continue


                # ------------------------------------------------
                # FFT 종료
                # ------------------------------------------------

                if line in (
                    "FFT_X_END",
                    "FFT_Y_END",
                    "FFT_Z_END"
                ):

                    if current_fft_axis:

                        self.data_queue.put(
                            (
                                "FFT",
                                current_fft_axis,
                                spectrum_freq,
                                spectrum_mag
                            )
                        )


                    current_fft_axis = None

                    continue


                # ------------------------------------------------
                # FFT 데이터
                # ------------------------------------------------

                if current_fft_axis:

                    parts = line.split(",")


                    if len(parts) != 2:
                        continue


                    try:

                        freq = float(
                            parts[0]
                        )

                        mag = float(
                            parts[1]
                        )

                    except ValueError:

                        continue


                    spectrum_freq.append(
                        freq
                    )

                    spectrum_mag.append(
                        mag
                    )


            except serial.SerialException:

                break


            except Exception:

                continue


    # ========================================================
    # Queue 처리
    # ========================================================

    def process_queue(self):

        graph_updated = False


        try:

            while True:

                item = self.data_queue.get_nowait()


                # ------------------------------------------------
                # Feature
                # ------------------------------------------------

                if item[0] == "FEATURE":

                    data = item[1]

                    self.update_feature_display(
                        data
                    )


                    # Recording 상태일 때만 저장
                    if (
                        self.recording
                        and self.current_label
                    ):

                        self.save_data(
                            data,
                            self.current_label
                        )


                # ------------------------------------------------
                # FFT
                # ------------------------------------------------

                elif item[0] == "FFT":

                    axis = item[1]
                    freq = item[2]
                    mag = item[3]


                    self.update_spectrum(
                        axis,
                        freq,
                        mag
                    )

                    graph_updated = True


        except queue.Empty:

            pass


        # 한 번에 Canvas 갱신
        if graph_updated:

            self.canvas.draw_idle()


        self.root.after(
            50,
            self.process_queue
        )


    # ========================================================
    # Feature 표시
    # ========================================================

    def update_feature_display(
        self,
        data
    ):

        (
            voltage,
            current,
            x_freq,
            x_energy,
            y_freq,
            y_energy,
            z_freq,
            z_energy
        ) = data


        self.value_labels["Voltage"].config(
            text=f"{voltage:.3f} V"
        )

        self.value_labels["Current"].config(
            text=f"{current:.2f} mA"
        )


        self.value_labels["X_Freq"].config(
            text=f"{x_freq:.2f} Hz"
        )

        self.value_labels["X_Energy"].config(
            text=f"{x_energy:.6f}"
        )


        self.value_labels["Y_Freq"].config(
            text=f"{y_freq:.2f} Hz"
        )

        self.value_labels["Y_Energy"].config(
            text=f"{y_energy:.6f}"
        )


        self.value_labels["Z_Freq"].config(
            text=f"{z_freq:.2f} Hz"
        )

        self.value_labels["Z_Energy"].config(
            text=f"{z_energy:.6f}"
        )


    # ========================================================
    # Spectrum 표시
    # ========================================================

    def update_spectrum(
        self,
        axis,
        freq,
        mag
    ):

        if not freq:
            return


        if axis == "X":

            ax = self.ax_x
            line = self.line_x


        elif axis == "Y":

            ax = self.ax_y
            line = self.line_y


        elif axis == "Z":

            ax = self.ax_z
            line = self.line_z


        else:

            return


        line.set_data(
            freq,
            mag
        )


        # ----------------------------------------------------
        # Y축 Auto Scale
        # ----------------------------------------------------

        ax.relim()

        ax.autoscale_view(
            scalex=False,
            scaley=True
        )

        # X축은 항상 고정
        ax.set_xlim(
            0,
            200
        )


    # ========================================================
    # Recording 시작
    # ========================================================

    def start_recording(
        self,
        label
    ):

        if not self.ser or not self.ser.is_open:

            messagebox.showwarning(
                "Warning",
                "먼저 Serial Port를 연결하세요."
            )

            return


        # ----------------------------------------------------
        # 상태 변경 과정의 과거 데이터를 제거
        # ----------------------------------------------------

        self.ser.reset_input_buffer()


        while not self.data_queue.empty():

            try:

                self.data_queue.get_nowait()

            except queue.Empty:

                break


        self.current_label = label
        self.recording = True


        self.recording_status.config(
            text=f"RECORDING : {label}"
        )


    # ========================================================
    # PAUSE
    # ========================================================

    def pause_recording(self):

        self.recording = False
        self.current_label = None


        self.recording_status.config(
            text="PAUSED"
        )


    # ========================================================
    # CSV 저장
    # ========================================================

    def save_data(
        self,
        data,
        label
    ):

        file_exists = os.path.exists(
            CSV_FILE
        )


        with open(
            CSV_FILE,
            "a",
            newline="",
            encoding="utf-8"
        ) as f:

            writer = csv.writer(f)


            if not file_exists:

                writer.writerow(
                    HEADER
                )


            writer.writerow(
                data + [label]
            )


        # ----------------------------------------------------
        # Sample Count
        # ----------------------------------------------------

        self.counts[label] += 1


        self.count_labels[label].config(
            text=str(
                self.counts[label]
            )
        )


    # ========================================================
    # 종료
    # ========================================================

    def on_close(self):

        self.recording = False

        self.disconnect_serial()

        self.root.destroy()


# ============================================================
# Main
# ============================================================

if __name__ == "__main__":

    root = tk.Tk()

    app = DatasetCollectorApp(
        root
    )

    root.mainloop()
