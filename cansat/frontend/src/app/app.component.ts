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
    FooterComponent
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
      
      <!-- Contenido principal que se expande cuando no hay datos -->
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
                <span class="label">Fecha Inicio:</span>
                <span class="value">{{ selectedLaunch.start_date }}</span>
              </div>
              <div class="info-item">
                <span class="label">Fecha Fin:</span>
                <span class="value">{{ selectedLaunch.end_date }}</span>
              </div>
              <div class="info-item">
                <span class="label">Datos Registrados:</span>
                <span class="value">{{ selectedLaunch.variables.length }} puntos</span>
              </div>
              <div class="info-item">
                <span class="label">Variable Activa:</span>
                <span class="value">{{ activeTab === 'humidity' ? 'Humedad' : 'Temperatura' }}</span>
              </div>
            </div>
          </aside>

          <!-- Contenido principal centrado -->
          <div class="main-panel">
            <app-chart 
              *ngIf="chartData.length > 0"
              [data]="chartData"
              [type]="activeTab"
              [title]="activeTab === 'humidity' ? 'Humedad vs Tiempo' : 'Temperatura vs Tiempo'"
            ></app-chart>
            
            <app-data-table 
              *ngIf="chartData.length > 0"
              [data]="chartData"
              [type]="activeTab"
            ></app-data-table>
          </div>
        </div>

        <!-- Mensaje cuando no hay datos seleccionados -->
        <ng-template #noData>
          <div class="no-data-message">
            <div class="no-data-content">
              <i class="icon">📊</i>
              <h3>Selecciona un lanzamiento</h3>
              <p>Elige un lanzamiento del menú desplegable para visualizar los datos de temperatura y humedad.</p>
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

  constructor(
    private launchService: LaunchService,
    private router: Router
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

    if (tab && (tab === 'humidity' || tab === 'temperature')) {
      this.activeTab = tab;
    }
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
    this.activeTab = tab;
    this.updateUrl();
    this.loadChartData();
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
            this.loadChartData();
          },
          error: (error) => {
            console.error('Error loading launch data:', error);
            this.selectedLaunch = null;
            this.chartData = [];
          }
        });
    }
  }

  loadChartData() {
    if (this.selectedLaunchId && this.activeTab) {
      this.launchService.getLaunchVariables(this.selectedLaunchId, this.activeTab)
        .subscribe({
          next: (data) => {
            this.chartData = data;
          },
          error: (error) => {
            console.error('Error loading chart data:', error);
            this.chartData = [];
          }
        });
    } else {
      this.chartData = [];
    }
  }
}