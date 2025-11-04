import time
import redis
import requests
from receiver import Receiver
from config import Config

class DataPublisher:
    TIME_INTERVAL = 1  # seconds
    def __init__(self):
        self.config = Config()
        self.receiver = Receiver(self.config.SERIAL_PORT, self.config.BAUDRATE)
        self.redis_client = redis.Redis(
            host=self.config.REDIS_HOST,
            port=self.config.REDIS_PORT,
            decode_responses=True
        )
        print(f"port: {self.config.SERIAL_PORT}, baudrate: {self.config.BAUDRATE}, api_url: {self.config.API_BASE_URL}")
        
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
            command = f"{self.config.ADMIN_KEY}-CANSAT_REQ_ID-{launch_id}"
            self.receiver.ser.write((command + '\n').encode('utf-8'))
            print(f"ID {launch_id} enviado al CANSAT")
            return True
        except Exception as e:
            print(f"Error assigning ID to CANSAT: {e}")
            return False
    
    def parse_data(self, raw_data):
        """Parse data from format: admin_key|launch_id|action|timestamp|temp|hum|lat|lon|alt"""
        try:
            if not raw_data or raw_data == "None":
                return None
                
            parts = raw_data.split('-')
            
            if len(parts) < 4:
                return None
                
            admin_key = parts[0]
            launch_id = int(parts[1])
            action = parts[2]
            timestamp = float(parts[3])
            
            parsed_data = {
                'admin_key': admin_key,
                'launch_id': launch_id,
                'action': action,
                'timestamp': timestamp,
                'received_at': time.time()
            }
            
            # Agregar datos de sensores si están disponibles
            if len(parts) > 4 and action == "launch":
                parsed_data['temperature'] = float(parts[4])
                parsed_data['humidity'] = float(parts[5])
                
                # Datos GPS opcionales
                if len(parts) > 7:
                    parsed_data['latitude'] = float(parts[6])
                    parsed_data['longitude'] = float(parts[7])
                    parsed_data['altitude'] = float(parts[8])
            
            return parsed_data
            
        except Exception as e:
            print(f"Error parsing data: {e}")
            return None
    
    def handle_id_request(self):
        """Maneja solicitud de ID del CANSAT"""
        print("Solicitud de ID recibida del CANSAT")
        launch_id = self.request_launch_id()
        if launch_id:
            self.assign_id_to_cansat(launch_id)
            return launch_id
        return None
    
    def publish_data(self):
        """Main loop to process incoming data"""
        current_launch_id = None
        
        while True:
            try:
                # Leer datos del serial
                raw_data = self.receiver.receive_data()
                print(f"Received raw data: {raw_data}")
                
                if raw_data:
                    # Verificar si es solicitud de ID
                    if raw_data == (self.config.ADMIN_KEY + "-CANSAT_REQ_ID"):
                        current_launch_id = self.handle_id_request()
                        continue
                    
                    # Parsear datos normales
                    parsed_data = self.parse_data(raw_data)
                    if parsed_data:
                        # Validar admin key
                        if parsed_data['admin_key'] == self.config.ADMIN_KEY:
                            # Publicar a Redis
                            self.redis_client.publish(
                                self.config.REDIS_CHANNEL, 
                                str(parsed_data)
                            )
                            print(f"Published: {parsed_data}")
                        else:
                            print("Invalid admin key")
                    else:
                        print("Failed to parse data")
                else:
                    # Si no hay datos pero tenemos un launch_id activo, solicitar datos
                    if current_launch_id:
                        request_command = f"{self.config.ADMIN_KEY}|{current_launch_id}|REQUEST-DATA"
                        self.receiver.ask_for_data(request_command)
                
                time.sleep(self.TIME_INTERVAL)
                
            except KeyboardInterrupt:
                print("Stopping publisher...")
                break
            except Exception as e:
                print(f"Error in publish loop: {e}")
                time.sleep(self.TIME_INTERVAL)
    
    def run(self):
        if self.connect():
            self.publish_data()
        self.receiver.close()

if __name__ == "__main__":
    publisher = DataPublisher()
    publisher.run()