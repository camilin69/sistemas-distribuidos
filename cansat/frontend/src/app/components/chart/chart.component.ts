import { Component, Input, OnInit, OnChanges, OnDestroy } from '@angular/core';
import { CommonModule } from '@angular/common';
import { ChartData } from '../../models/launch.model';
import * as d3 from 'd3';

@Component({
  selector: 'app-chart',
  standalone: true,
  imports: [CommonModule],
  template: '<div class="chart-container" id="chart"></div>',
  styleUrls: ['./chart.component.scss']
})
export class ChartComponent implements OnInit, OnChanges, OnDestroy {
  @Input() data: ChartData[] = [];
  @Input() type: string = 'humidity';
  @Input() title: string = '';

  private svg: any;
  private margin = { top: 20, right: 30, bottom: 40, left: 50 };
  private width = 800;
  private height = 400;
  private resizeObserver: ResizeObserver | null = null;

  ngOnInit() {
    this.createSvg();
    if (this.data.length > 0) {
      this.drawChart();
    }
    this.setupResizeObserver();
  }

  ngOnChanges() {
    if (this.data.length > 0 && this.svg) {
      this.drawChart();
    }
  }

  ngOnDestroy() {
    if (this.resizeObserver) {
      this.resizeObserver.disconnect();
    }
  }

  private setupResizeObserver() {
    const container = document.getElementById('chart');
    if (container) {
      this.resizeObserver = new ResizeObserver(entries => {
        if (this.data.length > 0 && this.svg) {
          this.drawChart();
        }
      });
      this.resizeObserver.observe(container);
    }
  }

  private createSvg() {
    d3.select('#chart').selectAll('*').remove();
    
    const container = document.getElementById('chart');
    const containerWidth = container ? container.clientWidth : 800;
    
    this.svg = d3.select('#chart')
      .append('svg')
      .attr('width', '100%')
      .attr('height', this.height + this.margin.top + this.margin.bottom)
      .attr('viewBox', `0 0 ${containerWidth} ${this.height + this.margin.top + this.margin.bottom}`)
      .append('g')
      .attr('transform', `translate(${this.margin.left},${this.margin.top})`);
  }

  private getYAxisLabel(): string {
    switch (this.type) {
      case 'humidity':
        return 'Humedad (%)';
      case 'temperature':
        return 'Temperatura (°C)';
      case 'latitude':
        return 'Latitud (°)';
      case 'longitude':
        return 'Longitud (°)';
      case 'altitude':
        return 'Altitud (m)';
      default:
        return 'Valor';
    }
  }

  private drawChart() {
    this.svg.selectAll('*').remove();

    // Hacer el chart responsive
    const container = document.getElementById('chart');
    if (container) {
      this.width = container.clientWidth - this.margin.left - this.margin.right - 40;
      this.height = 400;
    }

    // Usar tiempo relativo en segundos
    const startTime = this.data[0].timestamp;
    const x = d3.scaleLinear()
      .domain([0, d3.max(this.data, (d: ChartData) => (d.timestamp - startTime) / 1000) as number])
      .range([0, this.width]);

    const y = d3.scaleLinear()
      .domain(d3.extent(this.data, (d: ChartData) => d.value) as [number, number])
      .nice()
      .range([this.height, 0]);

    // Add X axis
    this.svg.append('g')
      .attr('transform', `translate(0,${this.height})`)
      .call(d3.axisBottom(x))
      .append('text')
      .attr('x', this.width / 2)
      .attr('y', 35)
      .attr('fill', '#2C3E50')
      .attr('font-size', '12px')
      .text('Tiempo desde inicio (s)');

    // Add Y axis
    this.svg.append('g')
      .call(d3.axisLeft(y))
      .append('text')
      .attr('transform', 'rotate(-90)')
      .attr('y', -35)
      .attr('x', -this.height / 2)
      .attr('fill', '#2C3E50')
      .attr('font-size', '12px')
      .text(this.getYAxisLabel());

    // Add line
    const line = d3.line<ChartData>()
      .x((d: ChartData) => x((d.timestamp - startTime) / 1000))
      .y((d: ChartData) => y(d.value))
      .curve(d3.curveMonotoneX);

    this.svg.append('path')
      .datum(this.data)
      .attr('fill', 'none')
      .attr('stroke', '#2196F3')
      .attr('stroke-width', 2)
      .attr('d', line);

    // Add points
    this.svg.selectAll('circle')
      .data(this.data)
      .enter()
      .append('circle')
      .attr('cx', (d: ChartData) => x((d.timestamp - startTime) / 1000))
      .attr('cy', (d: ChartData) => y(d.value))
      .attr('r', 3)
      .attr('fill', '#1976D2')
      .attr('stroke', 'white')
      .attr('stroke-width', 1);
  }
}