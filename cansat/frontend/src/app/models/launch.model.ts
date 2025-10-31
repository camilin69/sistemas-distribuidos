export interface Launch {
  launch_id: number;
  start_date: string;
  end_date: string;
  variables: Variable[];
}

export interface Variable {
  timestamp: number;
  temperature: number;
  humidity: number;
}

export interface ChartData {
  timestamp: number;
  value: number;
}