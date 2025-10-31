import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { Launch, ChartData } from '../models/launch.model';
import { environment } from '../enviroments/enviroment';

@Injectable({
  providedIn: 'root'
})
export class LaunchService {
  private apiUrl = environment.apiUrl;

  constructor(private http: HttpClient) { }

  getAllLaunches(): Observable<Launch[]> {
    return this.http.get<Launch[]>(`${this.apiUrl}/launches`);
  }

  getLaunchById(launchId: number): Observable<Launch> {
    return this.http.get<Launch>(`${this.apiUrl}/launch/${launchId}`);
  }

  getLaunchVariables(launchId: number, type: string): Observable<ChartData[]> {
    return this.http.get<ChartData[]>(`${this.apiUrl}/launch/${launchId}/variables?type=${type}`);
  }
}