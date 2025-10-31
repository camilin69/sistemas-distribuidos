import time
import redis
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
    
    def parse_data(self, raw_data):
        """Parse data from format: [HEADER][TIMESTAMP][TEMP][HUM]"""
        try:
            if not raw_data or raw_data == "None":
                return None
                
            # Remove brackets and split by commas
            data_clean = raw_data.strip('[]')
            parts = data_clean.split('][')
            
            if len(parts) < 4:
                return None
                
            header = parts[0]
            timestamp = float(parts[1])
            temperature = float(parts[2])
            humidity = float(parts[3])
            
            # Parse header: admin_key-launch_id-action
            header_parts = header.split('-')
            if len(header_parts) < 3:
                return None
                
            admin_key = header_parts[0]
            launch_id = int(header_parts[1])
            action = header_parts[2]
            
            return {
                'admin_key': admin_key,
                'launch_id': launch_id,
                'action': action,
                'timestamp': timestamp,
                'temperature': temperature,
                'humidity': humidity,
                'received_at': time.time()
            }
        except Exception as e:
            print(f"Error parsing data: {e}")
            return None
    
    def publish_data(self):
        """Main loop to request and publish data every second"""
        while True:
            try:
                # Request data from LoRa device
                raw_data = self.receiver.ask_for_data(Config.ADMIN_KEY + '-REQUEST-DATA')
                
                if raw_data:
                    parsed_data = self.parse_data(raw_data)
                    
                    if parsed_data:
                        # Validate admin key
                        if parsed_data['admin_key'] == self.config.ADMIN_KEY:
                            # Publish to Redis
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
                    print("No data received")
                
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