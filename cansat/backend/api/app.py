from flask import Flask, jsonify, request, send_file
from flask_cors import CORS
import pymongo
from config import Config
import json
from bson import ObjectId
from datetime import datetime
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import io
import base64

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

def generate_3d_plot(launch_data):
    """Genera un gráfico 3D de la trayectoria usando Matplotlib"""
    if not launch_data or 'variables' not in launch_data:
        return None
    
    variables = launch_data['variables']
    
    # Filtrar puntos con coordenadas GPS válidas
    gps_points = [
        v for v in variables 
        if v.get('latitude') is not None and v.get('longitude') is not None and v.get('altitude') is not None
    ]
    
    if len(gps_points) < 2:
        return None
    
    # Extraer coordenadas
    latitudes = [point['latitude'] for point in gps_points]
    longitudes = [point['longitude'] for point in gps_points]
    altitudes = [point['altitude'] for point in gps_points]
    timestamps = [point['timestamp'] for point in gps_points]
    
    # Crear figura 3D
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    # Crear colores basados en el tiempo (gradiente)
    colors = plt.cm.viridis(np.linspace(0, 1, len(gps_points)))
    
    # Graficar puntos
    scatter = ax.scatter(longitudes, latitudes, altitudes, 
                        c=colors, s=50, alpha=0.8, cmap='viridis')
    
    # Graficar línea de trayectoria
    line = ax.plot(longitudes, latitudes, altitudes, 
                  'b-', alpha=0.6, linewidth=2, label='Trayectoria')
    
    # Marcar punto inicial y final
    ax.scatter(longitudes[0], latitudes[0], altitudes[0], 
              c='green', s=200, marker='o', label='Inicio', edgecolors='white')
    ax.scatter(longitudes[-1], latitudes[-1], altitudes[-1], 
              c='red', s=200, marker='s', label='Fin', edgecolors='white')
    
    # Configurar etiquetas y título
    ax.set_xlabel('Longitud', fontsize=12, labelpad=10)
    ax.set_ylabel('Latitud', fontsize=12, labelpad=10)
    ax.set_zlabel('Altitud (m)', fontsize=12, labelpad=10)
    
    title = f"Trayectoria 3D - Lanzamiento {launch_data.get('launch_id', 'N/A')}"
    ax.set_title(title, fontsize=14, pad=20)
    
    # Añadir leyenda
    ax.legend(loc='upper left', bbox_to_anchor=(0, 1))
    
    # Añadir grid
    ax.grid(True, alpha=0.3)
    
    # Configurar vista inicial
    ax.view_init(elev=30, azim=45)
    
    # Ajustar diseño
    plt.tight_layout()
    
    return fig

@app.route('/launch_cansat/launch/<int:launch_id>/3d-plot', methods=['GET'])
def get_3d_plot(launch_id):
    """Genera y devuelve un gráfico 3D de la trayectoria"""
    try:
        collection = get_db_connection()
        launch = collection.find_one({'launch_id': launch_id}, {'_id': 0})
        
        if not launch:
            return jsonify({'error': 'Launch not found'}), 404
        
        # Generar el gráfico 3D
        fig = generate_3d_plot(launch)
        
        if not fig:
            return jsonify({'error': 'No hay datos GPS suficientes para generar el gráfico 3D'}), 400
        
        # Convertir figura a imagen PNG en memoria
        img_buffer = io.BytesIO()
        plt.savefig(img_buffer, format='png', dpi=150, bbox_inches='tight')
        img_buffer.seek(0)
        
        # Limpiar la figura para liberar memoria
        plt.close(fig)
        
        # Devolver la imagen
        return send_file(img_buffer, mimetype='image/png', 
                        as_attachment=False, 
                        download_name=f'trayectoria_3d_lanzamiento_{launch_id}.png')
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/launch_cansat/launch/<int:launch_id>/3d-plot-base64', methods=['GET'])
def get_3d_plot_base64(launch_id):
    """Genera y devuelve un gráfico 3D en base64 para usar directamente en el frontend"""
    try:
        collection = get_db_connection()
        launch = collection.find_one({'launch_id': launch_id}, {'_id': 0})
        
        if not launch:
            return jsonify({'error': 'Launch not found'}), 404
        
        # Generar el gráfico 3D
        fig = generate_3d_plot(launch)
        
        if not fig:
            return jsonify({'error': 'No hay datos GPS suficientes para generar el gráfico 3D'}), 400
        
        # Convertir figura a base64
        img_buffer = io.BytesIO()
        plt.savefig(img_buffer, format='png', dpi=150, bbox_inches='tight')
        img_buffer.seek(0)
        
        # Codificar en base64
        img_base64 = base64.b64encode(img_buffer.getvalue()).decode('utf-8')
        
        # Limpiar la figura
        plt.close(fig)
        
        return jsonify({
            'image': f'data:image/png;base64,{img_base64}',
            'launch_id': launch_id,
            'points_count': len([v for v in launch.get('variables', []) 
                               if v.get('latitude') and v.get('longitude') and v.get('altitude')])
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

# Tus endpoints existentes permanecen igual...
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
        launches = list(collection.find({}, {'_id': 0}).sort('launch_id', pymongo.DESCENDING))
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
        launch = collection.find_one({'launch_id': launch_id}, {'_id': 0})
        
        if not launch or 'variables' not in launch:
            return jsonify({'error': 'Launch not found or no variables'}), 404
        
        variables = launch['variables']
        
        # Filtrar por tipo de variable
        if variable_type == 'temperature':
            data = [{'timestamp': v['timestamp'], 'value': v.get('temperature', 0)} 
                   for v in variables if v.get('temperature') is not None]
        elif variable_type == 'humidity':
            data = [{'timestamp': v['timestamp'], 'value': v.get('humidity', 0)} 
                   for v in variables if v.get('humidity') is not None]
        elif variable_type == 'latitude':
            data = [{'timestamp': v['timestamp'], 'value': v.get('latitude', 0)} 
                   for v in variables if v.get('latitude') is not None]
        elif variable_type == 'longitude':
            data = [{'timestamp': v['timestamp'], 'value': v.get('longitude', 0)} 
                   for v in variables if v.get('longitude') is not None]
        elif variable_type == 'altitude':
            data = [{'timestamp': v['timestamp'], 'value': v.get('altitude', 0)} 
                   for v in variables if v.get('altitude') is not None]
        else:
            data = variables
            
        return jsonify(data)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/launch_cansat/health', methods=['GET'])
def health_check():
    return jsonify({'status': 'healthy', 'service': 'CANSAT API'})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=config.FLASK_PORT, debug=True)