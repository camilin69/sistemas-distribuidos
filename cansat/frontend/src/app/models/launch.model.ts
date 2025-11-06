// src/app/models/launch.model.ts
export interface Launch {
  launch_id: number;
  start_date: string;
  end_date: string;
  variables: Variable[];
}

export interface Variable {
  timestamp: number;
  temperature?: number;
  humidity?: number;
  latitude?: number;
  longitude?: number;
  altitude?: number;
}

export interface ChartData {
  timestamp: number;
  value: number;
  relativeTime?: number;
  formattedTime?: string;
  displayTime?: string;
  localTime?: string; 
  date?: string; 
  time?: string;
}