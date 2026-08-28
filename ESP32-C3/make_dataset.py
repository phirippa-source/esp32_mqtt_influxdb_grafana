import csv
import os
import queue
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox

import serial
import serial.tools.list_ports


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

        # 화면 높이를 너무 크게 잡지 않도록 조정
        self.root.geometry("780x610")
        self.root.resizable(False, False)

        # ----------------------------------------------------
        # Serial 관련
        # ----------------------------------------------------

        self.ser = None
        self.serial_thread = None
        self.running = False
        self.data_queue = queue.Queue()

        # ----------------------------------------------------
        # 데이터 저장 상태
        # ----------------------------------------------------

        self.recording = False
        self.current_label = None

        self.counts = {
            "NORMAL": 0,
            "CONTACT": 0,
            "AIRFLOW_RESTRICTED": 0
        }

        # ----------------------------------------------------
        # GUI 구성
        # ----------------------------------------------------

        self.create_widgets()

        self.refresh_ports()

        self.root.after(
            100,
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

        title.pack(pady=(12, 8))


        # ====================================================
        # 1. Serial Connection
        # ====================================================

        connection_frame = ttk.LabelFrame(
            self.root,
            text="Serial Connection",
            padding=8
        )

        connection_frame.pack(
            fill="x",
            padx=20,
            pady=5
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
            width=15,
            state="readonly"
        )

        self.port_combo.grid(
            row=0,
            column=1,
            padx=5
        )

        self.refresh_button = ttk.Button(
            connection_frame,
            text="Refresh",
            command=self.refresh_ports,
            width=10
        )

        self.refresh_button.grid(
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
        # 2. Current Feature
        # ====================================================

        feature_frame = ttk.LabelFrame(
            self.root,
            text="Current Feature",
            padding=8
        )

        feature_frame.pack(
            fill="x",
            padx=20,
            pady=5
        )

        self.value_labels = {}

        # 값 + 단위를 하나의 문자열로 표시
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
                width=12
            ).grid(
                row=row,
                column=col,
                padx=(10, 5),
                pady=4,
                sticky="e"
            )

            value_label = ttk.Label(
                feature_frame,
                text="---",
                width=22,
                font=("Consolas", 11, "bold")
            )

            value_label.grid(
                row=row,
                column=col + 1,
                padx=(0, 20),
                pady=4,
                sticky="w"
            )

            self.value_labels[name] = value_label


        # ====================================================
        # 3. Dataset Collection
        # ====================================================

        collect_frame = ttk.LabelFrame(
            self.root,
            text="Dataset Collection",
            padding=10
        )

        collect_frame.pack(
            fill="x",
            padx=20,
            pady=5
        )

        # 5개 버튼을 한 줄에 배치
        self.normal_button = ttk.Button(
            collect_frame,
            text="NORMAL",
            command=lambda: self.start_recording("NORMAL"),
            width=16
        )

        self.normal_button.grid(
            row=0,
            column=0,
            padx=4
        )

        self.contact_button = ttk.Button(
            collect_frame,
            text="CONTACT",
            command=lambda: self.start_recording("CONTACT"),
            width=16
        )

        self.contact_button.grid(
            row=0,
            column=1,
            padx=4
        )

        self.airflow_button = ttk.Button(
            collect_frame,
            text="AIRFLOW_RESTRICTED",
            command=lambda: self.start_recording(
                "AIRFLOW_RESTRICTED"
            ),
            width=20
        )

        self.airflow_button.grid(
            row=0,
            column=2,
            padx=4
        )

        self.pause_button = ttk.Button(
            collect_frame,
            text="PAUSE",
            command=self.pause_recording,
            width=12
        )

        self.pause_button.grid(
            row=0,
            column=3,
            padx=4
        )

        self.quit_button = ttk.Button(
            collect_frame,
            text="QUIT",
            command=self.on_close,
            width=12
        )

        self.quit_button.grid(
            row=0,
            column=4,
            padx=4
        )


        # ====================================================
        # 4. Recording Status
        # ====================================================

        status_frame = ttk.LabelFrame(
            self.root,
            text="Recording Status",
            padding=8
        )

        status_frame.pack(
            fill="x",
            padx=20,
            pady=5
        )

        self.recording_status = ttk.Label(
            status_frame,
            text="PAUSED",
            font=("Arial", 16, "bold")
        )

        self.recording_status.pack(
            pady=4
        )


        # ====================================================
        # 5. Collected Samples
        # ====================================================

        count_frame = ttk.LabelFrame(
            self.root,
            text="Collected Samples",
            padding=8
        )

        count_frame.pack(
            fill="x",
            padx=20,
            pady=5
        )

        self.count_labels = {}

        labels = [
            "NORMAL",
            "CONTACT",
            "AIRFLOW_RESTRICTED"
        ]

        for i, label in enumerate(labels):

            ttk.Label(
                count_frame,
                text=label,
                font=("Arial", 10, "bold")
            ).grid(
                row=0,
                column=i,
                padx=45,
                pady=(0, 2)
            )

            count_label = ttk.Label(
                count_frame,
                text="0",
                font=("Arial", 15)
            )

            count_label.grid(
                row=1,
                column=i,
                padx=45,
                pady=(0, 2)
            )

            self.count_labels[label] = count_label


        # ====================================================
        # 6. CSV File
        # ====================================================

        file_frame = ttk.Frame(
            self.root
        )

        file_frame.pack(
            fill="x",
            padx=22,
            pady=(5, 8)
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
            padx=10
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
    # Serial 연결 / 해제
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
    # Serial 연결 해제
    # ========================================================

    def disconnect_serial(self):

        self.recording = False
        self.current_label = None
        self.running = False

        if self.ser:

            try:

                if self.ser.is_open:
                    self.ser.close()

            except:
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
    # Serial Reader
    # ========================================================

    def serial_reader(self):

        while self.running:

            try:

                line = self.ser.readline().decode(
                    "utf-8",
                    errors="ignore"
                ).strip()

                if not line:
                    continue

                if line.startswith("Voltage"):
                    continue

                values = line.split(",")

                if len(values) != 8:
                    continue

                try:

                    data = [
                        float(value)
                        for value in values
                    ]

                except ValueError:
                    continue

                self.data_queue.put(data)

            except serial.SerialException:
                break


    # ========================================================
    # Queue 처리
    # ========================================================

    def process_queue(self):

        try:

            while True:

                data = self.data_queue.get_nowait()

                self.update_feature_display(data)

                if self.recording and self.current_label:

                    self.save_data(
                        data,
                        self.current_label
                    )

        except queue.Empty:
            pass

        self.root.after(
            100,
            self.process_queue
        )


    # ========================================================
    # Feature 표시
    # ========================================================

    def update_feature_display(self, data):

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
    # Recording 시작
    # ========================================================

    def start_recording(self, label):

        if not self.ser or not self.ser.is_open:

            messagebox.showwarning(
                "Warning",
                "먼저 Serial Port를 연결하세요."
            )

            return

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
    # Recording 일시 정지
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

    def save_data(self, data, label):

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

        self.counts[label] += 1

        self.count_labels[label].config(
            text=str(
                self.counts[label]
            )
        )


    # ========================================================
    # 프로그램 종료
    # ========================================================

    def on_close(self):

        self.disconnect_serial()

        self.root.destroy()


# ============================================================
# Main
# ============================================================

if __name__ == "__main__":

    root = tk.Tk()

    app = DatasetCollectorApp(root)

    root.mainloop()
