import socket
import struct
import time

SERVER_IP = "127.0.0.1"
SERVER_PORT = 9000

def send_dummy_packets():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sequence = 1
    print(f"Sending dummy UDP packets to {SERVER_IP}:{SERVER_PORT}... (Press Ctrl+C to stop)")
    try:
        while True:
            # PacketHeader: size(uint16), opcode(uint16), sequence(uint32)
            size = 8
            opcode = 1 # dummy opcode
            packet = struct.pack('<HHL', size, opcode, sequence)
            
            sock.sendto(packet, (SERVER_IP, SERVER_PORT))
            sequence += 1
            
            # Send roughly 100 packets per second
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("\nStopped.")

if __name__ == "__main__":
    send_dummy_packets()
