import { Component } from '@angular/core';
import { CommonModule } from '@angular/common';

@Component({
  selector: 'app-footer',
  standalone: true,
  imports: [CommonModule],
  template: `
    <footer class="footer">
      <p>&copy; 2025 CANSAT PROYECTO. Todos los derechos reservados.</p>
    </footer>
  `,
  styleUrls: ['./footer.component.scss']
})
export class FooterComponent { }