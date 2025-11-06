import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable, map } from 'rxjs';
import { Launch, Variable, ChartData } from '../models/launch.model';
import { environment } from '../enviroments/enviroment';
import { TimeService } from './time.service';

@Injectable({
  providedIn: 'root'
})
export class LaunchService {
  private apiUrl = environment.apiUrl;

  constructor(
    private http: HttpClient,
    private timeService: TimeService
  ) {}

  getLaunches(): Observable<Launch[]> {
    return this.http.get<Launch[]>(`${this.apiUrl}/launches`);
  }

  getLaunchById(id: number): Observable<Launch> {
    return this.http.get<Launch>(`${this.apiUrl}/launch/${id}`);
  }

  // Método para obtener datos para gráficos
  getChartData(launch: Launch, type: string): ChartData[] {
    if (!launch || !launch.variables) {
      return [];
    }

    const chartData = launch.variables
      .filter((variable: Variable) => {
        switch (type) {
          case 'humidity':
            return variable.humidity != null;
          case 'temperature':
            return variable.temperature != null;
          case 'latitude':
            return variable.latitude != null;
          case 'longitude':
            return variable.longitude != null;
          case 'altitude':
            return variable.altitude != null;
          default:
            return false;
        }
      })
      .map((variable: Variable) => {
        let value: number;
        
        switch (type) {
          case 'humidity':
            value = variable.humidity!;
            break;
          case 'temperature':
            value = variable.temperature!;
            break;
          case 'latitude':
            value = variable.latitude!;
            break;
          case 'longitude':
            value = variable.longitude!;
            break;
          case 'altitude':
            value = variable.altitude!;
            break;
          default:
            value = 0;
        }

        return {
          timestamp: variable.timestamp,
          value: value
        };
      })
      .sort((a: ChartData, b: ChartData) => a.timestamp - b.timestamp);

    // Aplicar el procesamiento de tiempo relativo
    return this.timeService.convertChartDataToRelative(chartData);
  }

  // Método para obtener datos formateados para la tabla
  getFormattedTableData(launch: Launch, type: string): ChartData[] {
    return this.getChartData(launch, type);
  }

  // Método para compatibilidad
  getLaunchVariables(launchId: number, type: string): Observable<ChartData[]> {
    return this.getLaunchById(launchId).pipe(
      map((launch: Launch) => this.getChartData(launch, type))
    );
  }

  // Obtener información de duración del lanzamiento
  getLaunchDuration(launch: Launch): { duration: string, status: string, isInProgress: boolean } {
    if (!launch) {
      return { duration: 'N/A', status: 'No disponible', isInProgress: false };
    }

    const isInProgress = this.timeService.isLaunchInProgress(launch.end_date);
    
    let duration: string;
    let status: string;

    if (isInProgress && launch.start_date) {
      // Lanzamiento en curso
      duration = this.timeService.getElapsedTime(launch.start_date);
      status = 'En curso';
    } else if (launch.start_date && launch.end_date) {
      // Lanzamiento completado con fechas válidas
      duration = this.timeService.calculateDurationFromVariables(launch.variables || []);
      status = 'Completado';
    } else if (launch.variables && launch.variables.length > 1) {
      // Calcular duración basada en variables si no hay fechas válidas
      duration = this.timeService.calculateDurationFromVariables(launch.variables);
      status = 'Completado (estimado)';
    } else {
      duration = 'N/A';
      status = 'Sin datos';
    }

    return { duration, status, isInProgress };
  }

  // Obtener estadísticas del lanzamiento
  getLaunchStats(launch: Launch): any {
    if (!launch || !launch.variables) {
      return null;
    }

    const sortedVars = [...launch.variables].sort((a, b) => a.timestamp - b.timestamp);
    const startTime = sortedVars[0]?.timestamp;
    const endTime = sortedVars[sortedVars.length - 1]?.timestamp;
    
    return {
      totalDataPoints: launch.variables.length,
      startTimestamp: startTime || 0,
      endTimestamp: endTime || 0,
      dataPointsWithTemp: launch.variables.filter(v => v.temperature != null).length,
      dataPointsWithHumidity: launch.variables.filter(v => v.humidity != null).length,
      dataPointsWithGPS: launch.variables.filter(v => v.latitude != null && v.longitude != null).length,
      duration: this.timeService.calculateDurationFromVariables(launch.variables)
    };
  }

  getGPSDataWithLocalTime(launch: Launch): any[] {
    if (!launch || !launch.variables) {
      return [];
    }

    const gpsData = launch.variables
      .filter((variable: Variable) => variable.latitude != null && variable.longitude != null)
      .map((variable: Variable) => ({
        latitude: variable.latitude!,
        longitude: variable.longitude!,
        altitude: variable.altitude || 0,
        timestamp: variable.timestamp
      }))
      .sort((a: any, b: any) => a.timestamp - b.timestamp);

    // Usar el nuevo método más flexible
    return this.timeService.convertAnyDataToRelative(gpsData);
  }

  // Método auxiliar para obtener timestamps de inicio y fin
  getLaunchTimestamps(launch: Launch): { start: number, end: number } {
    if (!launch || !launch.variables || launch.variables.length === 0) {
      return { start: 0, end: 0 };
    }

    const timestamps = launch.variables.map(v => v.timestamp);
    return {
      start: Math.min(...timestamps),
      end: Math.max(...timestamps)
    };
  }

  // Método específico para datos de coordenadas GPS
  getGPSData(launch: Launch): any[] {
    if (!launch || !launch.variables) {
      return [];
    }

    const gpsData = launch.variables
      .filter((variable: Variable) => variable.latitude != null && variable.longitude != null)
      .map((variable: Variable) => ({
        latitude: variable.latitude!,
        longitude: variable.longitude!,
        altitude: variable.altitude || 0,
        timestamp: variable.timestamp
      }))
      .sort((a: any, b: any) => a.timestamp - b.timestamp);

    // Procesar tiempos relativos manualmente para datos GPS
    if (gpsData.length === 0) return [];

    const startTime = gpsData[0].timestamp;
    return gpsData.map(item => ({
      ...item,
      relativeTime: item.timestamp - startTime,
      formattedTime: this.timeService.formatRelativeTime(item.timestamp - startTime),
      displayTime: `${this.timeService.formatRelativeTime(item.timestamp - startTime)} (${item.timestamp}ms)`
    }));
  }
}