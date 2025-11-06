import { Component, Output, EventEmitter, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { LaunchService } from '../../services/launch.service';
import { Launch } from '../../models/launch.model';

@Component({
  selector: 'app-launch-selector',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="selector-container">
      <div class="launch-selector-wrapper">
        <label for="launch-select" class="selector-label">Seleccionar Lanzamiento:</label>
        <select 
          id="launch-select"
          class="launch-selector"
          (change)="onLaunchChange($event)"
          [value]="selectedLaunchId"
        >
          <option value="">Seleccione un lanzamiento</option>
          <option 
            *ngFor="let launch of launches" 
            [value]="launch.launch_id"
          >
            Lanzamiento {{ launch.launch_id }} - {{ launch.start_date }}
          </option>
        </select>
      </div>
    </div>
  `,
  styleUrls: ['./launch-selector.component.scss']
})
export class LaunchSelectorComponent implements OnInit {
  @Output() launchSelected = new EventEmitter<number | null>();
  launches: Launch[] = [];
  selectedLaunchId: number | null = null;

  constructor(private launchService: LaunchService) {}

  ngOnInit() {
    this.loadLaunches();
  }

  loadLaunches() {
    this.launchService.getLaunches().subscribe({
      next: (launches: Launch[]) => {
        this.launches = launches;
      },
      error: (error: any) => {
        console.error('Error loading launches:', error);
        this.launches = [];
      }
    });
  }

  onLaunchChange(event: Event) {
    const selectElement = event.target as HTMLSelectElement;
    const launchId = selectElement.value ? parseInt(selectElement.value, 10) : null;
    this.selectedLaunchId = launchId;
    this.launchSelected.emit(launchId);
  }
}