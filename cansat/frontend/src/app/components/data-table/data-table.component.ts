import { Component, Input, OnChanges } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ChartData } from '../../models/launch.model';

@Component({
  selector: 'app-data-table',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="table-container" *ngIf="data.length > 0">
      <table class="data-table">
        <thead>
          <tr>
            <th>Tiempo Relativo</th>
            <th>Timestamp Arduino</th>
            <th>{{ getColumnHeader() }}</th>
          </tr>
        </thead>
        <tbody>
          <tr *ngFor="let item of data.slice(0, 20)">
            <td>{{ item.formattedTime || item.localTime || (item.relativeTime | number:'1.0-0') + 's' }}</td>
            <td>{{ item.timestamp | number:'1.0-0' }}s</td>
            <td>{{ item.value | number:'1.2-2' }}</td>
          </tr>
        </tbody>
      </table>
      <div class="table-info" *ngIf="data.length > 20">
        Mostrando 20 de {{ data.length }} registros
      </div>
    </div>
  `,
  styleUrls: ['./data-table.component.scss']
})
export class DataTableComponent implements OnChanges {
  @Input() data: ChartData[] = [];
  @Input() type: string = 'humidity';

  ngOnChanges() {
    // Asegurarse de que los datos tengan las propiedades necesarias
    this.data = this.data.map(item => ({
      ...item,
      formattedTime: item.formattedTime || this.formatRelativeTime(item.timestamp)
    }));
  }

  // Función auxiliar para formatear tiempo relativo si no viene procesado
  private formatRelativeTime(timestamp: number): string {
    const seconds = Math.floor(timestamp / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    
    const remainingSeconds = seconds % 60;
    const remainingMinutes = minutes % 60;
    
    if (hours > 0) {
      return `${hours}h ${remainingMinutes}m ${remainingSeconds}s`;
    } else if (minutes > 0) {
      return `${minutes}m ${remainingSeconds}s`;
    } else {
      return `${seconds}s`;
    }
  }

  getColumnHeader(): string {
    switch (this.type) {
      case 'humidity':
        return 'Humedad (%)';
      case 'temperature':
        return 'Temperatura (°C)';
      case 'latitude':
        return 'Latitud (°)';
      case 'longitude':
        return 'Longitud (°)';
      case 'altitude':
        return 'Altitud (m)';
      default:
        return 'Valor';
    }
  }
}