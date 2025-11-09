import { Component, OnInit } from '@angular/core';
import { Router, NavigationEnd } from '@angular/router';
import { HeaderComponent } from './components/header/header.component';
import { LaunchSelectorComponent } from './components/launch-selector/launch-selector.component';
import { DataNavComponent } from './components/data-nav/data-nav.component';
import { ChartComponent } from './components/chart/chart.component';
import { DataTableComponent } from './components/data-table/data-table.component';
import { FooterComponent } from './components/footer/footer.component';
import { CommonModule } from '@angular/common';
import { LaunchService } from './services/launch.service';
import { Launch } from './models/launch.model';
import { ChartData } from './models/launch.model';
import { GpsMapComponent } from './components/gps/gps-map.component';
import { TimeService } from './services/time.service';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [
    CommonModule,
    HeaderComponent,
    LaunchSelectorComponent,
    DataNavComponent,
    ChartComponent,
    DataTableComponent,
    FooterComponent,
    GpsMapComponent
  ],
  template: `
    <div class="app-container">
      <app-header></app-header>
      <app-launch-selector (launchSelected)="onLaunchSelected($event)"></app-launch-selector>
      
      <app-data-nav 
        *ngIf="showNav"
        [visible]="showNav" 
        [activeTab]="activeTab"
        (tabChanged)="onTabChanged($event)"
      ></app-data-nav>
      
      <main class="main-content" [class.has-data]="selectedLaunch">
        <div class="content-wrapper" *ngIf="selectedLaunch; else noData">
          <!-- Panel lateral con información del lanzamiento -->
          <aside class="launch-info-panel">
            <div class="info-card">
              <h3>Información del Lanzamiento</h3>
              
              <div class="info-item">
                <span class="label">ID:</span>
                <span class="value">{{ selectedLaunch.launch_id }}</span>
              </div>
              
              <div class="info-item">
                <span class="label">Estado:</span>
                <span class="value status" [class.in-progress]="launchDuration.isInProgress" 
                      [class.completed]="!launchDuration.isInProgress">
                  {{ launchDuration.status }}
                </span>
              </div>

              <div class="info-item">
                <span class="label">Duración:</span>
                <span class="value duration">{{ launchDuration.duration }}</span>
              </div>

              <div class="info-item">
                <span class="label">Timestamp Inicio:</span>
                <span class="value">{{ getStartTimestamp() | number:'1.0-0' }}ms</span>
              </div>

              <div class="info-item">
                <span class="label">Timestamp Fin:</span>
                <span class="value">{{ getEndTimestamp() | number:'1.0-0' }}ms</span>
              </div>
              
              <div class="info-item">
                <span class="label">Datos Registrados:</span>
                <span class="value">{{ selectedLaunch.variables.length }} puntos</span>
              </div>
              
              <div class="info-item">
                <span class="label">Zona Horaria:</span>
                <span class="value">{{ getUserTimezone() }}</span>
              </div>
            </div>
          </aside>

          <!-- Resto del template permanece igual -->
          <div class="main-panel">
            <app-chart 
              *ngIf="chartData.length > 0 && activeTab !== 'gps'"
              [data]="chartData"
              [type]="activeTab"
              [title]="getChartTitle()"
            ></app-chart>
            
            <app-data-table 
              *ngIf="chartData.length > 0 && activeTab !== 'gps'"
              [data]="chartData"
              [type]="activeTab"
            ></app-data-table>

            <app-gps-map 
              *ngIf="activeTab === 'gps' && selectedLaunch"
              [launch]="selectedLaunch"
            ></app-gps-map>
            
            <div *ngIf="chartData.length === 0 && activeTab !== 'gps'" class="no-chart-data">
              <div class="no-data-content">
                <i class="icon">📈</i>
                <h3>No hay datos disponibles</h3>
                <p>No se encontraron datos de {{ getActiveTabName().toLowerCase() }} para este lanzamiento.</p>
              </div>
            </div>
          </div>
        </div>

        <ng-template #noData>
          <div class="no-data-message">
            <div class="no-data-content">
              <i class="icon">📊</i>
              <h3>Selecciona un lanzamiento</h3>
              <p>Elige un lanzamiento del menú desplegable para visualizar los datos de sensores y GPS.</p>
            </div>
          </div>
        </ng-template>
      </main>
      
      <app-footer></app-footer>
    </div>
  `,
  styleUrls: ['./app.component.scss']
})
export class AppComponent implements OnInit {
  showNav = false;
  activeTab = 'humidity';
  selectedLaunchId: number | null = null;
  selectedLaunch: Launch | null = null;
  chartData: ChartData[] = [];
  launchDuration: any = { duration: 'N/A', status: 'No disponible', isInProgress: false };
  launchStats: any = null;

  constructor(
    private launchService: LaunchService,
    private router: Router,
    private timeService: TimeService
  ) {}

  ngOnInit() {
    this.router.events.subscribe(event => {
      if (event instanceof NavigationEnd) {
        this.parseUrlParams();
      }
    });
    this.parseUrlParams();
  }

  parseUrlParams() {
    const url = new URL(window.location.href);
    const launchId = url.searchParams.get('id_launch');
    const tab = url.searchParams.get('tab');

    if (launchId) {
      this.selectedLaunchId = parseInt(launchId, 10);
      this.showNav = true;
      this.loadLaunchData();
    } else {
      // Si no hay launchId en la URL, resetear todo
      this.selectedLaunchId = null;
      this.selectedLaunch = null;
      this.chartData = [];
      this.showNav = false;
    }

    // Validar que la pestaña sea una de las permitidas
    if (tab && this.isValidTab(tab)) {
      this.activeTab = tab;
    }
  }

  isValidTab(tab: string): boolean {
    const validTabs = ['humidity', 'temperature', 'latitude', 'longitude', 'altitude'];
    return validTabs.includes(tab);
  }

  onLaunchSelected(launchId: number | null) {
    if (launchId) {
      // Si selecciona un lanzamiento válido
      this.selectedLaunchId = launchId;
      this.showNav = true;
      this.updateUrl();
      this.loadLaunchData();
    } else {
      // Si selecciona "Seleccione un lanzamiento" (opción vacía)
      this.selectedLaunchId = null;
      this.selectedLaunch = null;
      this.chartData = [];
      this.showNav = false;
      this.router.navigate(['/']);
    }
  }

  onTabChanged(tab: string) {
    if (this.isValidTab(tab)) {
      this.activeTab = tab;
      this.updateUrl();
      this.loadChartData();
    }
  }

  updateUrl() {
    if (this.selectedLaunchId) {
      const queryParams: any = { 
        id_launch: this.selectedLaunchId
      };
      
      if (this.activeTab) {
        queryParams.tab = this.activeTab;
      }
      
      this.router.navigate([], {
        queryParams: queryParams
      });
    } else {
      // Si no hay lanzamiento seleccionado, navegar a la raíz sin parámetros
      this.router.navigate(['/']);
    }
  }

  loadLaunchData() {
    if (this.selectedLaunchId) {
      this.launchService.getLaunchById(this.selectedLaunchId)
        .subscribe({
          next: (launch) => {
            this.selectedLaunch = launch;
            this.updateLaunchInfo();
            this.loadChartData();
          },
          error: (error) => {
            console.error('Error loading launch data:', error);
            this.selectedLaunch = null;
            this.chartData = [];
            this.launchDuration = { duration: 'N/A', status: 'Error', isInProgress: false };
            this.launchStats = null;
          }
        });
    }
  }

  updateLaunchInfo() {
    if (this.selectedLaunch) {
      // Actualizar información de duración usando timestamps de Arduino
      this.launchDuration = this.calculateLaunchDuration(this.selectedLaunch);
      
      // Actualizar estadísticas
      this.launchStats = this.launchService.getLaunchStats(this.selectedLaunch);
    } else {
      this.launchDuration = { duration: 'N/A', status: 'No disponible', isInProgress: false };
      this.launchStats = null;
    }
  }

  calculateLaunchDuration(launch: any): any {
    if (!launch.variables || launch.variables.length < 2) {
        return { duration: 'N/A', status: 'Sin datos', isInProgress: false };
    }

    // Filtrar solo variables con timestamps válidos (del Arduino)
    const validVariables = launch.variables.filter((v: any) => 
        v.timestamp && v.timestamp < 1000000000 // Menos de 1,000,000,000 ms (~11 días)
    );

    if (validVariables.length < 2) {
        return { duration: 'N/A', status: 'Datos inválidos', isInProgress: false };
    }

    const sortedVars = [...validVariables].sort((a: any, b: any) => a.timestamp - b.timestamp);
    const startTime = sortedVars[0].timestamp;
    const endTime = sortedVars[sortedVars.length - 1].timestamp;
    const durationMs = endTime - startTime;

    // Determinar estado basado en end_date
    const isInProgress = !launch.end_date || 
                        launch.end_date === 'null' || 
                        launch.end_date === 'Invalid Date';

    return {
        duration: this.timeService.formatRelativeTime(durationMs),
        status: isInProgress ? 'En progreso' : 'Completado',
        isInProgress: isInProgress
    };
  }

  getStartTimestamp(): number {
      if (!this.selectedLaunch?.variables?.length) return 0;
      const validTimestamps = this.selectedLaunch.variables
          .filter((v: any) => v.timestamp && v.timestamp < 1000000000)
          .map((v: any) => v.timestamp);
      return validTimestamps.length > 0 ? Math.min(...validTimestamps) : 0;
  }

  getEndTimestamp(): number {
      if (!this.selectedLaunch?.variables?.length) return 0;
      const validTimestamps = this.selectedLaunch.variables
          .filter((v: any) => v.timestamp && v.timestamp < 1000000000)
          .map((v: any) => v.timestamp);
      return validTimestamps.length > 0 ? Math.max(...validTimestamps) : 0;
  }
  

  loadChartData() {
    if (this.selectedLaunch && this.activeTab) {
      this.chartData = this.launchService.getChartData(this.selectedLaunch, this.activeTab);
    } else {
      this.chartData = [];
    }
  }

  formatLaunchDate(dateString: string | null): string {
    return this.timeService.formatLaunchDate(dateString);
  }

  getUserTimezone(): string {
    return this.timeService.getUserTimezone();
  }

  getActiveTabName(): string {
    switch (this.activeTab) {
      case 'humidity':
        return 'Humedad';
      case 'temperature':
        return 'Temperatura';
      case 'latitude':
        return 'Latitud';
      case 'longitude':
        return 'Longitud';
      case 'altitude':
        return 'Altitud';
      default:
        return 'Datos';
    }
  }

  getChartTitle(): string {
    return `${this.getActiveTabName()} vs Tiempo`;
  }

  hasGPSData(): boolean {
    if (!this.selectedLaunch || !this.selectedLaunch.variables) {
      return false;
    }
    
    return this.selectedLaunch.variables.some(variable => 
      variable.latitude != null && variable.longitude != null
    );
  }
}