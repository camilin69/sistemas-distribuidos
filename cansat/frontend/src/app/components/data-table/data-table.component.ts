import { Component, Input } from '@angular/core';
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
            <th>Tiempo (s)</th>
            <th>{{ type === 'humidity' ? 'Humedad (%)' : 'Temperatura (°C)' }}</th>
          </tr>
        </thead>
        <tbody>
          <tr *ngFor="let item of data.slice(0, 20)">
            <td>{{ item.timestamp | number:'1.2-2' }}</td>
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
export class DataTableComponent {
  @Input() data: ChartData[] = [];
  @Input() type: string = 'humidity';
}