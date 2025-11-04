from flask import Flask, jsonify, request
from flask_cors import CORS
import pymongo
from config import Config
import json
from bson import ObjectId

class JSONEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, ObjectId):
            return str(o)
        return json.JSONEncoder.default(self, o)

app = Flask(__name__)
CORS(app)
app.json_encoder = JSONEncoder

config = Config()

def get_db_connection():
    try:
        client = pymongo.MongoClient(
            config.MONGODB_URI,
            username=config.MONGO_INITDB_ROOT_USERNAME,
            password=config.MONGO_INITDB_ROOT_PASSWORD,
            authSource='admin'
        )
        db = client[config.MONGODB_DB]
        return db[config.MONGODB_COLLECTION]
    except Exception as e:
        print(f"MongoDB connection error: {e}")
        return None

@app.route('/launch_cansat/cansat_req_id', methods=['POST'])
def assign_launch_id():
    """Asigna un nuevo ID de lanzamiento para CANSAT"""
    try:
        collection = get_db_connection()
        if collection is None:
            return jsonify({'error': 'Database connection failed'}), 500
        
        # Buscar el máximo ID existente
        max_launch = collection.find_one(sort=[("launch_id", pymongo.DESCENDING)])
        next_id = 1
        
        if max_launch and 'launch_id' in max_launch:
            next_id = max_launch['launch_id'] + 1
        
        # Crear documento base para el nuevo lanzamiento
        new_launch = {
            'launch_id': next_id,
            'start_date': None,
            'end_date': None,
            'variables': []
        }
        
        # Insertar en la base de datos
        collection.insert_one(new_launch)
        
        return jsonify({
            'assigned_id': next_id,
            'status': 'success'
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500
    
@app.route('/launch_cansat/launches', methods=['GET'])
def get_all_launches():
    try:
        collection = get_db_connection()
        launches = list(collection.find({}, {'_id': 0, 'launch_id': 1, 'start_date': 1, 'end_date': 1}))
        return jsonify(launches)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/launch_cansat/launch/<int:launch_id>', methods=['GET'])
def get_launch_by_id(launch_id):
    try:
        collection = get_db_connection()
        launch = collection.find_one({'launch_id': launch_id}, {'_id': 0})
        
        if not launch:
            return jsonify({'error': 'Launch not found'}), 404
            
        return jsonify(launch)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/launch_cansat/launch/<int:launch_id>/variables', methods=['GET'])
def get_launch_variables(launch_id):
    try:
        variable_type = request.args.get('type', 'all')
        collection = get_db_connection()
        launch = collection.find_one({'launch_id': launch_id}, {'_id': 0, 'variables': 1})
        
        if not launch or 'variables' not in launch:
            return jsonify({'error': 'Launch not found or no variables'}), 404
        
        variables = launch['variables']
        
        if variable_type == 'temperature':
            data = [{'timestamp': v['timestamp'], 'value': v['temperature']} for v in variables]
        elif variable_type == 'humidity':
            data = [{'timestamp': v['timestamp'], 'value': v['humidity']} for v in variables]
        else:
            data = variables
            
        return jsonify(data)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=config.FLASK_PORT, debug=True)