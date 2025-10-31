import { Component, Output, EventEmitter, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Router } from '@angular/router'; // Importar Router
import { Launch } from '../../models/launch.model';
import { LaunchService } from '../../services/launch.service';

@Component({
  selector: 'app-launch-selector',
  standalone: true,
  imports: [CommonModule, FormsModule],
  template: `
    <div class="selector-container">
      <select 
        [(ngModel)]="selectedLaunchId" 
        (ngModelChange)="onLaunchChange()"
        class="launch-selector"
      >
        <option value="">Seleccione un lanzamiento</option>
        <option *ngFor="let launch of launches" [value]="launch.launch_id">
          Lanzamiento {{ launch.launch_id }}
        </option>
      </select>
    </div>
  `,
  styleUrls: ['./launch-selector.component.scss']
})
export class LaunchSelectorComponent implements OnInit {
  @Output() launchSelected = new EventEmitter<number | null>(); // Cambiar para aceptar null
  
  launches: Launch[] = [];
  selectedLaunchId: string = "";

  constructor(
    private launchService: LaunchService,
    private router: Router // Inyectar Router
  ) { }

  ngOnInit() {
    this.loadLaunches();
    this.parseCurrentUrl(); // Para sincronizar con la URL actual
  }

  parseCurrentUrl() {
    const url = new URL(window.location.href);
    const launchId = url.searchParams.get('id_launch');
    
    if (launchId) {
      this.selectedLaunchId = launchId;
    }
  }

  loadLaunches() {
    this.launchService.getAllLaunches().subscribe({
      next: (launches) => {
        this.launches = launches;
      },
      error: (error) => {
        console.error('Error loading launches:', error);
      }
    });
  }

  onLaunchChange() {
    if (this.selectedLaunchId) {
      // Si selecciona un lanzamiento, emitir el ID
      this.launchSelected.emit(parseInt(this.selectedLaunchId));
    } else {
      // Si selecciona la opción vacía, navegar a la ruta principal
      this.launchSelected.emit(null);
      this.router.navigate(['/']);
    }
  }
}