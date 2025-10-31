import redis
import pymongo
import json
import time
from datetime import datetime
from config import Config

class DataSubscriber:
    def __init__(self):
        self.config = Config()
        self.redis_client = None
        self.mongo_client = None
        self.db = None
        self.collection = None
        
        # Track active launches
        self.active_launches = {}
        
    def connect_databases(self):
        """Connect to Redis and MongoDB"""
        try:
            # Connect to Redis
            self.redis_client = redis.Redis(
                host=self.config.REDIS_HOST,
                port=self.config.REDIS_PORT,
                decode_responses=True
            )
            self.redis_client.ping()
            print("Connected to Redis successfully")
            
            # Connect to MongoDB (sin autenticación para desarrollo)
            self.mongo_client = pymongo.MongoClient(
                self.config.MONGODB_URI,
                username=self.config.MONGO_INITDB_ROOT_USERNAME,
                password=self.config.MONGO_INITDB_ROOT_PASSWORD,
                authSource='admin'
            )
            
            # Verificar conexión y crear base de datos/colección si no existen
            self.db = self.mongo_client[self.config.MONGODB_DB]
            self.collection = self.db[self.config.MONGODB_COLLECTION]
            
            # Crear un índice para mejor performance
            self.collection.create_index("launch_id")
            
            print("Connected to MongoDB successfully")
            print(f"Database '{self.config.MONGODB_DB}' and collection '{self.config.MONGODB_COLLECTION}' are ready")
            
            return True
            
        except Exception as e:
            print(f"Database connection error: {e}")
            return False
    
    def format_timestamp(self, timestamp):
        """Convert timestamp to DD/MM/YY_HH:MM:SS format"""
        dt = datetime.fromtimestamp(timestamp)
        return dt.strftime("%d/%m/%y_%H:%M:%S")
    
    def handle_launch_start(self, data):
        """Handle launch start action"""
        launch_id = data['launch_id']
        
        if launch_id not in self.active_launches:
            self.active_launches[launch_id] = {
                'launch_id': launch_id,
                'start_date': self.format_timestamp(data['received_at']),
                'end_date': None,
                'variables': []
            }
            print(f"Launch {launch_id} started")
    
    def handle_launch_end(self, data):
        """Handle launch end action"""
        launch_id = data['launch_id']
        
        if launch_id in self.active_launches:
            self.active_launches[launch_id]['end_date'] = self.format_timestamp(data['received_at'])
            
            # Save completed launch to MongoDB
            self.save_launch_to_db(self.active_launches[launch_id])
            print(f"Launch {launch_id} ended and saved to database")
            
            # Remove from active launches
            del self.active_launches[launch_id]
    
    def handle_data_point(self, data):
        """Handle regular data point"""
        launch_id = data['launch_id']
        
        if launch_id in self.active_launches:
            variable_data = {
                'timestamp': data['timestamp'],
                'temperature': data['temperature'],
                'humidity': data['humidity']
            }
            
            self.active_launches[launch_id]['variables'].append(variable_data)
            print(f"Added data point to launch {launch_id}")
    
    def save_launch_to_db(self, launch_data):
        """Save launch data to MongoDB"""
        try:
            # Convert to the required schema
            db_document = {
                'launch_id': launch_data['launch_id'],
                'start_date': launch_data['start_date'],
                'end_date': launch_data['end_date'],
                'variables': launch_data['variables']
            }
            
            # Update existing or insert new
            self.collection.update_one(
                {'launch_id': launch_data['launch_id']},
                {'$set': db_document},
                upsert=True
            )
            
        except Exception as e:
            print(f"Error saving to MongoDB: {e}")
    
    def process_message(self, message):
        """Process incoming Redis message"""
        try:
            data = json.loads(message['data'])
            
            # Validate admin key
            if data.get('admin_key') != self.config.ADMIN_KEY:
                print("Invalid admin key in message")
                return
            
            action = data.get('action', '').lower()
            
            if action == 'start':
                self.handle_launch_start(data)
            elif action == 'end':
                self.handle_launch_end(data)
            elif action == 'falling':
                self.handle_data_point(data)
            else:
                print(f"Unknown action: {action}")
                
        except json.JSONDecodeError:
            print("Invalid JSON message")
        except Exception as e:
            print(f"Error processing message: {e}")
    
    def subscribe(self):
        """Subscribe to Redis channel and process messages"""
        pubsub = self.redis_client.pubsub()
        pubsub.subscribe(self.config.REDIS_CHANNEL)
        
        print("Subscribed to Redis channel. Waiting for messages...")
        
        try:
            for message in pubsub.listen():
                if message['type'] == 'message':
                    self.process_message(message)
        except KeyboardInterrupt:
            print("Stopping subscriber...")
        except Exception as e:
            print(f"Error in subscription: {e}")
        finally:
            pubsub.close()
    
    def run(self):
        if self.connect_databases():
            self.subscribe()
        if self.mongo_client:
            self.mongo_client.close()

if __name__ == "__main__":
    subscriber = DataSubscriber()
    subscriber.run()