import ipaddress
import json
import socket
import threading
import tkinter as tk
from pathlib import Path
from tkinter import colorchooser, filedialog, messagebox, ttk

import requests

MODE_OPTIONS = [
    ("Цвет", "color"),
    ("Градиент", "gradient"),
    ("Переливание", "rainbow"),
    ("Текст", "text"),
    ("Огонь", "fire"),
    ("Матрица", "matrix"),
    ("Прыгающий пиксель", "bounce"),
]

BRIGHTNESS_LIMIT = 76


class MatrixApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("LED Matrix Controller")
        self.geometry("640x520")

        self.ip_var = tk.StringVar(value="192.168.1.50")
        self.mode_var = tk.StringVar(value="color")
        self.brightness_var = tk.IntVar(value=32)
        self.speed_var = tk.IntVar(value=80)
        self.text_speed_var = tk.IntVar(value=90)
        self.text_var = tk.StringVar(value="HELLO 8266")
        self.text_direction_ltr = tk.BooleanVar(value=False)
        self.status_var = tk.StringVar(value="Не подключено")
        self.color1 = "#FF0000"
        self.color2 = "#0000FF"
        self.color3 = "#8000FF"
        self.text_color = "#FF0000"

        self._build_ui()

    def _build_ui(self):
        top = ttk.Frame(self)
        top.pack(fill="x", padx=8, pady=8)
        ttk.Label(top, text="IP устройства:").pack(side="left")
        ttk.Entry(top, textvariable=self.ip_var, width=16).pack(side="left", padx=4)
        ttk.Button(top, text="Подключиться", command=self.fetch_status).pack(side="left", padx=4)
        ttk.Button(top, text="Автопоиск", command=self.auto_discover).pack(side="left", padx=4)

        body = ttk.Frame(self)
        body.pack(fill="both", expand=True, padx=8, pady=4)

        ttk.Label(body, text="Режим").grid(row=0, column=0, sticky="w")
        mode_combo = ttk.Combobox(body, state="readonly", width=25, values=[m[0] for m in MODE_OPTIONS])
        mode_combo.current(0)
        mode_combo.grid(row=0, column=1, sticky="w")

        def on_mode_change(_):
            self.mode_var.set(MODE_OPTIONS[mode_combo.current()][1])

        mode_combo.bind("<<ComboboxSelected>>", on_mode_change)

        ttk.Label(body, text="Яркость (макс 30%)").grid(row=1, column=0, sticky="w")
        ttk.Scale(body, from_=1, to=BRIGHTNESS_LIMIT, variable=self.brightness_var, orient="horizontal", length=250).grid(
            row=1, column=1, sticky="w"
        )

        ttk.Label(body, text="Скорость эффекта").grid(row=2, column=0, sticky="w")
        ttk.Scale(body, from_=1, to=255, variable=self.speed_var, orient="horizontal", length=250).grid(row=2, column=1, sticky="w")

        ttk.Label(body, text="Текст").grid(row=3, column=0, sticky="w")
        ttk.Entry(body, textvariable=self.text_var, width=30).grid(row=3, column=1, sticky="w")

        ttk.Label(body, text="Скорость текста").grid(row=4, column=0, sticky="w")
        ttk.Scale(body, from_=1, to=255, variable=self.text_speed_var, orient="horizontal", length=250).grid(
            row=4, column=1, sticky="w"
        )

        ttk.Checkbutton(body, text="Текст слева направо", variable=self.text_direction_ltr).grid(row=5, column=1, sticky="w")

        color_frame = ttk.Frame(body)
        color_frame.grid(row=6, column=0, columnspan=2, sticky="w", pady=8)
        ttk.Button(color_frame, text="Цвет 1", command=lambda: self.pick_color("color1")).pack(side="left")
        ttk.Button(color_frame, text="Цвет 2", command=lambda: self.pick_color("color2")).pack(side="left", padx=4)
        ttk.Button(color_frame, text="Цвет 3", command=lambda: self.pick_color("color3")).pack(side="left", padx=4)
        ttk.Button(color_frame, text="Цвет текста", command=lambda: self.pick_color("text")).pack(side="left", padx=4)

        buttons = ttk.Frame(body)
        buttons.grid(row=7, column=0, columnspan=2, sticky="w", pady=10)
        ttk.Button(buttons, text="Применить", command=self.apply_state).pack(side="left")
        ttk.Button(buttons, text="Выключить", command=self.turn_off).pack(side="left", padx=4)
        ttk.Button(buttons, text="Сохранить пресет", command=self.save_preset).pack(side="left", padx=4)
        ttk.Button(buttons, text="Загрузить пресет", command=self.load_preset).pack(side="left", padx=4)

        ttk.Label(self, textvariable=self.status_var).pack(anchor="w", padx=8, pady=6)

    def base_url(self):
        return f"http://{self.ip_var.get().strip()}"

    def pick_color(self, target):
        color = colorchooser.askcolor()[1]
        if not color:
            return
        if target == "color1":
            self.color1 = color
        elif target == "color2":
            self.color2 = color
        elif target == "color3":
            self.color3 = color
        else:
            self.text_color = color

    def payload(self):
        return {
            "set_mode": self.mode_var.get(),
            "set_brightness": min(self.brightness_var.get(), BRIGHTNESS_LIMIT),
            "set_speed": self.speed_var.get(),
            "set_text": self.text_var.get(),
            "set_text_speed": self.text_speed_var.get(),
            "set_text_direction_ltr": self.text_direction_ltr.get(),
            "set_text_color": self.text_color,
            "set_colors": [self.color1, self.color2, self.color3],
            "save_config": True,
        }

    def apply_state(self):
        try:
            r = requests.post(f"{self.base_url()}/api/apply", json=self.payload(), timeout=3)
            r.raise_for_status()
            self.status_var.set(f"Применено: {r.json().get('mode', '?')}")
        except Exception as e:
            self.status_var.set(f"Ошибка отправки: {e}")

    def turn_off(self):
        try:
            requests.post(f"{self.base_url()}/api/apply", json={"set_mode": "off", "save_config": True}, timeout=3)
            self.status_var.set("Матрица выключена")
        except Exception as e:
            self.status_var.set(f"Ошибка off: {e}")

    def fetch_status(self):
        try:
            r = requests.get(f"{self.base_url()}/api/status", timeout=3)
            r.raise_for_status()
            data = r.json()
            self.mode_var.set(data.get("mode", "color"))
            self.brightness_var.set(min(data.get("brightness", 32), BRIGHTNESS_LIMIT))
            self.speed_var.set(data.get("speed", 80))
            self.text_speed_var.set(data.get("text_speed", 90))
            self.text_var.set(data.get("text", "HELLO"))
            self.text_direction_ltr.set(data.get("text_direction_ltr", False))
            self.status_var.set(f"Подключено: режим {data.get('mode')} | IP {data.get('ip')}")
        except Exception as e:
            self.status_var.set(f"Нет связи: {e}")

    def save_preset(self):
        path = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("JSON", "*.json")])
        if not path:
            return
        Path(path).write_text(json.dumps(self.payload(), ensure_ascii=False, indent=2), encoding="utf-8")
        self.status_var.set(f"Пресет сохранен: {path}")

    def load_preset(self):
        path = filedialog.askopenfilename(filetypes=[("JSON", "*.json")])
        if not path:
            return
        data = json.loads(Path(path).read_text(encoding="utf-8"))
        self.mode_var.set(data.get("set_mode", "color"))
        self.brightness_var.set(min(data.get("set_brightness", 32), BRIGHTNESS_LIMIT))
        self.speed_var.set(data.get("set_speed", 80))
        self.text_var.set(data.get("set_text", "HELLO"))
        self.text_speed_var.set(data.get("set_text_speed", 90))
        self.text_direction_ltr.set(data.get("set_text_direction_ltr", False))
        colors = data.get("set_colors", [self.color1, self.color2, self.color3])
        if len(colors) > 0:
            self.color1 = colors[0]
        if len(colors) > 1:
            self.color2 = colors[1]
        if len(colors) > 2:
            self.color3 = colors[2]
        self.text_color = data.get("set_text_color", self.text_color)
        self.status_var.set("Пресет загружен")

    def auto_discover(self):
        self.status_var.set("Поиск устройств...")

        def worker():
            candidates = []
            try:
                host_ip = socket.gethostbyname(socket.gethostname())
                net = ipaddress.ip_network(host_ip + "/24", strict=False)
            except Exception:
                net = ipaddress.ip_network("192.168.1.0/24", strict=False)
            for ip in net.hosts():
                addr = str(ip)
                try:
                    r = requests.get(f"http://{addr}/api/status", timeout=0.25)
                    if r.ok and "mode" in r.text:
                        candidates.append(addr)
                        break
                except Exception:
                    pass
            if candidates:
                self.ip_var.set(candidates[0])
                self.status_var.set(f"Найдено устройство: {candidates[0]}")
            else:
                self.status_var.set("Устройство не найдено")

        threading.Thread(target=worker, daemon=True).start()


if __name__ == "__main__":
    MatrixApp().mainloop()
