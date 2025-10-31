from sys import stdin 
from mpl_toolkits.mplot3d import axes3d
import matplotlib.pyplot as plt
import numpy as np
import math

datos_x = []
datos_y = [] 
datos_z = []

print("Comienza escaneo")

# Simulación de datos de un cuadrado (en lugar de leer de stdin)
print("Generando datos simulados de un cuadrado...")

# Parámetros del cuadrado
lado = 10.0  # Longitud del lado del cuadrado
puntos_por_lado = 300  # 300 puntos por lado * 4 lados = 1200 puntos totales

# Generar puntos para los 4 lados del cuadrado
for i in range(puntos_por_lado):
    # Lado inferior (y = -lado/2, x varía)
    t = i / puntos_por_lado
    x = -lado/2 + t * lado
    y = -lado/2
    datos_x.append(x)
    datos_y.append(y)
    datos_z.append(0)
    
    # Lado derecho (x = lado/2, y varía)
    x = lado/2
    y = -lado/2 + t * lado
    datos_x.append(x)
    datos_y.append(y)
    datos_z.append(0)
    
    # Lado superior (y = lado/2, x varía)
    x = lado/2 - t * lado
    y = lado/2
    datos_x.append(x)
    datos_y.append(y)
    datos_z.append(0)
    
    # Lado izquierdo (x = -lado/2, y varía)
    x = -lado/2
    y = lado/2 - t * lado
    datos_x.append(x)
    datos_y.append(y)
    datos_z.append(0)

# Mostrar resultados
print(f"\nTotal de puntos capturados: {len(datos_x)}")
print("Primeros 5 puntos X:", [f"{x:.2f}" for x in datos_x[:5]])
print("Primeros 5 puntos Y:", [f"{y:.2f}" for y in datos_y[:5]])

# Graficar si hay datos
if len(datos_x) > 0:
    fig = plt.figure(figsize=(10,8))
    ax = fig.add_subplot(111, projection='3d')
    ax.scatter(datos_x, datos_y, datos_z, c='blue', marker='o', s=10)
    ax.set_xlabel('X')
    ax.set_ylabel('Y') 
    ax.set_zlabel('Z')
    ax.set_title(f'Scan LiDAR - {len(datos_x)} puntos (Cuadrado Simulado)')
    
    # Configurar ejes para mejor visualización
    max_val = lado/2 * 1.2
    ax.set_xlim([-max_val, max_val])
    ax.set_ylim([-max_val, max_val])
    ax.set_zlim([-1, 1])
    
    # Vista desde arriba para mejor apreciación del cuadrado
    ax.view_init(elev=90, azim=-90)
    
    plt.show()
else:
    print("No se capturaron datos para graficar")