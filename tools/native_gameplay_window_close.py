"""Request the focused private-display game's ordinary X11 window close."""
import ctypes as C


def close_actual_game_window(inputs):
    if not inputs.window:
        raise ValueError('The owned game window must be focused and captured before closing')

    class Data(C.Union):
        _fields_=[('b',C.c_char*20),('s',C.c_short*10),('l',C.c_long*5)]

    class ClientMessage(C.Structure):
        _fields_=[('type',C.c_int),('serial',C.c_ulong),('send_event',C.c_int),
                  ('display',C.c_void_p),('window',C.c_ulong),('message_type',C.c_ulong),
                  ('format',C.c_int),('data',Data)]

    class Event(C.Union):
        _fields_=[('client',ClientMessage),('padding',C.c_long*24)]

    x=inputs.x
    x.XInternAtom.argtypes=[C.c_void_p,C.c_char_p,C.c_int]
    x.XInternAtom.restype=C.c_ulong
    x.XSendEvent.argtypes=[C.c_void_p,C.c_ulong,C.c_int,C.c_long,C.POINTER(Event)]
    x.XSendEvent.restype=C.c_int
    event=Event();event.client.type=33;event.client.send_event=1
    event.client.display=inputs.d;event.client.window=inputs.window;event.client.format=32
    event.client.message_type=x.XInternAtom(inputs.d,b'WM_PROTOCOLS',0)
    event.client.data.l[0]=x.XInternAtom(inputs.d,b'WM_DELETE_WINDOW',0)
    event.client.data.l[1]=0
    if not x.XSendEvent(inputs.d,inputs.window,0,0,C.byref(event)):
        raise RuntimeError('The owned game window rejected its normal close request')
    x.XFlush(inputs.d)
