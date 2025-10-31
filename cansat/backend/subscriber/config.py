import os
from dotenv import load_dotenv

load_dotenv()

class Config:
    REDIS_HOST = os.getenv('REDIS_HOST')
    REDIS_PORT = int(os.getenv('REDIS_PORT'))
    MONGODB_URI = os.getenv('MONGODB_URI')
    MONGODB_DB = os.getenv('MONGODB_DB')
    MONGODB_COLLECTION = os.getenv('MONGODB_COLLECTION')
    MONGO_INITDB_ROOT_USERNAME = os.getenv('MONGO_INITDB_ROOT_USERNAME')
    MONGO_INITDB_ROOT_PASSWORD = os.getenv('MONGO_INITDB_ROOT_PASSWORD')
    ADMIN_KEY = os.getenv('ADMIN_KEY')
    REDIS_CHANNEL = os.getenv('REDIS_CHANNEL')