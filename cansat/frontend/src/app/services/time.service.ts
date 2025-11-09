import { Injectable } from '@angular/core';
import { ChartData } from '../models/launch.model';

@Injectable({
  providedIn: 'root'
})
export class TimeService {

  constructor() { }

  // Convertir timestamp de Arduino (millis) a tiempo relativo
  arduinoToRelativeTime(timestamp: number, startTimestamp: number): number {
    return timestamp - startTimestamp;
  }

  // Formatear tiempo relativo en formato legible
  formatRelativeTime(relativeTimeMs: number): string {
    // Si el tiempo es muy grande, probablemente es un error
    if (relativeTimeMs > 86400000 * 30) { // Más de 30 días
        return 'Datos inválidos';
    }
    
    const seconds = Math.floor(relativeTimeMs / 1000);
    const minutes = Math.floor(seconds / 60);
    const hours = Math.floor(minutes / 60);
    const days = Math.floor(hours / 24);
    
    const remainingSeconds = seconds % 60;
    const remainingMinutes = minutes % 60;
    const remainingHours = hours % 24;
    
    if (days > 0) {
        return `${days}d ${remainingHours}h ${remainingMinutes}m ${remainingSeconds}s`;
    } else if (hours > 0) {
        return `${hours}h ${remainingMinutes}m ${remainingSeconds}s`;
    } else if (minutes > 0) {
        return `${minutes}m ${remainingSeconds}s`;
    } else {
        return `${seconds}s`;
    }
  }
  // Convertir datos del chart con tiempo relativo
  convertChartDataToRelative(chartData: ChartData[]): ChartData[] {
    if (!chartData || chartData.length === 0) return [];
    
    const startTime = chartData[0].timestamp;
    
    return chartData.map(item => {
      const relativeTime = this.arduinoToRelativeTime(item.timestamp, startTime);
      return {
        ...item,
        relativeTime: relativeTime,
        formattedTime: this.formatRelativeTime(relativeTime),
        displayTime: `${this.formatRelativeTime(relativeTime)} (${item.timestamp}ms)`
      };
    });
  }

  // Calcular duración basada en los timestamps de las variables
  calculateDurationFromVariables(variables: any[]): string {
    if (!variables || variables.length < 2) return 'N/A';
    
    // Filtrar timestamps válidos
    const validVariables = variables.filter(v => 
        v.timestamp && v.timestamp < 1000000000
    );
    
    if (validVariables.length < 2) return 'Datos inválidos';
    
    const sortedVariables = [...validVariables].sort((a, b) => a.timestamp - b.timestamp);
    const startTime = sortedVariables[0].timestamp;
    const endTime = sortedVariables[sortedVariables.length - 1].timestamp;
    const durationMs = endTime - startTime;
    
    return this.formatRelativeTime(durationMs);
  }

  convertAnyDataToRelative(data: any[], timestampField: string = 'timestamp'): any[] {
    if (!data || data.length === 0) return [];
    
    const startTime = data[0][timestampField];
    
    return data.map(item => {
      const relativeTime = this.arduinoToRelativeTime(item[timestampField], startTime);
      return {
        ...item,
        relativeTime: relativeTime,
        formattedTime: this.formatRelativeTime(relativeTime),
        displayTime: `${this.formatRelativeTime(relativeTime)} (${item[timestampField]}ms)`
      };
    });
  }

  // Obtener el timestamp mínimo (inicio)
  getStartTimestamp(variables: any[]): number {
    if (!variables || variables.length === 0) return 0;
    return Math.min(...variables.map(v => v.timestamp));
  }

  // Obtener el timestamp máximo (fin)
  getEndTimestamp(variables: any[]): number {
    if (!variables || variables.length === 0) return 0;
    return Math.max(...variables.map(v => v.timestamp));
  }

  // Formatear fecha de inicio/fin (si existen en el launch)
  formatLaunchDate(dateString: string | null): string {
    if (!dateString || dateString === 'null' || dateString === 'Invalid Date') {
      return 'N/A';
    }
    
    try {
      const date = new Date(dateString);
      // Verificar si es una fecha válida
      if (isNaN(date.getTime())) {
        return 'N/A';
      }
      return date.toLocaleString('es-ES', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
      });
    } catch (error) {
      console.error('Error formatting date:', error);
      return 'N/A';
    }
  }

  // Verificar si un lanzamiento está en curso
  isLaunchInProgress(endDate: string | null): boolean {
    if (!endDate || endDate === 'null' || endDate === 'Invalid Date') return true;
    
    try {
      const end = new Date(endDate);
      return isNaN(end.getTime()) || new Date() < end;
    } catch (error) {
      return true;
    }
  }

  // Obtener tiempo transcurrido desde el inicio
  getElapsedTime(startDate: string | null): string {
    if (!startDate || startDate === 'null' || startDate === 'Invalid Date') return 'N/A';
    
    try {
      const start = new Date(startDate);
      if (isNaN(start.getTime())) return 'N/A';
      
      const now = new Date();
      const elapsedMs = now.getTime() - start.getTime();
      
      return this.formatRelativeTime(elapsedMs);
    } catch (error) {
      return 'N/A';
    }
  }

  // Detectar la zona horaria del usuario
  getUserTimezone(): string {
    return Intl.DateTimeFormat().resolvedOptions().timeZone;
  }

  // Verificar si es horario de verano
  isDaylightSavingTime(): boolean {
    const now = new Date();
    const jan = new Date(now.getFullYear(), 0, 1);
    const jul = new Date(now.getFullYear(), 6, 1);
    return now.getTimezoneOffset() < Math.max(jan.getTimezoneOffset(), jul.getTimezoneOffset());
  }
}