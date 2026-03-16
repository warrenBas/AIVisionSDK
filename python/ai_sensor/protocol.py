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

def pack_packet(msg_id: int, cmd: int, payload: bytes) -> bytes:
    data_len = len(payload)
    # is_first=1 (bit 0), is_last=1 (bit 1), reserve=0 => 0b00000011 = 3
    flags = 3 
    
    #set CRC=0
    header = struct.pack(HEADER_FORMAT, MAGIC, msg_id, 0, data_len, flags, cmd, 0)
    packet = bytearray(header + payload)
    
    # Calculate and fill the CRC (10th byte, index 10).
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