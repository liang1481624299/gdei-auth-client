# -*- coding: utf-8 -*-
"""
GDEI 校园网自动认证 - 图形界面（仅依赖 Python 标准库 tkinter）。

运行:
    python campus_gui.py
或双击 login.bat。
账号密码保存在同目录 config.json（口令经可逆混淆，非明文）。
"""

import json
import os
import threading
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext

import campus_core as core


class App:
    def __init__(self, root: tk.Tk):
        self.root = root
        root.title("广东第二师范学院网络客户端")
        root.resizable(False, False)
        self.cfg = core.load_config()

        pad = {"padx": 10, "pady": 6}
        frm = ttk.Frame(root)
        frm.grid(row=0, column=0, sticky="nsew", **pad)

        # 账号
        ttk.Label(frm, text="账号：").grid(row=0, column=0, sticky="e", pady=4)
        self.var_user = tk.StringVar(value=self.cfg.get("username", ""))
        ttk.Entry(frm, textvariable=self.var_user, width=28).grid(row=0, column=1, columnspan=2, pady=4)

        # 密码
        ttk.Label(frm, text="密码：").grid(row=1, column=0, sticky="e", pady=4)
        self.var_pwd = tk.StringVar(value=core.get_password(self.cfg))
        self.var_show = tk.BooleanVar(value=False)
        self.pwd_entry = ttk.Entry(frm, textvariable=self.var_pwd, width=28, show="*")
        self.pwd_entry.grid(row=1, column=1, columnspan=2, pady=4)
        ttk.Checkbutton(frm, text="显示密码", variable=self.var_show,
                        command=self._toggle_pwd).grid(row=1, column=3, pady=4)

        self.var_save = tk.BooleanVar(value=bool(self.cfg.get("password_ob")))
        ttk.Checkbutton(frm, text="记住账号密码", variable=self.var_save).grid(
            row=2, column=1, sticky="w", pady=2)

        # 高级设置
        self.adv = ttk.LabelFrame(frm, text="高级设置（一般无需修改）")
        self.adv.grid(row=3, column=0, columnspan=4, sticky="nsew", **pad)
        ttk.Label(self.adv, text="门户主机").grid(row=0, column=0, sticky="e", padx=4, pady=3)
        self.var_host = tk.StringVar(value=self.cfg.get("host", core.DEFAULT_HOST))
        ttk.Entry(self.adv, textvariable=self.var_host, width=22).grid(row=0, column=1, padx=4)
        ttk.Label(self.adv, text="备用IP").grid(row=0, column=2, sticky="e", padx=4)
        self.var_ip = tk.StringVar(value=self.cfg.get("ip", core.DEFAULT_IP))
        ttk.Entry(self.adv, textvariable=self.var_ip, width=16).grid(row=0, column=3, padx=4)
        ttk.Label(self.adv, text="NAS IP").grid(row=1, column=0, sticky="e", padx=4, pady=3)
        self.var_nasip = tk.StringVar(value=self.cfg.get("nasip", core.DEFAULT_NASIP))
        ttk.Entry(self.adv, textvariable=self.var_nasip, width=22).grid(row=1, column=1, padx=4)

        # 按钮
        btns = ttk.Frame(frm)
        btns.grid(row=4, column=0, columnspan=4, pady=8)
        ttk.Button(btns, text="检测状态", command=self.on_status).pack(side="left", padx=6)
        ttk.Button(btns, text="上线", command=self.on_login).pack(side="left", padx=6)
        ttk.Button(btns, text="下线", command=self.on_logout).pack(side="left", padx=6)
        ttk.Button(btns, text="打开门户", command=self.open_portal).pack(side="left", padx=6)

        # 日志
        self.log = scrolledtext.ScrolledText(frm, width=62, height=12, state="disabled",
                                             font=("Consolas", 9))
        self.log.grid(row=5, column=0, columnspan=4, **pad)

        self.status = ttk.Label(frm, text="状态：未知", foreground="#555")
        self.status.grid(row=6, column=0, columnspan=4, sticky="w", padx=10)

        self._refresh_status()

    def _toggle_pwd(self):
        self.pwd_entry.configure(show="" if self.var_show.get() else "*")

    # ---------------------------------------------------------- #
    def _log(self, msg: str):
        self.log.configure(state="normal")
        self.log.insert("end", msg + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def _collect_config(self) -> dict:
        cfg = dict(self.cfg)
        cfg["username"] = self.var_user.get().strip()
        cfg["host"] = self.var_host.get().strip() or core.DEFAULT_HOST
        cfg["ip"] = self.var_ip.get().strip() or core.DEFAULT_IP
        cfg["nasip"] = self.var_nasip.get().strip() or core.DEFAULT_NASIP
        if self.var_save.get():
            cfg["password_ob"] = core.obfuscate(self.var_pwd.get())
        else:
            cfg["password_ob"] = ""
        return cfg

    def _save(self):
        self.cfg = self._collect_config()
        core.save_config(self.cfg)

    def _run(self, fn, label: str):
        self._log(f"—— {label} ——")

        def worker():
            try:
                cfg = self._collect_config()
                if self.var_save.get():
                    core.save_config(cfg)
                    self.cfg = cfg
                ok, msg = fn(cfg, log=self._log)
                self.root.after(0, lambda: self._after_action(ok, msg))
            except Exception as e:
                self.root.after(0, lambda: self._log(f"{label}异常：{e}"))

        threading.Thread(target=worker, daemon=True).start()

    def _after_action(self, ok: bool, msg: str):
        self._log(msg)
        self._refresh_status()
        if not ok:
            messagebox.showwarning("提示", msg)

    def on_status(self):
        self._run(lambda cfg, log: (core.is_online(), "在线" if core.is_online() else "未上线（需要认证）"), "检测状态")

    def on_login(self):
        if not self.var_user.get().strip() or not self.var_pwd.get():
            messagebox.showinfo("提示", "请输入账号和密码。")
            return
        self._run(core.login, "上线")

    def on_logout(self):
        self._run(core.logout, "下线")

    def open_portal(self):
        import webbrowser
        url = f"https://{self.var_host.get()}/portal?nasip={self.var_nasip.get()}"
        webbrowser.open(url)

    def _refresh_status(self):
        online = core.is_online()
        self.status.configure(text="状态：" + ("已联网" if online else "未上线（需要认证）"),
                              foreground="#1a7f37" if online else "#c0392b")


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
