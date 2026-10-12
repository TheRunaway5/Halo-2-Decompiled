"""Distinct loopback OAuth fixture port for concurrent native UI processes."""
import socket


def callback_port():
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return str(listener.getsockname()[1])
