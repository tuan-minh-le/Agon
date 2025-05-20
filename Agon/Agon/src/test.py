import socket
import pyaudio
import threading
import sys

# Configuration audio
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 44100
CHUNK = 1024

class VoiceChatClient:
    def __init__(self, server_ip, server_port):
        self.server_ip = server_ip
        self.server_port = server_port

        self.audio = pyaudio.PyAudio()

        self.play_stream = self.audio.open(format=FORMAT,
                                           channels=CHANNELS,
                                           rate=RATE,
                                           output=True,
                                           frames_per_buffer=CHUNK)

        self.record_stream = self.audio.open(format=FORMAT,
                                             channels=CHANNELS,
                                             rate=RATE,
                                             input=True,
                                             frames_per_buffer=CHUNK)

        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.running = True

    def send_audio(self):
        while self.running:
            try:
                data = self.record_stream.read(CHUNK, exception_on_overflow=False)
                self.client_socket.sendall(data)
            except Exception as e:
                print(f"Erreur envoi audio : {e}")
                break

    def receive_audio(self):
        while self.running:
            try:
                data = self.client_socket.recv(CHUNK * 4)
                if not data:
                    break
                self.play_stream.write(data)
            except Exception as e:
                print(f"Erreur réception audio : {e}")
                break

    def start(self, room_id="default"):
        self.client_socket.connect((self.server_ip, self.server_port))
        self.client_socket.sendall(room_id.encode())  # Envoie le room ID

        print(f"Connecté au serveur vocal {self.server_ip}:{self.server_port}, room '{room_id}'")

        send_thread = threading.Thread(target=self.send_audio)
        receive_thread = threading.Thread(target=self.receive_audio)
        send_thread.start()
        receive_thread.start()

        try:
            while True:
                pass
        except KeyboardInterrupt:
            print("\nDéconnexion...")
            self.running = False
            self.client_socket.close()
            self.record_stream.stop_stream()
            self.record_stream.close()
            self.play_stream.stop_stream()
            self.play_stream.close()
            self.audio.terminate()

if __name__ == "__main__":
    room_id = str(sys.argv[1]) if len(sys.argv) > 1 else "default"
    ip_serveur = "10.42.234.253"
    port_serveur = 12345

    client = VoiceChatClient(ip_serveur, port_serveur)
    client.start(room_id)
