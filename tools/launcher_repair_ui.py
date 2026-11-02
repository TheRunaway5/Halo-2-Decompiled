"""An owned repair child with live progress; completion never launches the game."""
import queue

from setup_assets_ui import SetupProcess


class LauncherRepairWindow:
    def __init__(self, parent):
        self.parent, self.closed, self.finished = parent, False, False
        self.events = queue.Queue()
        arguments = ('--runtime', str(parent.runtime_executable)) if parent.runtime_executable else ()
        self.process = SetupProcess(parent.root, self.events, tool='repair_launcher.py',
                                    extra_arguments=arguments)
        tk = parent.tk
        self.window = tk.Toplevel(parent.window)
        window = self.window
        window.title('Forever Launcher — Repair')
        window.transient(parent.window)
        window.configure(background='#06101c', padx=24, pady=24)
        window.resizable(False, False)
        tk.Label(window, text='Repair game files', background='#06101c', foreground='#c8edff',
                 font=parent.font(22), anchor='w').pack(fill='x', pady=(0, 18))
        self.status = tk.Label(window, text='Checking the installed game and imported MCC files…',
                               background='#06101c', foreground='#9baec3', font=parent.font(13),
                               anchor='nw', justify='left', wraplength=parent.px(520), width=60, height=6)
        self.status.pack(fill='x', pady=(0, 12))
        self.bar = parent.progressbar(window)
        self.bar.configure(height=parent.px(6))
        self.bar.pack(fill='x', pady=(0, 20))
        self.bar.start()
        self.back = tk.Button(window, text='Cancel', command=self.close, font=parent.font(13),
                              background='#10273c', foreground='#c8edff', activebackground='#1c3c56',
                              activeforeground='#c8edff', relief='flat', padx=20, pady=8)
        self.back.pack(anchor='e')
        self.choose = tk.Button(window, text='Choose MCC folder…', command=self.choose_mcc,
                                font=parent.font(12), background='#10273c', foreground='#c8edff',
                                relief='flat', padx=12, pady=8)
        window.protocol('WM_DELETE_WINDOW', self.close)
        window.bind('<Escape>', lambda event: self.close())
        window.grab_set()
        self.poll_id = None
        try:
            self.process.start(None)
        except Exception as error:
            self.events.put(('error', str(error)))
        self.poll_id = window.after(50, self.poll)

    def poll(self):
        from setup_progress import parse_update

        self.poll_id = None
        if self.closed:
            return
        self.process.poll()
        while not self.events.empty():
            kind, value = self.events.get_nowait()
            if kind == 'status':
                update = parse_update(value)
                if update is not None:
                    self.bar.stop()
                    self.bar.configure(mode='determinate', value=update.percent)
                else:
                    self.bar.configure(mode='indeterminate')
                    self.bar.start()
                self.status.configure(text=str(value)[:900])
            elif kind == 'done':
                self.finished = True
                self.close()
                return
            elif kind == 'error':
                self.bar.stop()
                detail = str(value)
                at = detail.rfind('Repair failed: ')
                detail = detail[at:at+900] if at >= 0 else detail[-900:]
                self.status.configure(text=detail, foreground='#e7ba8f')
                self.back.configure(text='Back')
                self.choose.pack(anchor='w', before=self.back)
                return
        self.poll_id = self.window.after(50, self.poll)

    def choose_mcc(self):
        from tkinter import filedialog
        selected = filedialog.askdirectory(parent=self.window, title='Select your MCC installation')
        if not selected or self.closed:
            return
        self.choose.pack_forget()
        self.back.configure(text='Cancel')
        self.status.configure(text='Checking the selected MCC installation…', foreground='#9baec3')
        self.bar.configure(mode='indeterminate')
        self.bar.start()
        try:
            self.process.start(selected)
        except Exception as error:
            self.events.put(('error', str(error)))
        self.poll_id = self.window.after(50, self.poll)

    def close(self, *, recheck=True):
        if self.closed:
            return
        self.closed = True
        if self.poll_id is not None:
            self.window.after_cancel(self.poll_id)
        try:
            self.process.shutdown()
        finally:
            self.bar.stop()
            self.window.destroy()
            self.parent.repair_dialog = None
            if recheck and not self.parent.closed:
                self.parent.check_readiness()
