"""
labs_frame_bridge.py — Zero-copy frame reader for Labs Engine

Reads raw BGRA frames published by Labs Engine's CvPythonPlugin over Win32 Shared Memory.
Bypasses Desktop Compositor / BetterCam entirely for ultra-low latency capture.
"""
import ctypes
import mmap
import struct
import numpy as np
import cv2
import time

_kernel32 = ctypes.windll.kernel32
_WaitForSingleObject = _kernel32.WaitForSingleObject
_OpenFileMappingW = _kernel32.OpenFileMappingW
_MapViewOfFile = _kernel32.MapViewOfFile
_UnmapViewOfFile = _kernel32.UnmapViewOfFile
_CloseHandle = _kernel32.CloseHandle
_OpenEventW = _kernel32.OpenEventW

_kernel32.OpenFileMappingW.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_wchar_p]
_kernel32.OpenFileMappingW.restype = ctypes.c_void_p
_kernel32.OpenEventW.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_wchar_p]
_kernel32.OpenEventW.restype = ctypes.c_void_p
_kernel32.MapViewOfFile.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_size_t]
_kernel32.MapViewOfFile.restype = ctypes.c_void_p
_kernel32.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
_kernel32.UnmapViewOfFile.restype = ctypes.c_int
_kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
_kernel32.CloseHandle.restype = ctypes.c_int
_kernel32.WaitForSingleObject.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
_kernel32.WaitForSingleObject.restype = ctypes.c_uint32

FILE_MAP_READ = 4
SYNCHRONIZE = 0x00100000
WAIT_OBJECT_0 = 0
WAIT_TIMEOUT = 258

_MAGIC = 0x4D52464C  # "FRML"
_HEADER_SIZE = 64
_MAX_PAYLOAD = 1920 * 1080 * 4
_BLOCK_SIZE = _HEADER_SIZE + _MAX_PAYLOAD

# Header Layout: <5I = magic, version, writerPid, sequence, payload_size
#               <4I = width, height, stride, format
#               <i = sessionId
#               <q = timestampUs
_FMT_HDR = "<IIIIIIIIiiq"

class ShmFrameReader:
    def __init__(self, labs_pid: int, session_id: int):
        self.labs_pid = labs_pid
        self.session_id = session_id
        
        self.block_name = f"Labs_{labs_pid}_Frame_{session_id}"
        self.event_name = f"Global\\Labs_{labs_pid}_Frame_{session_id}_Written"
        self.local_event_name = f"Labs_{labs_pid}_Frame_{session_id}_Written"

        self.mapping = None
        self.event = None
        self.view_ptr = None
        
        # We'll use mmap module to wrap the memory view
        self.mm = None
        self.last_seq = 0
        
        self._connect()
        
    def _connect(self):
        # Open Event
        self.event = _OpenEventW(SYNCHRONIZE, 0, self.event_name)
        if not self.event:
            self.event = _OpenEventW(SYNCHRONIZE, 0, self.local_event_name)
            
        # Open Mapping
        try:
            self.mm = mmap.mmap(-1, _BLOCK_SIZE, tagname=self.block_name, access=mmap.ACCESS_READ)
        except Exception as e:
            print(f"[SHM] mmap open failed: {e}")
            self.mm = None

    def get_latest_frame(self, timeout_ms=50):
        if not self.mm:
            self._connect()
            if not self.mm:
                return None
        
        if self.event:
            res = _WaitForSingleObject(self.event, timeout_ms)
            if res != WAIT_OBJECT_0:
                return None # Timeout or error
        else:
            # Fallback busy wait if event handle failed to open
            time.sleep(0.005)

        self.mm.seek(0)
        header_data = self.mm.read(_HEADER_SIZE)
        
        try:
            unpacked = struct.unpack(_FMT_HDR, header_data[:48])
        except struct.error:
            return None
            
        magic, version, pid, seq, payload_size, width, height, stride, fmt, sid, ts = unpacked
        
        if magic != _MAGIC:
            return None
            
        if seq == self.last_seq:
            return None
            
        self.last_seq = seq
        
        if payload_size == 0 or payload_size > _MAX_PAYLOAD or width == 0 or height == 0:
            return None

        # Read payload
        self.mm.seek(_HEADER_SIZE)
        payload = self.mm.read(payload_size)
        
        # Format 0 is BGRA8
        # Use cv2.cvtColor for a contiguous BGR array — numpy slicing
        # creates non-contiguous views that kill OpenCV's fast SIMD path.
        bgra = np.frombuffer(payload, dtype=np.uint8).reshape((height, stride // 4, 4))
        bgra = bgra[:height, :width]
        return cv2.cvtColor(bgra, cv2.COLOR_BGRA2BGR)

    def close(self):
        if self.mm:
            self.mm.close()
            self.mm = None
        if self.event:
            _CloseHandle(self.event)
            self.event = None
