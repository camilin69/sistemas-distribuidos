import time
import redis
import requests
from receiver import Receiver
from config import Config

class DataPublisherTX:
    TIME_INTERVAL = 1  # seconds
    
    def __init__(self):
        self.config = Config()
        self.receiver = Receiver(self.config.SERIAL_PORT_TX, self.config.BAUDRATE)
        self.redis_client = redis.Redis(
            host=self.config.REDIS_HOST,
            port=self.config.REDIS_PORT,
            decode_responses=True
        )
        print(f"TX - Port: {self.config.SERIAL_PORT_TX}, Baudrate: {self.config.BAUDRATE}")
        
    def connect(self):
        if not self.receiver.connect():
            print("Failed to connect to serial port")
            return False
        
        try:
            self.redis_client.ping()
            print("Connected to Redis successfully")
            return True
        except redis.ConnectionError:
            print("Failed to connect to Redis")
            return False
    
    def request_launch_id(self):
        """Solicita un nuevo ID de lanzamiento a la API"""
        try:
            response = requests.post(f"{self.config.API_BASE_URL}/cansat_req_id")
            if response.status_code == 200:
                data = response.json()
                return data.get('assigned_id')
            else:
                print(f"API error: {response.status_code}")
                return None
        except Exception as e:
            print(f"Error requesting launch ID: {e}")
            return None
    
    def assign_id_to_cansat(self, launch_id):
        """Envía el ID asignado al CANSAT via serial"""
        try:
            self.receiver.ser.reset_input_buffer()
            time.sleep(0.1)
            
            command = f"{self.config.ADMIN_KEY}-CANSAT_REQ_ID-{launch_id}\n"
            self.receiver.ser.write(command.encode('utf-8'))
            self.receiver.ser.flush()
            
            print(f"ID {launch_id} enviado al CANSAT")
            return True
        except Exception as e:
            print(f"Error assigning ID to CANSAT: {e}")
            return False
    
    def handle_id_request(self):
        """Maneja solicitud de ID del CANSAT"""
        print("Solicitud de ID recibida del CANSAT")
        launch_id = self.request_launch_id()
        if launch_id:
            success = self.assign_id_to_cansat(launch_id)
            if success:
                return launch_id
        return None
    
    def publish_data(self):
        """Main loop para TX - solo maneja solicitudes de ID"""
        while True:
            try:
                # Leer datos del serial
                raw_data = self.receiver.receive_data()
                
                if raw_data:
                    print(f"TX Received: {raw_data}")
                    
                    # Verificar si es solicitud de ID
                    if raw_data.strip() == (self.config.ADMIN_KEY + "-CANSAT_REQ_ID"):
                        self.handle_id_request()
                
                
            except KeyboardInterrupt:
                print("Stopping TX publisher...")
                break
            except Exception as e:
                print(f"Error in TX publish loop: {e}")
                time.sleep(self.TIME_INTERVAL)
    
    def run(self):
        if self.connect():
            self.publish_data()
        self.receiver.close()

if __name__ == "__main__":
    publisher = DataPublisherTX()
    publisher.run()