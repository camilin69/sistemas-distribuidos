// src/app/components/gps/gps-map.component.ts
import { Component, Input, OnInit, OnChanges, AfterViewInit, ElementRef, ViewChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Launch, Variable } from '../../models/launch.model';
import * as L from 'leaflet';
import * as d3 from 'd3';

@Component({
  selector: 'app-gps-map',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="gps-container" *ngIf="gpsData.length > 0">
      <!-- Mapa Leaflet -->
      <div class="map-section">
        <h3>Mapa de Trayectoria - Leaflet</h3>
        <div #mapContainer class="map-container"></div>
      </div>

      <!-- Gráfico 3D de Trayectoria -->
      <div class="chart-section">
        <h3>Trayectoria 3D - Altitud vs Posición</h3>
        <div #chart3dContainer class="chart-3d-container"></div>
      </div>

      <!-- Lista de coordenadas -->
      <div class="coordinates-section">
        <h3>Puntos de Trayectoria ({{ gpsData.length }} puntos)</h3>
        <div class="coordinates-list">
          <div *ngFor="let point of gpsData.slice(0, 15)" class="coordinate-item">
            <div class="coordinate-time">{{ point.localTime }}</div>
            <div class="coordinate-data">
              <span>Lat: {{ point.latitude | number:'1.6-6' }}</span>
              <span>Lon: {{ point.longitude | number:'1.6-6' }}</span>
              <span>Alt: {{ point.altitude | number:'1.2-2' }}m</span>
            </div>
          </div>
        </div>
        <div *ngIf="gpsData.length > 15" class="more-points">
          ... y {{ gpsData.length - 15 }} puntos más
        </div>
      </div>
    </div>
    
    <div *ngIf="gpsData.length === 0" class="no-data">
      <div class="no-data-content">
        <i class="icon">🗺️</i>
        <h3>No hay datos GPS disponibles</h3>
        <p>No se encontraron datos de geolocalización para este lanzamiento.</p>
      </div>
    </div>
  `,
  styleUrls: ['./gps-map.component.scss']
})
export class GpsMapComponent implements OnChanges, AfterViewInit {
  @Input() launch: Launch | null = null;
  @ViewChild('mapContainer') mapContainer!: ElementRef;
  @ViewChild('chart3dContainer') chart3dContainer!: ElementRef;

  gpsData: any[] = [];
  private map: any;
  private chart3d: any;

  ngOnChanges() {
    if (this.launch) {
      this.gpsData = this.getGPSData(this.launch);
      if (this.gpsData.length > 0) {
        setTimeout(() => {
          this.initMap();
          this.init3DChart();
        }, 100);
      }
    }
  }

  ngAfterViewInit() {
    if (this.gpsData.length > 0) {
      setTimeout(() => {
        this.initMap();
        this.init3DChart();
      }, 500);
    }
  }

  private getGPSData(launch: Launch): any[] {
    if (!launch.variables) return [];
    
    return launch.variables
      .filter((v: Variable) => v.latitude != null && v.longitude != null)
      .map((v: Variable) => ({
        latitude: v.latitude!,
        longitude: v.longitude!,
        altitude: v.altitude || 0,
        timestamp: v.timestamp
      }))
      .sort((a: any, b: any) => a.timestamp - b.timestamp);
  }

  private initMap() {
    if (!this.mapContainer || this.gpsData.length === 0) return;

    if (this.map) {
      this.map.remove();
    }

    this.map = L.map(this.mapContainer.nativeElement).setView(
      [this.gpsData[0].latitude, this.gpsData[0].longitude], 
      13
    );

    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      attribution: '© OpenStreetMap contributors'
    }).addTo(this.map);

    const latLngs = this.gpsData.map((point: any) => 
      L.latLng(point.latitude, point.longitude)
    );

    const polyline = L.polyline(latLngs, {
      color: '#2196F3',
      weight: 4,
      opacity: 0.7
    }).addTo(this.map);

    if (this.gpsData.length > 0) {
      L.marker([this.gpsData[0].latitude, this.gpsData[0].longitude])
        .addTo(this.map)
        .bindPopup('Inicio de trayectoria')
        .openPopup();

      if (this.gpsData.length > 1) {
        const lastPoint = this.gpsData[this.gpsData.length - 1];
        L.marker([lastPoint.latitude, lastPoint.longitude])
          .addTo(this.map)
          .bindPopup('Fin de trayectoria');
      }

      this.map.fitBounds(polyline.getBounds(), { padding: [20, 20] });
    }
  }

  private init3DChart() {
    if (!this.chart3dContainer || this.gpsData.length === 0) return;

    d3.select(this.chart3dContainer.nativeElement).selectAll('*').remove();

    const width = this.chart3dContainer.nativeElement.clientWidth;
    const height = 400;
    const margin = { top: 20, right: 30, bottom: 50, left: 60 };

    const svg = d3.select(this.chart3dContainer.nativeElement)
      .append('svg')
      .attr('width', width)
      .attr('height', height);

    const xScale = d3.scaleLinear()
      .domain(d3.extent(this.gpsData, (d: any) => d.longitude) as [number, number])
      .range([margin.left, width - margin.right]);

    const yScale = d3.scaleLinear()
      .domain(d3.extent(this.gpsData, (d: any) => d.latitude) as [number, number])
      .range([height - margin.bottom, margin.top]);

    const zScale = d3.scaleLinear()
      .domain(d3.extent(this.gpsData, (d: any) => d.altitude) as [number, number])
      .range([3, 15]);

    const colorScale = d3.scaleSequential(d3.interpolateViridis)
      .domain(d3.extent(this.gpsData, (d: any) => d.timestamp) as [number, number]);

    const xAxis = d3.axisBottom(xScale);
    const yAxis = d3.axisLeft(yScale);

    svg.append('g')
      .attr('transform', `translate(0,${height - margin.bottom})`)
      .call(xAxis)
      .append('text')
      .attr('x', width / 2)
      .attr('y', 35)
      .attr('fill', '#2C3E50')
      .attr('font-size', '12px')
      .text('Longitud');

    svg.append('g')
      .attr('transform', `translate(${margin.left},0)`)
      .call(yAxis)
      .append('text')
      .attr('transform', 'rotate(-90)')
      .attr('y', -35)
      .attr('x', -height / 2)
      .attr('fill', '#2C3E50')
      .attr('font-size', '12px')
      .text('Latitud');

    const line = d3.line<any>()
      .x((d: any) => xScale(d.longitude))
      .y((d: any) => yScale(d.latitude))
      .curve(d3.curveMonotoneX);

    svg.append('path')
      .datum(this.gpsData)
      .attr('fill', 'none')
      .attr('stroke', '#2196F3')
      .attr('stroke-width', 2)
      .attr('stroke-opacity', 0.6)
      .attr('d', line);

    svg.selectAll('.trajectory-point')
      .data(this.gpsData)
      .enter()
      .append('circle')
      .attr('class', 'trajectory-point')
      .attr('cx', (d: any) => xScale(d.longitude))
      .attr('cy', (d: any) => yScale(d.latitude))
      .attr('r', (d: any) => zScale(d.altitude))
      .attr('fill', (d: any) => colorScale(d.timestamp))
      .attr('stroke', 'white')
      .attr('stroke-width', 1)
      .on('mouseover', function(event: any, d: any) {
        d3.select(this)
          .transition()
          .duration(200)
          .attr('r', zScale(d.altitude) * 1.5);
        
        const tooltip = d3.select('body')
          .append('div')
          .attr('class', 'tooltip')
          .style('opacity', 0);

        tooltip.transition()
          .duration(200)
          .style('opacity', .9);
        
        tooltip.html(`
          <strong>Tiempo:</strong> ${d.timestamp}s<br>
          <strong>Lat:</strong> ${d.latitude.toFixed(6)}<br>
          <strong>Lon:</strong> ${d.longitude.toFixed(6)}<br>
          <strong>Alt:</strong> ${d.altitude.toFixed(2)}m
        `)
          .style('left', (event.pageX + 10) + 'px')
          .style('top', (event.pageY - 28) + 'px');
      })
      .on('mouseout', function(event: any, d: any) {
        d3.select(this)
          .transition()
          .duration(200)
          .attr('r', zScale(d.altitude));
        
        d3.selectAll('.tooltip').remove();
      });

    const legendWidth = 200;
    const legendHeight = 20;

    const legendSvg = svg.append('g')
      .attr('transform', `translate(${width - legendWidth - 20},${margin.top})`);

    const legendScale = d3.scaleLinear()
      .domain(colorScale.domain())
      .range([0, legendWidth]);

    const legendAxis = d3.axisBottom(legendScale)
      .ticks(5)
      .tickFormat((d: any) => `${d}s`);

    const defs = svg.append('defs');
    const gradient = defs.append('linearGradient')
      .attr('id', 'gradient')
      .attr('x1', '0%')
      .attr('x2', '100%')
      .attr('y1', '0%')
      .attr('y2', '0%');

    gradient.selectAll('stop')
      .data(d3.range(0, 1.1, 0.1))
      .enter()
      .append('stop')
      .attr('offset', (d: any) => `${d * 100}%`)
      .attr('stop-color', (d: any) => colorScale(d * (colorScale.domain()[1]! - colorScale.domain()[0]!) + colorScale.domain()[0]!));

    legendSvg.append('rect')
      .attr('width', legendWidth)
      .attr('height', legendHeight)
      .style('fill', 'url(#gradient)');

    legendSvg.append('g')
      .attr('transform', `translate(0,${legendHeight})`)
      .call(legendAxis);

    legendSvg.append('text')
      .attr('x', legendWidth / 2)
      .attr('y', -5)
      .attr('text-anchor', 'middle')
      .attr('fill', '#2C3E50')
      .attr('font-size', '10px')
      .text('Tiempo (s)');
  }
}