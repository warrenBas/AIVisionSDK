import sys

# Automatic environment recognition
if sys.implementation.name == 'micropython':
    from ai_sensor.mpy_uart import UartAdapter
else:
    try:
        from ai_sensor.py_uart import UartAdapter
    except (ImportError, ModuleNotFoundError) as e:             # Ensure exports are available even when pyserial isn't installed.
        # The class will raise an informative error if used.
        print("Warning: pyserial not installed. PC mode may not work.")
        raise e
    
from ai_sensor.core import AISensor

__all__ = ['AISensor', 'UartAdapter']