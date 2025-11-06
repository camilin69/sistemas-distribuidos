import redis
import pymongo
import json
import time
import ast
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
            print("✅ Connected to Redis successfully")
            
            # Connect to MongoDB
            self.mongo_client = pymongo.MongoClient(
                self.config.MONGODB_URI,
                username=self.config.MONGO_INITDB_ROOT_USERNAME,
                password=self.config.MONGO_INITDB_ROOT_PASSWORD,
                authSource='admin'
            )
            
            self.db = self.mongo_client[self.config.MONGODB_DB]
            self.collection = self.db[self.config.MONGODB_COLLECTION]
            
            # Crear índices
            self.collection.create_index("launch_id")
            self.collection.create_index([("launch_id", 1), ("timestamp", 1)])
            
            print("✅ Connected to MongoDB successfully")
            print(f"📊 Database: {self.config.MONGODB_DB}, Collection: {self.config.MONGODB_COLLECTION}")
            print(f"🔑 Admin key: '{self.config.ADMIN_KEY}'")
            
            return True
            
        except Exception as e:
            print(f"❌ Database connection error: {e}")
            return False
    
    def safe_parse_dict(self, message_str):
        """Safely parse dictionary string handling single quotes"""
        try:
            # Reemplazar comillas simples por dobles para JSON válido
            json_str = message_str.replace("'", '"')
            return json.loads(json_str)
        except Exception as e:
            print(f"❌ JSON parse error: {e}")
            return None
    
    def parse_raw_data(self, raw_data):
        """Parse raw data from format: admin_key-launch_id-action-timestamp-temp-humidity-lat-lon-alt"""
        try:
            print(f"🔄 Parsing raw data: {raw_data}")
            
            if not raw_data or raw_data == "None":
                return None
            
            # Limpiar el string (remover espacios y caracteres extraños)
            clean_data = raw_data.strip()
            
            # Dividir por guiones
            parts = clean_data.split('-')
            print(f"📋 Parts: {parts}")
            
            # Validar formato mínimo
            if len(parts) < 4:
                print(f"❌ Invalid format: expected at least 4 parts, got {len(parts)}")
                return None
            
            # Extraer datos básicos
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
            
            # Extraer datos de sensores si están disponibles
            if len(parts) > 4 and parts[4]:  # temperature
                try:
                    parsed_data['temperature'] = float(parts[4])
                except (ValueError, IndexError):
                    parsed_data['temperature'] = None
            
            if len(parts) > 5 and parts[5]:  # humidity
                try:
                    parsed_data['humidity'] = float(parts[5])
                except (ValueError, IndexError):
                    parsed_data['humidity'] = None
            
            # Extraer datos GPS si están disponibles
            if len(parts) > 8:  # lat, lon, alt
                try:
                    parsed_data['latitude'] = float(parts[6])
                    parsed_data['longitude'] = float(parts[7])
                    parsed_data['altitude'] = float(parts[8])
                except (ValueError, IndexError):
                    parsed_data['latitude'] = None
                    parsed_data['longitude'] = None
                    parsed_data['altitude'] = None
            
            print(f"✅ Parsed data: {parsed_data}")
            return parsed_data
            
        except Exception as e:
            print(f"❌ Error parsing raw data: {e}")
            return None
    
    def parse_message_data(self, message_str):
        """Parse message string - puede ser dict string o raw data"""
        try:
            print(f"📨 Message string: {message_str}")
            
            # Si es un string de diccionario (como antes)
            if message_str.startswith('{') and message_str.endswith('}'):
                print("🔍 Detected dictionary format")
                # Usar el nuevo método seguro
                data = self.safe_parse_dict(message_str)
                if data:
                    print(f"✅ Successfully parsed dictionary: {data}")
                return data
            
            # Si es raw data (nuevo formato)
            else:
                print("🔍 Detected raw data format")
                return self.parse_raw_data(message_str)
                
        except Exception as e:
            print(f"❌ Error parsing message: {e}")
            return None
    
    def handle_start_action(self, data):
        """Handle START action - inicializar lanzamiento"""
        try:
            launch_id = data['launch_id']
            
            if launch_id not in self.active_launches:
                self.active_launches[launch_id] = {
                    'launch_id': launch_id,
                    'start_date': datetime.now().strftime("%d/%m/%y_%H:%M:%S"),
                    'end_date': None,
                    'variables': []
                }
                print(f"🚀 Launch {launch_id} STARTED")
            
            # Para acción START, también guardamos el primer dato en variables
            self.save_data_point(launch_id, data)
            
        except Exception as e:
            print(f"❌ Error handling START action: {e}")
    
    def handle_launch_action(self, data):
        """Handle LAUNCH action - agregar datos a variables"""
        try:
            launch_id = data['launch_id']
            
            if launch_id not in self.active_launches:
                print(f"⚠️  Launch {launch_id} not found for LAUNCH action, creating...")
                self.handle_start_action(data)
                return
            
            # Simplemente agregar datos a variables
            self.save_data_point(launch_id, data)
            
        except Exception as e:
            print(f"❌ Error handling LAUNCH action: {e}")
    
    def handle_end_action(self, data):
        """Handle END action - finalizar lanzamiento"""
        try:
            launch_id = data['launch_id']
            
            if launch_id not in self.active_launches:
                print(f"⚠️  Launch {launch_id} not found for END action, creating...")
                self.handle_start_action(data)
            
            # Guardar último dato
            self.save_data_point(launch_id, data)
            
            # Marcar como finalizado
            self.active_launches[launch_id]['end_date'] = datetime.now().strftime("%d/%m/%y_%H:%M:%S")
            
            # Guardar en MongoDB
            self.save_launch_to_db(self.active_launches[launch_id])
            
            print(f"🏁 Launch {launch_id} ENDED")
            
            # Opcional: mantener en memoria o limpiar
            # del self.active_launches[launch_id]
            
        except Exception as e:
            print(f"❌ Error handling END action: {e}")
    
    def save_data_point(self, launch_id, data):
        """Guardar punto de datos en variables"""
        try:
            # Crear variable data
            variable_data = {
                'timestamp': data.get('timestamp'),
                'received_at': data.get('received_at', time.time()),
                'action': data.get('action', '').lower()
            }
            
            # Agregar datos de sensores si existen
            if 'temperature' in data and data['temperature'] is not None:
                variable_data['temperature'] = data['temperature']
            
            if 'humidity' in data and data['humidity'] is not None:
                variable_data['humidity'] = data['humidity']
            
            # Agregar datos GPS si existen
            if 'latitude' in data and data['latitude'] is not None:
                variable_data['latitude'] = data['latitude']
                variable_data['longitude'] = data['longitude']
                variable_data['altitude'] = data['altitude']
            
            # Agregar a variables
            self.active_launches[launch_id]['variables'].append(variable_data)
            
            # Guardar en MongoDB inmediatamente
            self.save_launch_to_db(self.active_launches[launch_id])
            
            print(f"📊 Data point saved for launch {launch_id}")
            print(f"📈 Variables count: {len(self.active_launches[launch_id]['variables'])}")
            
        except Exception as e:
            print(f"❌ Error saving data point: {e}")
    
    def handle_launch_data(self, data):
        """Handle all types of launch data based on action"""
        try:
            action = data.get('action', '').lower()
            launch_id = data['launch_id']
            
            print(f"🔄 Processing: launch_id={launch_id}, action={action}")
            
            # Manejar según la acción
            if action == 'start':
                self.handle_start_action(data)
            elif action == 'launch':
                self.handle_launch_action(data)
            elif action == 'end':
                self.handle_end_action(data)
            else:
                print(f"⚠️  Unknown action: {action}, treating as LAUNCH")
                self.handle_launch_action(data)
                
        except Exception as e:
            print(f"❌ Error handling launch data: {e}")
    
    def save_launch_to_db(self, launch_data):
        """Save launch data to MongoDB in the required format"""
        try:
            # Crear documento en el formato exacto que necesitas
            document = {
                'launch_id': launch_data['launch_id'],
                'start_date': launch_data['start_date'],
                'end_date': launch_data['end_date'],
                'variables': launch_data['variables']
            }
            
            # Actualizar o insertar documento
            result = self.collection.update_one(
                {'launch_id': launch_data['launch_id']},
                {'$set': document},
                upsert=True
            )
            
            if result.upserted_id:
                print(f"✅ New launch saved to MongoDB: {result.upserted_id}")
            else:
                print(f"✅ Launch updated in MongoDB: {launch_data['launch_id']}")
            
        except Exception as e:
            print(f"❌ Error saving to MongoDB: {e}")
    
    def process_message(self, message):
        """Process incoming Redis message"""
        try:
            if message['type'] != 'message':
                return
            
            message_data = message['data']
            print(f"📦 Received message: {message_data}")
            
            # Parsear el mensaje
            data = self.parse_message_data(message_data)
            
            if not data:
                print("❌ Failed to parse message data")
                return
            
            # Validar admin key - MOSTRAR COMPARACIÓN PARA DEBUG
            received_admin_key = data.get('admin_key')
            expected_admin_key = self.config.ADMIN_KEY
            
            print(f"🔑 Comparing admin keys - Received: '{received_admin_key}', Expected: '{expected_admin_key}'")
            
            if received_admin_key != expected_admin_key:
                print(f"❌ Invalid admin key: '{received_admin_key}' != '{expected_admin_key}'")
                return
            
            print("✅ Admin key validation passed")
            
            # Procesar datos según la acción
            self.handle_launch_data(data)
                
        except Exception as e:
            print(f"❌ Error processing message: {e}")
    
    def subscribe(self):
        """Subscribe to Redis channel and process messages"""
        try:
            pubsub = self.redis_client.pubsub()
            pubsub.subscribe(self.config.REDIS_CHANNEL)
            
            print(f"🎯 Subscribed to Redis channel: {self.config.REDIS_CHANNEL}")
            print("⏳ Waiting for messages...")
            print("=" * 50)
            
            for message in pubsub.listen():
                self.process_message(message)
                
        except KeyboardInterrupt:
            print("\n🛑 Subscriber stopped by user")
        except Exception as e:
            print(f"❌ Error in subscription: {e}")
        finally:
            if hasattr(self, 'pubsub'):
                pubsub.close()
    
    def run(self):
        """Main execution"""
        if self.connect_databases():
            self.subscribe()
        if self.mongo_client:
            self.mongo_client.close()

if __name__ == "__main__":
    subscriber = DataSubscriber()
    subscriber.run()