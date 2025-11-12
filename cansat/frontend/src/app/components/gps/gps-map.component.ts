// gps-map.component.ts
import { Component, Input, OnChanges } from '@angular/core';
import { CommonModule } from '@angular/common';
import { HttpClient } from '@angular/common/http';
import { Launch, Variable } from '../../models/launch.model';

@Component({
  selector: 'app-gps-map',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="gps-container" *ngIf="gpsData.length > 0">
      <!-- Información de estadísticas -->
      <div class="stats-section">
        <h3>Estadísticas de Trayectoria GPS</h3>
        <div class="stats-grid">
          <div class="stat-item">
            <span class="stat-label">Puntos Totales:</span>
            <span class="stat-value">{{ gpsData.length }}</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">Altitud Máxima:</span>
            <span class="stat-value">{{ getMaxAltitude() | number:'1.2-2' }}m</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">Altitud Mínima:</span>
            <span class="stat-value">{{ getMinAltitude() | number:'1.2-2' }}m</span>
          </div>
          <div class="stat-item">
            <span class="stat-label">Distancia Recorrida:</span>
            <span class="stat-value">{{ calculateTotalDistance() | number:'1.2-2' }}km</span>
          </div>
        </div>
      </div>

      <!-- Visualización 3D desde el backend -->
      <div class="visualization-3d-section">
        <h3>Visualización 3D - Trayectoria (Latitud, Longitud, Altitud)</h3>
        
        <div class="plot-controls">
          <button (click)="load3DPlot()" [disabled]="loadingPlot" class="btn-primary">
            {{ loadingPlot ? 'Generando...' : 'Generar Visualización 3D' }}
          </button>
          <button (click)="download3DPlot()" [disabled]="!plotImage" class="btn-secondary">
            Descargar Imagen
          </button>
        </div>

        <div class="plot-container" *ngIf="plotImage">
          <img [src]="plotImage" alt="Visualización 3D de la trayectoria" class="plot-image" />
          <div class="plot-info">
            <p><strong>Visualización generada con Matplotlib</strong></p>
            <p>La imagen muestra la trayectoria completa en 3D con latitud, longitud y altitud</p>
          </div>
        </div>

        <div *ngIf="plotError" class="plot-error">
          <p>❌ {{ plotError }}</p>
        </div>
      </div>

      <!-- Lista de coordenadas -->
      <div class="coordinates-section">
        <h3>Puntos de Trayectoria ({{ gpsData.length }} puntos)</h3>
        <div class="coordinates-list">
          <div *ngFor="let point of gpsData.slice(0, 15); let i = index" 
               class="coordinate-item">
            <div class="coordinate-time">
              <span *ngIf="i === 0" class="point-badge start">INICIO</span>
              <span *ngIf="i === 14 || i === gpsData.length - 1" class="point-badge end">FIN</span>
              T: {{ point.timestamp | number:'1.0-0' }}ms
            </div>
            <div class="coordinate-data">
              <span>Lat: {{ point.latitude | number:'1.6-6' }}</span>
              <span>Lon: {{ point.longitude | number:'1.6-6' }}</span>
              <span>Alt: {{ point.altitude | number:'1.2-2' }}m</span>
            </div>
          </div>
        </div>
        <div *ngIf="gpsData.length > 15" class="more-points">
          ... y {{ gpsData.length - 15 }} puntos más
        </div>
      </div>
    </div>
    
    <div *ngIf="gpsData.length === 0" class="no-data">
      <div class="no-data-content">
        <i class="icon">🗺️</i>
        <h3>No hay datos GPS disponibles</h3>
        <p>No se encontraron datos de geolocalización para este lanzamiento.</p>
      </div>
    </div>
  `,
  styleUrls: ['./gps-map.component.scss']
})
export class GpsMapComponent implements OnChanges {
  @Input() launch: Launch | null = null;

  gpsData: any[] = [];
  plotImage: string | null = null;
  loadingPlot = false;
  plotError: string | null = null;

  constructor(private http: HttpClient) {}

  ngOnChanges() {
    if (this.launch) {
      this.gpsData = this.getGPSData(this.launch);
      this.plotImage = null;
      this.plotError = null;
    }
  }

  private getGPSData(launch: Launch): any[] {
    if (!launch.variables) return [];
    
    return launch.variables
      .filter((v: Variable) => v.latitude != null && v.longitude != null)
      .map((v: Variable) => ({
        latitude: v.latitude!,
        longitude: v.longitude!,
        altitude: v.altitude || 0,
        timestamp: v.timestamp
      }))
      .sort((a: any, b: any) => a.timestamp - b.timestamp);
  }

  load3DPlot() {
    if (!this.launch) return;

    this.loadingPlot = true;
    this.plotError = null;
    
    const launchId = this.launch.launch_id;
    
    this.http.get<any>(`http://localhost:5000/launch_cansat/launch/${launchId}/3d-plot-base64`)
      .subscribe({
        next: (response) => {
          this.plotImage = response.image;
          this.loadingPlot = false;
        },
        error: (error) => {
          this.plotError = error.error?.error || 'Error al generar la visualización 3D';
          this.loadingPlot = false;
        }
      });
  }

  download3DPlot() {
    if (!this.launch || !this.plotImage) return;
    
    const link = document.createElement('a');
    link.href = this.plotImage;
    link.download = `trayectoria_3d_lanzamiento_${this.launch.launch_id}.png`;
    link.click();
  }

  // Métodos de utilidad para las estadísticas
  getMaxAltitude(): number {
    return this.gpsData.length > 0 ? Math.max(...this.gpsData.map(d => d.altitude)) : 0;
  }

  getMinAltitude(): number {
    return this.gpsData.length > 0 ? Math.min(...this.gpsData.map(d => d.altitude)) : 0;
  }

  calculateTotalDistance(): number {
    if (this.gpsData.length < 2) return 0;
    
    let totalDistance = 0;
    for (let i = 1; i < this.gpsData.length; i++) {
      const prev = this.gpsData[i - 1];
      const curr = this.gpsData[i];
      totalDistance += this.calculateHaversineDistance(prev, curr);
    }
    return totalDistance / 1000;
  }

  private calculateHaversineDistance(point1: any, point2: any): number {
    const R = 6371000;
    const dLat = this.toRad(point2.latitude - point1.latitude);
    const dLon = this.toRad(point2.longitude - point1.longitude);
    const lat1 = this.toRad(point1.latitude);
    const lat2 = this.toRad(point2.latitude);

    const a = Math.sin(dLat/2) * Math.sin(dLat/2) +
            Math.sin(dLon/2) * Math.sin(dLon/2) * Math.cos(lat1) * Math.cos(lat2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
    return R * c;
  }

  private toRad(degrees: number): number {
    return degrees * (Math.PI / 180);
  }
}