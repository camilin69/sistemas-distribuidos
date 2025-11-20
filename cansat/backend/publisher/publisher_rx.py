import time
import redis
from receiver import Receiver
from config import Config

class DataPublisherRX:
    TIME_INTERVAL = 1  # seconds
    END_TIMEOUT = 20    # segundos para detectar fin de transmisión
    
    def __init__(self):
        self.config = Config()
        self.receiver = Receiver(self.config.SERIAL_PORT_RX, self.config.BAUDRATE)
        self.redis_client = redis.Redis(
            host=self.config.REDIS_HOST,
            port=self.config.REDIS_PORT,
            decode_responses=True
        )
        
        # Track de lanzamientos activos
        self.active_launches = {}
        
        print(f"RX - Port: {self.config.SERIAL_PORT_RX}, Baudrate: {self.config.BAUDRATE}")
    
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
    
    def has_valid_gps_data(self, parsed_data):
        """Verifica si los datos GPS son válidos (no cero o valores corruptos)"""
        # Si no hay datos GPS en el paquete, considerar como válido
        if 'latitude' not in parsed_data or 'longitude' not in parsed_data or 'altitude' not in parsed_data:
            return True
            
        # Obtener valores GPS
        latitude = parsed_data.get('latitude')
        longitude = parsed_data.get('longitude')
        altitude = parsed_data.get('altitude')
        
        # Si alguno de los valores es None, considerar como válido (puede ser que no llegaron los datos)
        if latitude is None or longitude is None or altitude is None:
            return True
        
        # Verificar valores corruptos:
        # 1. Valores exactamente 0.0
        # 2. Valores muy pequeños (como 0.016667, 0.017333 que son corruptos)
        # 3. Valores fuera de rangos realistas para coordenadas GPS
        
        # Rangos realistas para Colombia (ajustar según tu ubicación)
        MIN_LAT = 4.0   # Latitud mínima para Colombia
        MAX_LAT = 12.0  # Latitud máxima para Colombia  
        MIN_LON = -79.0 # Longitud mínima para Colombia
        MAX_LON = -67.0 # Longitud máxima para Colombia
        MIN_ALT = 0.0   # Altitud mínima (nivel del mar)
        MAX_ALT = 5000.0 # Altitud máxima realista
        
        # Verificar si los valores están fuera de rangos realistas
        if (latitude == 0.0 or longitude == 0.0 or altitude == 0.0 or
            abs(latitude) < 0.1 or abs(longitude) < 0.1 or  # Valores muy pequeños
            not (MIN_LAT <= latitude <= MAX_LAT) or
            not (MIN_LON <= longitude <= MAX_LON) or
            not (MIN_ALT <= altitude <= MAX_ALT)):
            
            print(f"Paquete GPS corrupto detectado - lat: {latitude}, lon: {longitude}, alt: {altitude}")
            return False
            
        return True
    
    def parse_data(self, raw_data):
        """Parse data from format: admin_key-launch_id-timestamp-temp-hum-lat-lon-alt"""
        try:
            if not raw_data or raw_data == "None":
                return None
                
            # Limpiar el raw_data de caracteres extraños
            raw_data = raw_data.strip()
            
            parts = raw_data.split('*')
            
            if len(parts) < 3:
                print(f"Datos insuficientes: {raw_data}")
                return None
                
            admin_key = parts[0]
            
            # Validar que el launch_id sea numérico
            try:
                launch_id = int(parts[1])
            except ValueError:
                print(f"Launch_id inválido: {parts[1]}")
                return None
                
            # Validar que el timestamp sea numérico
            try:
                timestamp = float(parts[2])
            except ValueError:
                print(f"Timestamp inválido: {parts[2]}")
                return None
            
            parsed_data = {
                'admin_key': admin_key,
                'launch_id': launch_id,
                'timestamp': timestamp,
                'received_at': time.time()
            }
            
            # Agregar datos de sensores si están disponibles
            if len(parts) > 3:
                try:
                    temp_str = parts[3].strip()
                    parsed_data['temperature'] = float(temp_str) if temp_str else None
                except (ValueError, IndexError):
                    parsed_data['temperature'] = None
                
                try:
                    hum_str = parts[4].strip()
                    parsed_data['humidity'] = float(hum_str) if hum_str else None
                except (ValueError, IndexError):
                    parsed_data['humidity'] = None
            
            # Agregar datos GPS si están disponibles (partes 5, 6, 7)
            if len(parts) > 7:
                try:
                    lat_str = parts[5].strip()
                    lon_str = parts[6].strip()
                    alt_str = parts[7].strip()
                    
                    # Solo convertir si los strings no están vacíos
                    parsed_data['latitude'] = float(lat_str) if lat_str else None
                    parsed_data['longitude'] = float(lon_str) if lon_str else None
                    parsed_data['altitude'] = float(alt_str) if alt_str else None
                except (ValueError, IndexError) as e:
                    print(f"Error convirtiendo datos GPS: {e}")
                    parsed_data['latitude'] = None
                    parsed_data['longitude'] = None
                    parsed_data['altitude'] = None
            
            return parsed_data
            
        except Exception as e:
            print(f"Error parsing data: {e} - Raw: {raw_data}")
            return None
    
    def determine_action(self, launch_id, current_time):
        """Determina la acción basada en el estado del lanzamiento"""
        if launch_id not in self.active_launches:
            # Primer paquete de este lanzamiento
            self.active_launches[launch_id] = {
                'first_packet_time': current_time,
                'last_packet_time': current_time,
                'packet_count': 1,
                'state': 'started'
            }
            return 'start'
        else:
            # Actualizar timestamp del último paquete
            self.active_launches[launch_id]['last_packet_time'] = current_time
            self.active_launches[launch_id]['packet_count'] += 1
            
            # Si ya estaba en estado 'ended', cambiar a 'launch' (reinicio)
            if self.active_launches[launch_id]['state'] == 'ended':
                self.active_launches[launch_id]['state'] = 'running'
                return 'start'  # O 'launch' dependiendo de tu lógica
            
            return 'launch'
    
    def check_for_ended_launches(self, current_time):
        """Verifica si algún lanzamiento ha terminado por timeout"""
        ended_launches = []
        
        for launch_id, launch_data in self.active_launches.items():
            time_since_last_packet = current_time - launch_data['last_packet_time']
            
            if (time_since_last_packet > self.END_TIMEOUT and 
                launch_data['state'] != 'ended'):
                
                launch_data['state'] = 'ended'
                ended_launches.append(launch_id)
                print(f"Launch {launch_id} marcado como ENDED (timeout: {time_since_last_packet:.1f}s)")
        
        return ended_launches
    
    def publish_to_redis(self, data, action):
        """Publica datos a Redis con la acción determinada"""
        try:
            data_with_action = data.copy()
            data_with_action['action'] = action
            
            self.redis_client.publish(
                self.config.REDIS_CHANNEL, 
                str(data_with_action)
            )
            print(f"Published [{action.upper()}]: launch_id={data['launch_id']}, timestamp={data['timestamp']}")
            
        except Exception as e:
            print(f"Error publishing to Redis: {e}")
    
    def publish_ended_launch(self, launch_id):
        """Publica un paquete END para un lanzamiento terminado"""
        try:
            # Usar el timestamp del último paquete recibido + 1ms
            if launch_id in self.active_launches:
                last_timestamp = self.active_launches[launch_id]['last_packet_time']
                end_timestamp = last_timestamp + 1  # 1ms después del último paquete
            else:
                end_timestamp = time.time() * 1000  # Fallback: timestamp actual en ms

            end_data = {
                'admin_key': self.config.ADMIN_KEY,
                'launch_id': launch_id,
                'action': 'end',
                'timestamp': end_timestamp,  # Usar timestamp consistente
                'received_at': time.time()
            }
            
            self.redis_client.publish(
                self.config.REDIS_CHANNEL, 
                str(end_data)
            )
            print(f"Published [END] for launch {launch_id} with timestamp {end_timestamp}")
        
        except Exception as e:
            print(f"Error publishing END packet: {e}")
    
    def publish_data(self):
        """Main loop para RX - procesa datos de lanzamiento"""
        while True:
            try:
                current_time = time.time()
                
                # Verificar timeouts primero
                ended_launches = self.check_for_ended_launches(current_time)
                for launch_id in ended_launches:
                    self.publish_ended_launch(launch_id)
                
                # Leer datos del serial
                raw_data = self.receiver.receive_data()
                
                if raw_data:
                    print(f"RX Received: {raw_data}")
                    
                    # Parsear datos
                    parsed_data = self.parse_data(raw_data)
                    
                    if parsed_data:
                        # Validar admin key
                        if parsed_data['admin_key'] == self.config.ADMIN_KEY:
                            # Verificar si los datos GPS son válidos
                            if self.has_valid_gps_data(parsed_data):
                                launch_id = parsed_data['launch_id']
                                
                                # Determinar acción
                                action = self.determine_action(launch_id, current_time)
                                
                                # Publicar a Redis
                                self.publish_to_redis(parsed_data, action)
                            else:
                                print(f"Paquete corrupto descartado - Datos GPS inválidos para launch_id {parsed_data['launch_id']}")
                        else:
                            print("Invalid admin key")
                    else:
                        print("Failed to parse data")
                
                time.sleep(self.TIME_INTERVAL)
                
            except KeyboardInterrupt:
                print("Stopping RX publisher...")
                # Publicar END para todos los lanzamientos activos al salir
                for launch_id in list(self.active_launches.keys()):
                    self.publish_ended_launch(launch_id)
                break
            except Exception as e:
                print(f"Error in RX publish loop: {e}")
                time.sleep(self.TIME_INTERVAL)
    
    def run(self):
        if self.connect():
            self.publish_data()
        self.receiver.close()

if __name__ == "__main__":
    publisher = DataPublisherRX()
    publisher.run()