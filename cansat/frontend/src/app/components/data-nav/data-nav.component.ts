import { Component, Input, Output, EventEmitter } from '@angular/core';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-data-nav',
  standalone: true,
  imports: [CommonModule],
  template: `
    <nav class="data-nav" *ngIf="visible">
      <ul>
        <li>
          <button 
            [class.active]="activeTab === 'humidity'"
            (click)="onTabChange('humidity')"
          >
            Humedad
          </button>
        </li>
        <li>
          <button 
            [class.active]="activeTab === 'temperature'"
            (click)="onTabChange('temperature')"
          >
            Temperatura
          </button>
        </li>
      </ul>
    </nav>
  `,
  styleUrls: ['./data-nav.component.scss']
})
export class DataNavComponent {
  @Input() visible: boolean = false;
  @Input() activeTab: string = 'humidity';
  @Output() tabChanged = new EventEmitter<string>();

  onTabChange(tab: string) {
    this.activeTab = tab;
    this.tabChanged.emit(tab);
  }
}