"""Locate the setup bundle's explicitly owned Steam runtime for this CPU."""
import os
import contextlib
from pathlib import Path
import platform
import sys


def helper_path(root):
    if sys.platform != 'darwin':
        return None
    version = platform.mac_ver()[0].split('.')[0]
    if not version.isdigit() or int(version) < 15:
        return None  # The private .NET 10 feature supports macOS 15 and newer.
    from installation_paths import resource_root
    arch = {'arm64': 'arm64', 'x86_64': 'x86_64'}.get(platform.machine())
    if arch is None:
        return None
    helper = resource_root(root) / 'steam' / arch / 'Halo2Steam'
    if helper.is_file() and not helper.is_symlink() and os.access(helper, os.X_OK):
        return helper
    return None


class SteamSetupWindow:
    """Transient QR/plan view; the parent drives polling on Tk's main thread."""
    def __init__(self, parent, helper, *, process_factory=None):
        from steam_session import SteamDownloadProcess
        self.parent, self.image, self.closed = parent, None, False
        self.process = (process_factory or SteamDownloadProcess)(parent.root, helper, parent.events)
        tk = parent.tk
        self.window = tk.Toplevel(parent.window)
        window = self.window
        window.title('Download MCC from Steam')
        window.configure(background='#061522')
        window.geometry(f'{parent.px(600)}x{parent.px(650)}')
        window.resizable(False, False)
        window.transient(parent.window)
        self.title = tk.Label(window, text='Download your MCC copy', background='#061522', foreground='#e4f5fb',
                              font=parent.font(24, bold=True))
        self.title.pack(padx=parent.px(22), pady=(parent.px(22), parent.px(12)))
        tk.Label(window, text='Approve sign-in with Steam Mobile.\nMCC and its Halo 2 DLC are required.\nSign-in lasts for this setup session.',
                 background='#061522', foreground='#94b5c8', font=parent.font(14), justify='center').pack()
        self.qr = tk.Label(window, text='Connecting to Steam…', background='#061522', foreground='#8fe4f5',
                           font=parent.font(15), wraplength=parent.px(520))
        self.qr.pack(padx=parent.px(22), pady=parent.px(16), fill='both', expand=True)
        self.status = tk.Label(window, text='Preparing a sign-in request…', background='#0b2032', foreground='#e4f5fb',
                               font=parent.font(14), justify='left', anchor='nw', wraplength=parent.px(520),
                               padx=parent.px(14), pady=parent.px(12))
        self.status.pack(padx=parent.px(22), fill='x')
        buttons = tk.Frame(window, background='#061522')
        buttons.pack(padx=parent.px(22), pady=parent.px(22), fill='x')
        self.cancel_button = tk.Button(buttons, text='Cancel', command=self.cancel, font=parent.font(14),
                                       background='#16364c', foreground='#e4f5fb', relief='flat')
        self.cancel_button.pack(side='left', ipadx=parent.px(18), ipady=parent.px(8))
        self.confirm = tk.Button(buttons, text='Download and prepare', command=self.download, state='disabled',
                                 font=parent.font(14), background='#a6e4ef', foreground='#092335', relief='flat')
        self.confirm.pack(side='right', ipadx=parent.px(18), ipady=parent.px(8))
        window.protocol('WM_DELETE_WINDOW', self.cancel)
        window.bind('<Escape>', lambda event: self.cancel())
        window.grab_set()
        self.cancel_button.focus_set()
        try:
            self.process.start()
        except BaseException:
            self.finish()
            raise

    def poll(self):
        self.process.poll()

    def clear_qr(self, text):
        self.qr.configure(image='', text=text)
        self.image = None

    def event(self, kind, value):
        if self.closed: return
        if kind == 'steam-finished':
            # poll() has already reaped the helper and reader before handing
            # this source to the existing CLI preparation process.
            self.finish()
            self.parent.value.set(value['source'])
            self.parent.start()
        elif kind == 'steam-error':
            self.clear_qr('Steam setup stopped')
            self.status.configure(text=value, foreground='#ffd29b')
            self.confirm.configure(state='disabled')
            self.cancel_button.configure(text='Back')
        elif value['event'] == 'qr':
            image = self.parent.tk.PhotoImage(master=self.window, data=value['png'], format='png')
            maximum = self.parent.px(300)
            divisor = max(1, (image.width() + maximum - 1) // maximum)
            self.image = image.subsample(divisor) if divisor > 1 else image
            self.qr.configure(image=self.image, text='')
            self.status.configure(text='Scan this code using Steam Mobile, then approve the sign-in.', foreground='#e4f5fb')
        elif value['event'] == 'ownership_verified':
            self.clear_qr('MCC and Halo 2 access confirmed')
            self.status.configure(text='Checking required files and available disk space…')
        elif value['event'] == 'plan':
            gib = lambda number: f'{number / (1024**3):,.1f} GiB'
            enough = value['available_free_bytes'] >= value['required_free_bytes']
            self.clear_qr('Ready to download' if enough else 'More disk space needed')
            self.status.configure(text=(f"{value['files']:,} selected files · {gib(value['download_bytes'])} download\n"
                f"{gib(value['bytes'])} of source content\n"
                f"{gib(value['required_free_bytes'])} free space required for download and preparation\n"
                f"{gib(value['available_free_bytes'])} available"), foreground='#e4f5fb' if enough else '#ffd29b')
            self.confirm.configure(state='normal' if enough else 'disabled')
        elif value['event'] == 'progress':
            total = value['total']
            percentage = min(100, 100 * value['bytes'] / total) if total else 0
            self.status.configure(text=f'Downloading MCC Halo 2 content… {percentage:.1f}%\nCancel keeps partial files for a later resume.')
        elif value['event'] == 'verifying':
            self.clear_qr('Verifying downloaded files')
            self.status.configure(text='Checking each selected file against its Steam manifest before preparation…')

    def download(self):
        try:
            self.process.download()
            self.confirm.configure(state='disabled')
            self.clear_qr('Downloading MCC Halo 2 content')
            self.status.configure(text='Download started. Cancel keeps partial files for a later resume.')
        except RuntimeError as error:
            self.status.configure(text=str(error), foreground='#ffd29b')

    def finish(self):
        if self.closed: return
        self.process.shutdown()
        self.closed = True
        self.image = None
        with contextlib.suppress(self.parent.tk.TclError):
            self.window.grab_release()
            self.window.destroy()
        self.parent.steam = None
        self.parent._busy(False)

    def cancel(self):
        self.finish()
        self.parent.status_label.configure(text='Steam setup closed. Choose a local MCC folder or sign in again to resume.', foreground='#94b5c8')
