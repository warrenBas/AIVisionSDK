import struct

#header struct: magic(2), msg_id(2), fragment_id(2), data_len(2), flags(1), cmd(1), crc(1)
HEADER_FORMAT = "<HHHHBBB"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
MAGIC = 0xAA88

def calc_crc(data_bytes: bytearray) -> int:
    crc = 0
    for byte in data_bytes:
        crc ^= byte
    return crc

def pack_packet(msg_id: int, cmd: int, payload: bytes, fragment_id: int, is_first: int, is_last: int) -> bytes:
    data_len = len(payload)
    
    flags = (is_first & 0x01) | ((is_last & 0x01) << 1)
    
    header = struct.pack(HEADER_FORMAT, MAGIC, msg_id, fragment_id, data_len, flags, cmd, 0)
    packet = bytearray(header + payload)
    crc_val = calc_crc(packet)
    packet[10] = crc_val 
    
    return bytes(packet)

def unpack_header(data: bytes):
    if len(data) < HEADER_SIZE:
        return None
    magic, msg_id, frag_id, data_len, flags, cmd, crc = struct.unpack(HEADER_FORMAT, data[:HEADER_SIZE])
    return {
        "magic": magic, "msg_id": msg_id, "frag_id": frag_id,
        "data_len": data_len, "flags": flags, "cmd": cmd, "crc": crc
    }