import os
from dotenv import load_dotenv

load_dotenv()

class Config:
    SERIAL_PORT = os.getenv('SERIAL_PORT')
    BAUDRATE = int(os.getenv('BAUDRATE'))
    REDIS_HOST = os.getenv('REDIS_HOST')
    REDIS_PORT = int(os.getenv('REDIS_PORT'))
    ADMIN_KEY = os.getenv('ADMIN_KEY')
    REDIS_CHANNEL = os.getenv('REDIS_CHANNEL')
    API_BASE_URL = os.getenv('API_BASE_URL')
