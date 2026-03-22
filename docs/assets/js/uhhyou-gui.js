const Palette = {
  fontSans: 'system-ui, sans-serif',
  background: '#ffffff',
  foreground: '#000000',
  surface: '#f8f8f8',
  border: '#888888',
  main: '#fcc04f',
  accent: '#13c136',
  warning: '#fc8080',
  borderWidth: 1
};

function syncPalette() {
  const root = getComputedStyle(document.documentElement);
  const get = v => root.getPropertyValue(v).trim();
  const bg = get('--background-color');
  if (bg) {
    Palette.fontSans = get('--font-sans') || Palette.fontSans;
    Object.assign(Palette, {
      background: bg,
      foreground: get('--foreground-color'),
      surface: get('--table-p1-color'),
      border: get('--border-color'),
      main: get('--component-main'),
      accent: get('--component-accent'),
      warning: get('--component-warning')
    });
  }
}

const UIUtils = {
  updateStatus: t => {
    const s = document.getElementById('status');
    if (s) s.textContent = t || '';
  },
  createAriaAnnouncer: () => {
    const a = document.createElement('div');
    a.setAttribute('aria-live', 'polite');
    Object.assign(a.style, {
      position: 'absolute',
      width: '1px',
      height: '1px',
      padding: 0,
      margin: '-1px',
      overflow: 'hidden',
      clip: 'rect(0,0,0,0)',
      border: 0,
      whiteSpace: 'nowrap'
    });
    return a;
  },
  drawRoundedBox: (ctx, x, y, w, h, r, bg, border, lw = Palette.borderWidth) => {
    ctx.beginPath();
    ctx.roundRect(x, y, w, h, r);
    if (bg) {
      ctx.fillStyle = bg;
      ctx.fill();
    }
    if (border) {
      ctx.strokeStyle = border;
      ctx.lineWidth = lw;
      ctx.stroke();
    }
  }
};

class SnapEngine {
  constructor(distancePx = 24) {
    Object.assign(this, {snaps: [], distancePx, isSnapping: false, accum: 0});
  }
  setSnaps(arr, isRotary = false) {
    this.snaps = arr.sort((a, b) => a - b);
    if (isRotary && this.snaps[0] === 0) this.snaps.push(1.0);
  }
  cycle(val, dir, wrap = false) {
    if (!this.snaps.length) return val;
    if (dir > 0) return val >= 1.0 ? (wrap ? 0 : 1.0) : (this.snaps.find(s => s > val) ?? 1.0);
    return val <= 0.0 ? (wrap ? 1.0 : 0) : ([...this.snaps].reverse().find(s => s < val) ?? 0);
  }
  handleSnap(val, deltaPixel, rangePixels, isEnabled = true, isRotary = false) {
    if (rangePixels <= 0) return {val, snapped: false};
    const deltaNorm = deltaPixel / rangePixels, nextVal = val + deltaNorm;
    const clamp = v => isRotary ? v - Math.floor(v) : Math.max(0, Math.min(1, v));
    if (!isEnabled) return {val: clamp(nextVal), snapped: false};

    if (this.isSnapping) {
      if (Math.abs(this.accum += deltaPixel) > this.distancePx) {
        this.accum = 0;
        this.isSnapping = false;
        return {val, snapped: false};
      }
      return {val, snapped: true};
    }
    if (this.snaps.length && deltaNorm !== 0) {
      const snap = deltaNorm > 0 ? this.snaps.find(s => s > val)
                                 : [...this.snaps].reverse().find(s => s < val);
      if (snap !== undefined && (deltaNorm > 0 ? nextVal >= snap : nextVal <= snap)) {
        this.accum = 0;
        this.isSnapping = true;
        return {val: isRotary && snap === 1 ? 0 : snap, snapped: true};
      }
    }
    return {val: clamp(nextVal), snapped: false};
  }
  reset() {
    this.isSnapping = false;
    this.accum = 0;
  }
}

class CanvasBase extends HTMLElement {
  constructor() {
    super();
    this.isHovered = false;
    this.canvas = document.createElement('canvas');
    this.canvas.tabIndex = 0;
    Object.assign(this.canvas.style,
                  {width: '100%', height: '100%', display: 'block', outline: 'none'});
    this.ctx = this.canvas.getContext('2d');

    const trig = h => {
      this.isHovered = h;
      this.paint();
    };
    ['mouseenter', 'focus'].forEach(e => this.canvas.addEventListener(e, () => trig(true)));
    ['mouseleave', 'blur'].forEach(e => this.canvas.addEventListener(e, () => trig(false)));

    this.resizeObserver = new ResizeObserver(entries => {
      const r = entries[0].target.getBoundingClientRect();
      if (!r.width || !r.height) return;
      const dpr = window.devicePixelRatio || 1;
      this.canvas.width = (this.width = r.width) * dpr;
      this.canvas.height = (this.height = r.height) * dpr;
      this.ctx.scale(dpr, dpr);
      this.onResize(this.width, this.height);
      this.paint();
    });
  }
  connectedCallback() {
    if (!this._initCanvas) {
      this.mountContent();
      this._initCanvas = true;
      this.init?.();
    }
    this.resizeObserver.observe(this);
  }
  disconnectedCallback() { this.resizeObserver.disconnect(); }
  mountContent() { this.appendChild(this.canvas); }
  paint() {}
  onResize() {}
}

class CanvasWrapperBase extends CanvasBase {
  constructor() {
    super();
    this.wrapper = document.createElement('div');
    Object.assign(this.wrapper.style, {position: 'relative', width: '100%', height: '100%'});
    this.wrapper.appendChild(this.canvas);
  }
  mountContent() { this.appendChild(this.wrapper); }
}

class ButtonBase extends CanvasBase {
  constructor() {
    super();
    this.value = 0;
    this.canvas.setAttribute('role', 'button');
    this.canvas.style.cursor = 'pointer';
  }
  init() {
    this.label = this.getAttribute('label') || 'Button';
    this.hint = this.getAttribute('hint');
    this.btnStyle = this.getAttribute('type') || 'common';
    this.canvas.setAttribute('aria-label', this.label);
    if (this.hint) this.canvas.setAttribute('aria-description', this.hint);
  }
  paint() {
    if (!this.width) return;
    const s = this.height / 20.0, lw = Palette.borderWidth * s,
          col = Palette[this.btnStyle] || Palette.main;
    this.ctx.clearRect(0, 0, this.width, this.height);
    UIUtils.drawRoundedBox(this.ctx, lw / 2, lw / 2, this.width - lw, this.height - lw, lw * 2,
                           this.value ? col : Palette.surface,
                           this.isHovered ? col : Palette.border, lw);
    this.ctx.fillStyle = Palette.foreground;
    this.ctx.font = `${14 * s}px ${Palette.fontSans}`;
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText(this.label, this.width / 2, (this.height / 2) + s);
  }
}

class ToggleButton extends ButtonBase {
  constructor() {
    super();
    this.canvas.addEventListener('mousedown', e => e.button === 0 && this.toggleValue());
    this.canvas.addEventListener(
      'keydown', e => ['Enter', ' '].includes(e.key) && (e.preventDefault(), this.toggleValue()));
  }
  init() {
    super.init();
    this.canvas.setAttribute('aria-pressed', this.value ? 'true' : 'false');
  }
  toggleValue() {
    this.value ^= 1;
    this.canvas.setAttribute('aria-pressed', this.value ? 'true' : 'false');
    this.paint();
  }
}

class MomentaryButton extends ButtonBase {
  constructor() {
    super();
    const set = v => {
      if (this.value === v) return;
      this.value = v;
      this.canvas.setAttribute('aria-pressed', v ? 'true' : 'false');
      this.paint();
      if (v) this.dispatchEvent(new CustomEvent('action', {bubbles: true}));
    };
    this.canvas.addEventListener('mousedown', e => e.button === 0 && set(1));
    window.addEventListener('mouseup', () => set(0));
    this.canvas.addEventListener('blur', () => set(0));
    this.canvas.addEventListener(
      'keydown', e => ['Enter', ' '].includes(e.key) && !e.repeat && (e.preventDefault(), set(1)));
    this.canvas.addEventListener(
      'keyup', e => ['Enter', ' '].includes(e.key) && (e.preventDefault(), set(0)));
  }
}

class UhhyouKnobBase extends CanvasBase {
  constructor(knobType, defaultValue) {
    super();
    Object.assign(this, {
      knobType,
      defaultValue,
      value: defaultValue,
      isDragging: false,
      sensitivity: 0.004,
      liveUpdate: true
    });
    this.snapEngine = new SnapEngine(24);
    this.canvas.setAttribute('role', 'slider');
    this.bindEvents();
  }
  init() {
    const attrVal = this.getAttribute('value');
    if (attrVal !== null) this.value = this.defaultValue = parseFloat(attrVal);
    this.canvas.setAttribute('aria-label', this.getAttribute('label') || 'Knob');

    const parseSnaps = s => s.split(',').map(v => parseFloat(v)).filter(v => !isNaN(v));
    const attrSnap = this.getAttribute('data-snaps');
    if (attrSnap)
      this.snapEngine.setSnaps(parseSnaps(attrSnap), this.knobType === 'rotary');
    else if (['data-min', 'data-max', 'data-step'].every(a => this.hasAttribute(a))) {
      const [min, max, step] =
        ['min', 'max', 'step'].map(a => parseFloat(this.getAttribute(`data-${a}`)));
      const snaps = [];
      for (let i = min; i <= max; i += step) snaps.push((i - min) / (max - min));
      this.snapEngine.setSnaps(snaps, this.knobType === 'rotary');
    }
    this.updateAriaAttributes();
  }
  getDisplayRange() {
    return this.hasAttribute('data-min') ? {
      min: parseFloat(this.getAttribute('data-min')),
      max: parseFloat(this.getAttribute('data-max'))
    }
                                         : {min: 0, max: 1};
  }
  updateAriaAttributes() {
    const {min, max} = this.getDisplayRange();
    this.canvas.setAttribute('aria-valuemin', min);
    this.canvas.setAttribute('aria-valuemax', max);
    this.canvas.setAttribute('aria-valuenow', (min + this.value * (max - min)).toFixed(2));
  }
  setValue(v, emit = true) {
    if (this.value === v) return;
    this.value = v;
    this.updateAriaAttributes();
    this.paint();
    if (emit) this.dispatchEvent(new CustomEvent('value-changed', {detail: {value: v}}));
  }
  getFlooredInternalValue() {
    const {min, max} = this.getDisplayRange();
    return Math.max(0,
                    Math.min(1, (Math.floor(min + this.value * (max - min)) - min) / (max - min)));
  }
  updateValue(delta, emit = true) {
    const n = this.value + delta;
    this.setValue(this.knobType === 'rotary' ? n - Math.floor(n) : Math.max(0, Math.min(1, n)),
                  emit);
  }
  bindEvents() {
    this.canvas.addEventListener('pointerdown', e => {
      this.canvas.setPointerCapture(e.pointerId);
      if ((e.ctrlKey || e.metaKey) && e.button === 0)
        return (e.preventDefault(), this.setValue(this.defaultValue));
      if (e.button === 1)
        return (e.preventDefault(),
                this.setValue(
                  e.shiftKey ? this.getFlooredInternalValue()
                             : this.snapEngine.cycle(this.value, 1, this.knobType === 'rotary')));
      if (e.button !== 0) return;
      this.isDragging = true;
      this.lastY = e.clientY;
      this.canvas.style.cursor = 'ns-resize';
      e.preventDefault();
      this.paint();
    });
    this.canvas.addEventListener('pointermove', e => {
      if (!this.isDragging) return;
      const dY = this.lastY - e.clientY,
            sensi = e.shiftKey ? this.sensitivity / 5 : this.sensitivity, dNorm = dY * sensi;
      this.lastY = e.clientY;
      if (!this.snapEngine.snaps.length) return this.updateValue(dNorm, this.liveUpdate);
      const res = this.snapEngine.handleSnap(this.value, Math.abs(dY) * Math.sign(dNorm), 1 / sensi,
                                             true, this.knobType === 'rotary');
      res.snapped ? this.setValue(res.val, this.liveUpdate)
                  : this.updateValue(dNorm, this.liveUpdate);
    });
    const end = e => {
      this.canvas.releasePointerCapture(e.pointerId);
      if (this.isDragging) {
        this.isDragging = false;
        this.snapEngine.reset();
        this.canvas.style.cursor = 'default';
        if (!this.liveUpdate) this.setValue(this.value);
        this.paint();
      }
    };
    ['pointerup', 'pointercancel'].forEach(ev => this.canvas.addEventListener(ev, end));
    this.canvas.addEventListener(
      'wheel',
      e => Math.abs(e.deltaY) > 0.001
        && (e.preventDefault(), this.updateValue(Math.sign(-e.deltaY) * this.sensitivity * 2)));
    this.canvas.addEventListener('keydown', e => {
      const sensi = e.shiftKey ? this.sensitivity / 5 : this.sensitivity;
      const d = ({
                  ArrowUp: sensi * 10,
                  ArrowRight: sensi * 10,
                  ArrowDown: -sensi * 10,
                  ArrowLeft: -sensi * 10
                })[e.key]
        || 0;
      if (d) return (e.preventDefault(), this.updateValue(d));
      if (e.key === 'Home' || (e.key === 'ArrowUp' && (e.metaKey || e.ctrlKey)))
        return (e.preventDefault(),
                this.setValue(this.snapEngine.cycle(this.value, 1, this.knobType === 'rotary')));
      if (e.key === 'End' || (e.key === 'ArrowDown' && (e.metaKey || e.ctrlKey)))
        return (e.preventDefault(),
                this.setValue(this.snapEngine.cycle(this.value, -1, this.knobType === 'rotary')));
      if (['Backspace', 'Delete'].includes(e.key))
        return (e.preventDefault(),
                this.setValue(e.shiftKey && !(e.metaKey || e.ctrlKey)
                                ? this.getFlooredInternalValue()
                                : this.defaultValue));
      if (['Enter', ' '].includes(e.key)) return (e.preventDefault(), this.promptForValue());
    });
    this.canvas.addEventListener('dblclick', e => e.button === 0 && this.promptForValue());
  }
  promptForValue() {
    const {min, max} = this.getDisplayRange(), cur = min + this.value * (max - min);
    const input = prompt(`Enter new value (${min} to ${max}):`, cur.toFixed(2));
    if (input === '') return this.setValue(this.defaultValue);
    if (input && !isNaN(input)) {
      let v = (Math.max(min, Math.min(max, parseFloat(input))) - min) / (max - min);
      this.setValue(this.knobType === 'rotary' ? v - Math.floor(v) : v);
    }
  }
  getHandCoordinates(norm, radLen) {
    const rad
      = this.knobType === 'rotary' ? Math.PI * 2 * norm : Math.PI * (2 * norm - 1) * (1 - 2 / 12);
    return {
      x: this.width / 2 + Math.sin(rad) * radLen,
      y: this.height / 2 - Math.cos(rad) * radLen
    };
  }
}

class CircularKnobPainter extends UhhyouKnobBase {
  paint() {
    if (!this.width) return;
    const w = this.width, h = this.height, size = Math.min(w, h), scale = size / 80,
          lw = Palette.borderWidth * 8 * scale, r = size / 2 - lw / 2 - 2;
    this.ctx.clearRect(0, 0, w, h);
    this.ctx.beginPath();
    this.knobType === 'rotary'
      ? this.ctx.arc(w / 2, h / 2, r, 0, Math.PI * 2)
      : this.ctx.arc(w / 2, h / 2, r, Math.PI * 0.666, Math.PI * 0.333, false);
    this.ctx.strokeStyle = this.isHovered || this.isDragging ? Palette.main : `${Palette.border}4D`;
    this.ctx.lineWidth = lw;
    this.ctx.lineCap = 'round';
    this.ctx.stroke();

    const dS = this.getHandCoordinates(this.defaultValue, r / 2),
          dE = this.getHandCoordinates(this.defaultValue, r - lw / 2);
    this.ctx.beginPath();
    this.ctx.moveTo(dS.x, dS.y);
    this.ctx.lineTo(dE.x, dE.y);
    this.ctx.lineWidth = Palette.borderWidth * 2 * scale;
    this.ctx.stroke();

    const hp = this.getHandCoordinates(this.value, r);
    this.ctx.beginPath();
    this.ctx.moveTo(w / 2, h / 2);
    this.ctx.lineTo(hp.x, hp.y);
    this.ctx.strokeStyle = Palette.foreground;
    this.ctx.stroke();
    this.ctx.beginPath();
    this.ctx.arc(hp.x, hp.y, lw / 2, 0, Math.PI * 2);
    this.ctx.fillStyle = Palette.foreground;
    this.ctx.fill();
  }
}

class TextKnobPainter extends UhhyouKnobBase {
  paint() {
    if (!this.width) return;
    const s = this.height / 20, lw = Palette.borderWidth * s, rh = this.height - lw;
    this.ctx.clearRect(0, 0, this.width, this.height);
    UIUtils.drawRoundedBox(this.ctx, lw / 2, lw / 2, this.width - lw, rh, lw * 2, Palette.surface,
                           this.isHovered || this.isDragging ? Palette.main : Palette.border, lw);

    if (this.isHovered || this.isDragging) {
      this.ctx.fillStyle = `${this.ctx.strokeStyle}80`;
      this.ctx.beginPath();
      this.ctx.roundRect(this.width - lw * 2.5 - lw * 4, this.height - lw * 1.5 - rh * this.value,
                         lw * 4, rh * this.value, lw * 2);
      this.ctx.fill();
    }
    const {min, max} = this.getDisplayRange();
    this.ctx.fillStyle = Palette.foreground;
    this.ctx.font = `${14 * s}px ${Palette.fontSans}`;
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText((min + this.value * (max - min)).toFixed(this.precision ?? 2), this.width / 2,
                      (this.height / 2) + s);
  }
}

class Knob extends CircularKnobPainter {
  constructor() { super('clamped', 0.5); }
}
class RotaryKnob extends CircularKnobPainter {
  constructor() { super('rotary', 0.0); }
}
class TextKnob extends TextKnobPainter {
  constructor() {
    super('clamped', 0.5);
    this.precision = 2;
  }
}
class RotaryTextKnob extends TextKnobPainter {
  constructor() {
    super('rotary', 0.0);
    this.precision = 2;
  }
}

class XYPad extends CanvasBase {
  constructor() {
    super();
    Object.assign(this, {
      isEditing: false,
      lastActiveAxis: 'x',
      snappingEnabled: true,
      snapDistancePixel: 24,
      lastMousePos: {x: -1, y: -1},
      mousePos: {x: -1, y: -1},
      axisLock: 'none'
    });
    this.snapX = new SnapEngine(24);
    this.snapY = new SnapEngine(24);
    ['role', 'aria-label', 'aria-valuemin', 'aria-valuemax', 'aria-description'].forEach(
      (a, i) => this.canvas.setAttribute(
        a,
        ['slider', 'XY Pad', '0', '100', 'Use Left/Right for X axis. Use Up/Down for Y axis.'][i]));
    this.canvas.oncontextmenu = () => false;
    this.announcer = UIUtils.createAriaAnnouncer();
    this.bindPadEvents();
  }
  mountContent() {
    super.mountContent();
    this.appendChild(this.announcer);
  }
  init() {
    this.valueX = parseFloat(this.getAttribute('x') || '0.5');
    this.valueY = parseFloat(this.getAttribute('y') || '0.5');
    const parse
      = a => (this.getAttribute(a) || '').split(',').map(v => parseFloat(v)).filter(v => !isNaN(v));
    const sx = parse('data-snaps-x'), sy = parse('data-snaps-y');
    if (sx.length) this.snapX.setSnaps(sx);
    if (sy.length) this.snapY.setSnaps(sy);
    this.updateAria();
  }
  updateAria() {
    const xp = (this.valueX * 100).toFixed(1), yp = (this.valueY * 100).toFixed(1),
          t = `X: ${xp}%, Y: ${yp}%`;
    this.canvas.setAttribute('aria-valuenow', this.lastActiveAxis === 'x' ? xp : yp);
    this.canvas.setAttribute('aria-valuetext', t);
    this.announcer.textContent = t;
  }
  setSnappingEnabled(s) { this.snappingEnabled = s; }
  setValues(x, y, emit = true) {
    let ch = false;
    if (x !== null && x !== this.valueX) {
      this.valueX = x;
      this.lastActiveAxis = 'x';
      ch = true;
    }
    if (y !== null && y !== this.valueY) {
      this.valueY = y;
      this.lastActiveAxis = 'y';
      ch = true;
    }
    if (ch) {
      this.updateAria();
      this.paint();
      if (emit)
        this.dispatchEvent(
          new CustomEvent('value-changed', {detail: {x: this.valueX, y: this.valueY}}));
    }
  }
  cycleValue(axis, dir) {
    const isX = axis === 'x';
    const val = (isX ? this.snapX : this.snapY).cycle(isX ? this.valueX : this.valueY, dir);
    this.setValues(isX ? val : null, isX ? null : val);
  }
  bindPadEvents() {
    this.canvas.addEventListener('mousemove', e => {
      if (!this.isEditing) {
        this.mousePos = {x: e.offsetX, y: e.offsetY};
        this.paint();
      }
    });
    this.canvas.addEventListener('mousedown', e => e.button === 1 && e.preventDefault());
    this.canvas.addEventListener('pointerdown', e => {
      this.canvas.setPointerCapture(e.pointerId);
      if (e.button === 2)
        return UIUtils.updateStatus(
          `Host Menu opened for ${e.offsetX < this.width / 2 ? 'X' : 'Y'} Axis`);
      this.axisLock = e.button === 1 ? (e.shiftKey ? 'x' : 'y') : 'none';
      if (e.button === 1)
        UIUtils.updateStatus(`Axis locked: ${this.axisLock.toUpperCase()} will not change.`);
      if ((e.ctrlKey || e.metaKey) && e.button !== 1)
        return (this.setValues(0.5, 0.5), UIUtils.updateStatus('Reset to default'));
      this.isEditing = true;
      this.snapX.reset();
      this.snapY.reset();
      this.lastMousePos = {x: e.clientX, y: e.clientY};
      if (e.button !== 1) {
        this.setValues(
          this.axisLock !== 'x' ? Math.max(0, Math.min(1, e.offsetX / this.width)) : null,
          this.axisLock !== 'y' ? Math.max(0, Math.min(1, 1 - e.offsetY / this.height)) : null);
        UIUtils.updateStatus(`X: ${this.valueX.toFixed(3)}, Y: ${this.valueY.toFixed(3)}`);
      }
      e.preventDefault();
    });
    this.canvas.addEventListener('pointermove', e => {
      if (!this.isEditing) return;
      const dx = e.clientX - this.lastMousePos.x, dy = e.clientY - this.lastMousePos.y;
      this.lastMousePos = {x: e.clientX, y: e.clientY};
      this.setValues(
        this.axisLock !== 'x'
          ? this.snapX.handleSnap(this.valueX, dx, this.width, this.snappingEnabled).val
          : null,
        this.axisLock !== 'y'
          ? this.snapY.handleSnap(this.valueY, -dy, this.height, this.snappingEnabled).val
          : null);
      UIUtils.updateStatus(`X: ${this.valueX.toFixed(3)}, Y: ${this.valueY.toFixed(3)}`);
    });
    const end = e => {
      this.canvas.releasePointerCapture(e.pointerId);
      if (this.isEditing) {
        this.isEditing = false;
        this.snapX.reset();
        this.snapY.reset();
        this.paint();
      }
    };
    ['pointerup', 'pointercancel'].forEach(ev => this.canvas.addEventListener(ev, end));
    this.canvas.addEventListener('wheel', e => {
      if (Math.abs(e.deltaY) < 0.001) return;
      e.preventDefault();
      const d = Math.sign(-e.deltaY) * 0.02;
      e.shiftKey ? this.setValues(null, Math.max(0, Math.min(1, this.valueY + d)))
                 : this.setValues(Math.max(0, Math.min(1, this.valueX + d)), null);
    });
    this.canvas.addEventListener('keydown', e => {
      const s = e.shiftKey ? 0.0078125 / 5 : 0.0078125, m = e.metaKey || e.ctrlKey;
      if (e.key === 'PageUp' || (e.key === 'ArrowRight' && m))
        return (e.preventDefault(), this.cycleValue('x', 1));
      if (e.key === 'PageDown' || (e.key === 'ArrowLeft' && m))
        return (e.preventDefault(), this.cycleValue('x', -1));
      if (e.key === 'ArrowRight')
        return (e.preventDefault(), this.setValues(Math.min(1, this.valueX + s), null));
      if (e.key === 'ArrowLeft')
        return (e.preventDefault(), this.setValues(Math.max(0, this.valueX - s), null));
      if (e.key === 'Home' || (e.key === 'ArrowUp' && m))
        return (e.preventDefault(), this.cycleValue('y', 1));
      if (e.key === 'End' || (e.key === 'ArrowDown' && m))
        return (e.preventDefault(), this.cycleValue('y', -1));
      if (e.key === 'ArrowUp')
        return (e.preventDefault(), this.setValues(null, Math.min(1, this.valueY + s)));
      if (e.key === 'ArrowDown')
        return (e.preventDefault(), this.setValues(null, Math.max(0, this.valueY - s)));
      if (m && ['Backspace', 'Delete'].includes(e.key))
        return (e.preventDefault(), this.setValues(0.5, 0.5),
                UIUtils.updateStatus('Reset to default'));
    });
  }
  paint() {
    if (!this.width) return;
    const w = this.width, h = this.height, s = w / 140, lw = Palette.borderWidth * s;
    this.ctx.clearRect(0, 0, w, h);
    this.ctx.fillStyle = Palette.surface;
    this.ctx.fillRect(0, 0, w, h);

    this.ctx.fillStyle = `${Palette.foreground}80`;
    for (let x = 1; x < 8; ++x)
      for (let y = 1; y < 8; ++y) {
        this.ctx.beginPath();
        this.ctx.arc(x * w / 8, y * h / 8, 2 * lw, 0, Math.PI * 2);
        this.ctx.fill();
      }

    if (this.isHovered && !this.isEditing) {
      this.ctx.strokeStyle = Palette.main;
      this.ctx.lineWidth = lw;
      this.ctx.beginPath();
      this.ctx.moveTo(0, this.mousePos.y);
      this.ctx.lineTo(w, this.mousePos.y);
      this.ctx.moveTo(this.mousePos.x, 0);
      this.ctx.lineTo(this.mousePos.x, h);
      this.ctx.stroke();
    }
    const vx = this.valueX * w, vy = (1 - this.valueY) * h;
    this.ctx.strokeStyle = Palette.foreground;
    this.ctx.lineWidth = lw;
    this.ctx.beginPath();
    this.ctx.moveTo(0, vy);
    this.ctx.lineTo(w, vy);
    this.ctx.moveTo(vx, 0);
    this.ctx.lineTo(vx, h);
    this.ctx.stroke();
    this.ctx.beginPath();
    this.ctx.arc(vx, vy, 8 * lw, 0, Math.PI * 2);
    this.ctx.lineWidth = 2 * s;
    this.ctx.stroke();

    this.ctx.strokeStyle = this.isHovered || this.isEditing ? Palette.main : Palette.border;
    this.ctx.lineWidth = lw;
    this.ctx.strokeRect(0, 0, w, h);
    if (document.activeElement === this.canvas) {
      this.ctx.strokeStyle = `${Palette.main}80`;
      this.ctx.lineWidth = 2;
      this.ctx.strokeRect(1, 1, w - 2, h - 2);
    }
  }
}

class SnapToggle extends CanvasBase {
  constructor() {
    super();
    this.canvas.setAttribute('role', 'switch');
    this.canvas.setAttribute('aria-label', 'XY Pad Snap');
    this.canvas.style.cursor = 'pointer';
    this.canvas.addEventListener('mousedown', e => e.button === 0 && this.toggle());
    this.canvas.addEventListener(
      'keydown', e => ['Enter', ' '].includes(e.key) && (e.preventDefault(), this.toggle()));
  }
  init() {
    this.state = this.getAttribute('state') !== 'false';
    this.canvas.setAttribute('aria-checked', this.state);
  }
  toggle() {
    this.state = !this.state;
    this.canvas.setAttribute('aria-checked', this.state);
    this.dispatchEvent(new CustomEvent('change', {detail: {state: this.state}}));
    UIUtils.updateStatus(`XY Pad: Snap ${this.state ? 'ON' : 'OFF'}`);
    this.paint();
  }
  paint() {
    if (!this.width) return;
    const lw = Palette.borderWidth;
    this.ctx.clearRect(0, 0, this.width, this.height);
    this.ctx.strokeStyle = Palette.border;
    this.ctx.lineWidth = lw;
    this.ctx.strokeRect(lw / 2, lw / 2, this.width - lw, this.height - lw);
    if (this.state) {
      this.ctx.fillStyle = Palette.main;
      this.ctx.fillRect(4 * lw, 4 * lw, this.width - 8 * lw, this.height - 8 * lw);
    }
    if (document.activeElement === this.canvas) {
      this.ctx.strokeStyle = `${Palette.main}80`;
      this.ctx.lineWidth = 2;
      this.ctx.strokeRect(1, 1, this.width - 2, this.height - 2);
    }
  }
}

function createLockableLabel(text, cls, isLocked, onChange, getStatusText) {
  const btn = document.createElement('button');
  btn.className = cls;
  btn.textContent = text;
  const update = () => {
    btn.setAttribute('aria-pressed', isLocked);
    if (isLocked)
      btn.setAttribute('data-locked', 'true');
    else
      btn.removeAttribute('data-locked');
  };
  update();
  const hover = clear => UIUtils.updateStatus(clear ? '' : getStatusText(isLocked));
  ['mouseenter', 'focus'].forEach(e => btn.addEventListener(e, () => hover(false)));
  ['mouseleave', 'blur'].forEach(e => btn.addEventListener(e, () => hover(true)));
  btn.addEventListener('click', () => {
    isLocked = !isLocked;
    update();
    hover(false);
    onChange(isLocked);
  });
  return btn;
}

class LabeledXYPad extends HTMLElement {
  connectedCallback() {
    if (this._init) return;
    this._init = true;
    const addLbl = axis => {
      let lck
        = this.hasAttribute(`locked-${axis}`) && this.getAttribute(`locked-${axis}`) !== 'false';
      this[`isLocked${axis.toUpperCase()}`] = lck;
      return createLockableLabel(
        this.getAttribute(`label-${axis}`) || `${axis.toUpperCase()} Axis`, `label-${axis}`,
        lck, v => {
          this[`isLocked${axis.toUpperCase()}`] = v;
          this.dispatchEvent(new CustomEvent('lock-changed', {detail: {axis, locked: v}}));
        }, v => `${axis.toUpperCase()} Axis: Randomize ${v ? 'OFF' : 'ON'}`);
    };
    const tog = document.createElement('uhhyou-snap-toggle');
    tog.addEventListener(
      'change', e => this.querySelector('uhhyou-xy-pad')?.setSnappingEnabled(e.detail.state));
    this.prepend(tog, addLbl('x'), addLbl('y'));
  }
}

class LabeledWidget extends HTMLElement {
  connectedCallback() {
    if (this._init) return;
    this._init = true;
    this.layoutOption = this.getAttribute('layout') || 'showLabel';
    this.isLocked = this.layoutOption === 'expand'
      ? true
      : (this.hasAttribute('locked') && this.getAttribute('locked') !== 'false');
    this.setAttribute('layout', this.layoutOption);

    this.widgetContainer = document.createElement('div');
    this.widgetContainer.className = 'widget-container';
    while (this.firstChild) this.widgetContainer.appendChild(this.firstChild);

    if (this.layoutOption === 'showLabel') {
      const lblTxt = this.getAttribute('label') || 'Parameter';
      const btn = createLockableLabel(lblTxt, 'lockable-label', this.isLocked, v => {
        this.isLocked = v;
        btn.setAttribute('aria-label', `${lblTxt}, Randomize ${v ? 'off.' : 'on.'}`);
        this.dispatchEvent(new CustomEvent('lock-changed', {detail: {locked: v}}));
      }, v => `${lblTxt}: Randomize ${v ? 'OFF' : 'ON'}`);
      btn.setAttribute('aria-label', `${lblTxt}, Randomize ${this.isLocked ? 'off.' : 'on.'}`);

      const c = document.createElement('div');
      c.className = 'label-container';
      const u = document.createElement('div');
      u.className = 'underline';
      c.append(btn, u);
      this.appendChild(c);
    }
    this.appendChild(this.widgetContainer);
  }
}

class MenuBase extends CanvasWrapperBase {
  constructor() {
    super();
    this.isMenuOpen = false;
    this.menuContainer = document.createElement('div');
    this.menuContainer.tabIndex = -1;
    Object.assign(this.menuContainer.style, {
      position: 'absolute',
      top: '100%',
      left: '0',
      background: 'var(--table-p1-color, #f8f8f8)',
      border: '1px solid var(--border-color, #888)',
      boxShadow: '0 4px 12px color-mix(in srgb, var(--foreground-color) 20%, transparent)',
      display: 'none',
      zIndex: '100',
      maxHeight: '200px',
      overflowY: 'auto',
      fontFamily: 'var(--font-sans, sans-serif)',
      color: 'var(--foreground-color, #000)',
      boxSizing: 'border-box'
    });
    this.wrapper.appendChild(this.menuContainer);
    document.addEventListener('mousedown',
                              e => !this.contains(e.target) && this.isMenuOpen && this.closeMenu());
    this.canvas.addEventListener('blur',
                                 () => setTimeout(() => {
                                   if (this.isMenuOpen && !this.contains(document.activeElement))
                                     this.closeMenu();
                                 }, 150));
  }
  openMenu() {
    this.isMenuOpen = true;
    this.canvas.setAttribute('aria-expanded', 'true');
    this.menuContainer.style.display = 'block';
  }
  closeMenu() {
    this.isMenuOpen = false;
    this.canvas.setAttribute('aria-expanded', 'false');
    this.menuContainer.style.display = 'none';
  }
}

class ComboBox extends MenuBase {
  constructor() {
    super();
    this.idPrefix = `cb-${Math.random().toString(36).substr(2)}`;
    ['role', 'aria-haspopup', 'aria-expanded', 'aria-controls'].forEach(
      (a, i) => this.canvas.setAttribute(
        a, ['combobox', 'listbox', 'false', `${this.idPrefix}-list`][i]));
    this.canvas.style.cursor = 'pointer';
    this.canvas.oncontextmenu = () => false;
    this.announcer = UIUtils.createAriaAnnouncer();
    Object.assign(this.menuContainer, {id: `${this.idPrefix}-list`});
    this.menuContainer.setAttribute('role', 'listbox');
    this.menuContainer.style.width = '100%';
    this.bindComboEvents();
  }
  mountContent() {
    super.mountContent();
    this.wrapper.appendChild(this.announcer);
  }
  init() {
    this.items = (this.getAttribute('items') || 'Item 1').split(',').map(s => s.trim());
    this.itemIndex = this.defaultIndex = parseInt(this.getAttribute('value') || '0', 10);
    this.btnStyle = this.getAttribute('type') || 'common';
    const lbl = this.getAttribute('label') || 'Combo Box';
    this.canvas.setAttribute('aria-label', lbl);
    this.menuContainer.setAttribute('aria-label', `${lbl} Options`);
    this.canvas.setAttribute('aria-activedescendant', `${this.idPrefix}-item-${this.itemIndex}`);
    this.optionElements = this.items.map((it, i) => {
      const el = document.createElement('div');
      el.id = `${this.idPrefix}-item-${i}`;
      el.setAttribute('role', 'option');
      el.textContent = it;
      el.style.cursor = 'pointer';
      el.addEventListener('mouseenter', () => this.highlightOption(i));
      el.addEventListener('mouseleave', () => this.highlightOption(this.itemIndex));
      el.addEventListener('mousedown', e => {
        e.stopPropagation();
        this.setInternalValue(i);
        this.closeMenu();
      });
      this.menuContainer.appendChild(el);
      return el;
    });
  }
  onResize(w, h) {
    const s = h / 20;
    this.menuContainer.style.fontSize = `${13 * s}px`;
    this.menuContainer.style.padding = `${2 * s}px 0`;
    this.highlightOption(this.itemIndex);
  }
  updateStatusHover() {
    if (this.items?.length)
      UIUtils.updateStatus(`${this.getAttribute('label') || 'ComboBox'}: ${
        this.items[this.itemIndex]} (${this.itemIndex + 1}/${this.items.length})`);
  }
  highlightOption(idx) {
    const s = this.height ? this.height / 20 : 1;
    this.optionElements?.forEach((el, i) => {
      el.setAttribute('aria-selected', i === this.itemIndex);
      Object.assign(el.style, {
        padding: `${4 * s}px ${8 * s}px`,
        background: i === idx ? 'var(--component-main, #fcc04f)' : 'transparent',
        color: i === idx ? '#000' : 'inherit'
      });
    });
  }
  setInternalValue(i) {
    if (i < 0 || i >= this.items.length || i === this.itemIndex) return;
    this.itemIndex = i;
    this.canvas.setAttribute('aria-activedescendant', `${this.idPrefix}-item-${i}`);
    this.announcer.textContent = this.items[i];
    this.updateStatusHover();
    this.highlightOption(i);
    this.paint();
    this.dispatchEvent(
      new CustomEvent('value-changed', {detail: {index: i, value: this.items[i]}}));
  }
  openMenu() {
    super.openMenu();
    this.highlightOption(this.itemIndex);
    setTimeout(() => this.optionElements[this.itemIndex]?.scrollIntoView({block: 'nearest'}), 0);
  }
  bindComboEvents() {
    this.canvas.addEventListener('mouseenter', () => {
      this.updateStatusHover();
      this.paint();
    });
    this.canvas.addEventListener('mouseleave', () => UIUtils.updateStatus(''));
    this.canvas.addEventListener('mousedown', e => {
      if (e.button === 2) return UIUtils.updateStatus('Host Context Menu opened');
      if (e.metaKey || e.ctrlKey)
        return (e.preventDefault(), this.setInternalValue(this.defaultIndex), this.closeMenu());
      if (e.button === 0) {
        this.canvas.focus();
        this.isMenuOpen ? this.closeMenu() : this.openMenu();
      }
    });
    this.canvas.addEventListener('wheel', e => {
      if (Math.abs(e.deltaY) < 0.001 || !this.items?.length) return;
      e.preventDefault();
      const n = (this.itemIndex + Math.sign(e.deltaY) + this.items.length) % this.items.length;
      this.setInternalValue(n);
      if (this.isMenuOpen) this.optionElements[n]?.scrollIntoView({block: 'nearest'});
    });
    this.canvas.addEventListener('keydown', e => {
      if (e.key === 'Escape' && this.isMenuOpen) return (e.preventDefault(), this.closeMenu());
      if (['Enter', ' '].includes(e.key))
        return (e.preventDefault(), this.isMenuOpen ? this.closeMenu() : this.openMenu());
      if ((e.metaKey || e.ctrlKey) && ['Backspace', 'Delete'].includes(e.key))
        return (e.preventDefault(), this.setInternalValue(this.defaultIndex), this.closeMenu());
      if (e.key === 'Home')
        return (e.preventDefault(), this.setInternalValue(0),
                this.isMenuOpen && this.optionElements[0]?.scrollIntoView({block: 'nearest'}));
      if (e.key === 'End') {
        const end = this.items.length - 1;
        return (e.preventDefault(), this.setInternalValue(end),
                this.isMenuOpen && this.optionElements[end]?.scrollIntoView({block: 'nearest'}));
      }
      if (e.key.length === 1 && !e.ctrlKey && !e.metaKey && !e.altKey) {
        const c = e.key.toLowerCase();
        for (let i = 1; i <= this.items.length; i++) {
          let idx = (this.itemIndex + i) % this.items.length;
          if (this.items[idx].toLowerCase().startsWith(c))
            return (this.setInternalValue(idx),
                    this.isMenuOpen
                      && this.optionElements[idx]?.scrollIntoView({block: 'nearest'}));
        }
      }
      const d = ({ArrowUp: -1, ArrowLeft: -1, ArrowDown: 1, ArrowRight: 1})[e.key] || 0;
      if (d && this.items?.length) {
        e.preventDefault();
        const n = (this.itemIndex + d + this.items.length) % this.items.length;
        this.setInternalValue(n);
        if (this.isMenuOpen) this.optionElements[n]?.scrollIntoView({block: 'nearest'});
      }
    });
  }
  paint() {
    if (!this.width) return;
    const s = this.height / 20, lw = Palette.borderWidth * s;
    this.ctx.clearRect(0, 0, this.width, this.height);
    UIUtils.drawRoundedBox(
      this.ctx, lw / 2, lw / 2, this.width - lw, this.height - lw, lw * 2, Palette.surface,
      this.isHovered ? (Palette[this.btnStyle] || Palette.main) : Palette.border, lw);
    if (document.activeElement === this.canvas)
      UIUtils.drawRoundedBox(this.ctx, 1, 1, this.width - 2, this.height - 2, lw * 2, null,
                             `${Palette.main}80`, 2);
    if (this.items && this.itemIndex >= 0) {
      this.ctx.fillStyle = Palette.foreground;
      this.ctx.font = `${14 * s}px ${Palette.fontSans}`;
      this.ctx.textAlign = 'center';
      this.ctx.textBaseline = 'middle';
      this.ctx.fillText(this.items[this.itemIndex], this.width / 2, (this.height / 2) + s);
    }
  }
}

const MockFileSystem = {
  "Factory Presets": {
    "Basses": {"Deep Sub.xml": "file", "FM Pluck.xml": "file"},
    "Pads": {"Warm Strings.xml": "file", "Glassy Choir.xml": "file"},
    "Init Preset.xml": "file"
  },
  "User Presets": {"Alpha.xml": "file", "Beta.xml": "file", "Gamma.xml": "file"},
  "BuzzSaw.xml": "file",
  "SlapBass.xml": "file",
  "Theremin.xml": "file"
};

class PresetManager extends MenuBase {
  constructor() {
    super();
    this.idPrefix = `pm-${Math.random().toString(36).substr(2)}`;
    this.hoverRegion = 'none';
    this.currentMenuPath = [];
    this.menuItems = [];
    this.activeMenuIndex = -1;
    ['role', 'aria-expanded', 'aria-haspopup'].forEach(
      (a, i) => this.canvas.setAttribute(a, ['combobox', 'false', 'menu'][i]));
    this.menuContainer.setAttribute('role', 'menu');
    this.menuContainer.style.left = '50%';
    this.menuContainer.style.transform = 'translateX(-50%)';
    this.menuContainer.style.width = '90%';
    this.announcer = UIUtils.createAriaAnnouncer();
    this.bindPresetEvents();
  }
  mountContent() {
    super.mountContent();
    this.wrapper.appendChild(this.announcer);
  }
  init() {
    this.currentPreset = this.getAttribute('preset') || 'Default';
    this.canvas.setAttribute('aria-label', `Preset: ${this.currentPreset}`);
  }
  onResize(w, h) {
    this.bw = Math.min(h, w / 3);
    this.scale = h / 20;
    this.menuContainer.style.fontSize = `${13 * this.scale}px`;
    this.menuContainer.style.padding = `${4 * this.scale}px 0`;
  }

  highlightMenuItem(idx, ctxMsg = '') {
    this.menuItems.forEach((it, i) => it.el.style.background
                           = i === idx ? Palette.main : 'transparent');
    this.activeMenuIndex = idx;
    if (idx >= 0 && this.menuItems[idx]) {
      this.canvas.setAttribute('aria-activedescendant', this.menuItems[idx].el.id);
      const txt = this.menuItems[idx].el.getAttribute('aria-label')
        || this.menuItems[idx].el.textContent.replace(' >', ' folder');
      this.announcer.textContent = (ctxMsg ? `${ctxMsg}. ` : '') + txt;
    } else
      this.canvas.removeAttribute('aria-activedescendant');
  }

  openMenu(path = [], ctxMsg = '') {
    super.openMenu();
    this.currentMenuPath = path;
    this.menuContainer.innerHTML = '';
    this.menuItems = [];
    this.activeMenuIndex = -1;
    const mName = path.length ? path[path.length - 1] : 'Root';
    this.menuContainer.setAttribute('aria-label', `Preset Menu, ${mName}`);
    const dir = path.reduce((c, f) => c[f], MockFileSystem);

    const addItem = (t, isH = false, isB = false, hasA = false, onClick = null) => {
      const el = document.createElement('div');
      el.textContent = t + (hasA ? ' >' : '');
      el.setAttribute('role', isH ? 'presentation' : 'menuitem');
      if (!isH) el.tabIndex = -1;
      Object.assign(el.style, {
        padding: `${6 * this.scale}px ${12 * this.scale}px`,
        cursor: isH ? 'default' : 'pointer',
        fontWeight: isH ? 'bold' : 'normal',
        opacity: isH ? '0.7' : '1',
        borderBottom: isH ? '1px solid color-mix(in srgb, var(--border-color) 30%, transparent)'
                          : 'none'
      });
      if (!isH) {
        const idx = this.menuItems.length;
        el.id = `${this.idPrefix}-item-${idx}`;
        if (hasA) {
          el.setAttribute('aria-haspopup', 'menu');
          el.setAttribute('aria-expanded', 'false');
        }
        if (isB)
          el.setAttribute('aria-label',
                          `Go back to ${path.length > 1 ? path[path.length - 2] : 'Root'}`);
        el.addEventListener('mouseenter', () => this.highlightMenuItem(idx));
        el.addEventListener('mouseleave', () => {
          if (this.activeMenuIndex === idx) this.highlightMenuItem(-1);
        });
        if (onClick)
          el.addEventListener('mousedown', e => {
            e.stopPropagation();
            onClick();
          });
        this.menuItems.push({el, action: onClick, isBack: isB, hasArrow: hasA});
      }
      this.menuContainer.appendChild(el);
    };

    const setPreset = n => {
      this.currentPreset = n.replace('.xml', '');
      this.canvas.setAttribute('aria-label', `Preset: ${this.currentPreset}`);
      this.paint();
      this.closeMenu();
    };

    if (!path.length) {
      addItem('Preset', true);
      ['Save', 'Load', 'Refresh'].forEach(
        a => addItem(a, false, false, false, () => setPreset(`Action: ${a}`)));
      addItem('', true);
    } else {
      addItem(mName, true);
      addItem(
        '.. (Back)', false, true, false,
        () => this.openMenu(path.slice(0, -1),
                            `Returned to ${path.length > 1 ? path[path.length - 2] : 'Root'}`));
      addItem('', true);
    }

    for (const [k, v] of Object.entries(dir))
      v === 'file'
        ? addItem(k.replace('.xml', ''), false, false, false, () => setPreset(k))
        : addItem(k, false, false, true, () => this.openMenu([...path, k], `Entered ${k}`));

    if (this.menuItems.length)
      this.highlightMenuItem(0, ctxMsg || (path.length === 0 ? 'Opened Preset Menu' : ''));
  }

  updateHoverRegion(x) {
    this.hoverRegion = x < this.bw ? 'prev' : (x > this.width - this.bw ? 'next' : 'center');
    UIUtils.updateStatus(this.hoverRegion === 'prev'
                           ? 'Previous preset'
                           : (this.hoverRegion === 'next' ? 'Next preset' : 'Preset menu'));
  }
  cyclePreset(dir) {
    this.currentPreset = dir > 0 ? 'Next Preset...' : 'Previous Preset...';
    this.canvas.setAttribute('aria-label', `Preset: ${this.currentPreset}`);
    this.paint();
  }

  bindPresetEvents() {
    this.canvas.addEventListener('mousemove', e => {
      this.updateHoverRegion(e.offsetX);
      this.canvas.style.cursor = 'pointer';
      this.paint();
    });
    this.canvas.addEventListener('mouseleave', () => {
      this.hoverRegion = 'none';
      UIUtils.updateStatus('');
      this.paint();
    });
    ['focus', 'blur'].forEach((ev, i) => this.canvas.addEventListener(ev, () => {
      UIUtils.updateStatus(i ? '' : 'Preset menu');
      this.paint();
    }));

    this.canvas.addEventListener('mousedown', () => {
      if (this.isMenuOpen) return this.closeMenu();
      this.canvas.focus();
      this.hoverRegion === 'center' ? this.openMenu()
                                    : this.cyclePreset(this.hoverRegion === 'prev' ? -1 : 1);
    });

    this.canvas.addEventListener('keydown', e => {
      if (this.isMenuOpen) {
        if (e.key === 'Escape') return (e.preventDefault(), this.closeMenu());
        if (e.key === 'ArrowDown') {
          e.preventDefault();
          const n = (this.activeMenuIndex + 1) % this.menuItems.length;
          return (this.highlightMenuItem(n),
                  this.menuItems[n]?.el.scrollIntoView({block: 'nearest'}));
        }
        if (e.key === 'ArrowUp') {
          e.preventDefault();
          const p = (this.activeMenuIndex - 1 + this.menuItems.length) % this.menuItems.length;
          return (this.highlightMenuItem(p),
                  this.menuItems[p]?.el.scrollIntoView({block: 'nearest'}));
        }
        if (e.key === 'ArrowRight')
          return (e.preventDefault(),
                  this.activeMenuIndex >= 0 && this.menuItems[this.activeMenuIndex].hasArrow
                    && this.menuItems[this.activeMenuIndex].action());
        if (e.key === 'ArrowLeft')
          return (e.preventDefault(),
                  this.currentMenuPath.length > 0
                    && this.openMenu(this.currentMenuPath.slice(0, -1),
                                     `Returned to ${
                                       this.currentMenuPath.length > 1
                                         ? this.currentMenuPath[this.currentMenuPath.length - 2]
                                         : 'Root'}`));
        if (['Enter', ' '].includes(e.key))
          return (e.preventDefault(),
                  this.activeMenuIndex >= 0 ? this.menuItems[this.activeMenuIndex].action()
                                            : this.closeMenu());
      } else {
        if (['ArrowLeft', 'ArrowRight'].includes(e.key))
          return (e.preventDefault(), this.cyclePreset(e.key === 'ArrowLeft' ? -1 : 1));
        if (['Enter', 'ArrowDown', ' '].includes(e.key))
          return (e.preventDefault(), this.openMenu());
      }
    });
  }

  drawArrow(x, isLeft) {
    const s = Math.min(this.bw, this.height / 2), cx = x + this.bw / 2, cy = this.height / 2;
    this.ctx.fillStyle = Palette.foreground;
    this.ctx.beginPath();
    (isLeft ? [[0, 0.5], [1, 0], [0.62, 0.5], [1, 1]] : [
      [1, 0.5], [0, 0], [0.38, 0.5], [0, 1]
    ]).forEach((pt, i) => {
      const px = cx + (pt[0] - 0.5) * s, py = cy + (pt[1] - 0.5) * s;
      i === 0 ? this.ctx.moveTo(px, py) : this.ctx.lineTo(px, py);
    });
    this.ctx.fill();
  }

  paint() {
    if (!this.width) return;
    const lw = Palette.borderWidth * this.scale, c = 2 * lw;
    this.ctx.clearRect(0, 0, this.width, this.height);
    UIUtils.drawRoundedBox(this.ctx, lw / 2, lw / 2, this.width - lw, this.height - lw, c,
                           Palette.surface, Palette.border, lw);
    if (this.hoverRegion !== 'none') {
      this.ctx.fillStyle = Palette.main;
      this.ctx.beginPath();
      const hw = this.hoverRegion === 'prev'
        ? [lw / 2, this.bw - lw / 2]
        : (this.hoverRegion === 'next' ? [this.width - this.bw, this.bw - lw / 2]
                                       : [this.bw, this.width - this.bw * 2]);
      this.ctx.roundRect(hw[0], lw / 2, hw[1], this.height - lw, c);
      this.ctx.fill();
    }
    if (document.activeElement === this.canvas)
      UIUtils.drawRoundedBox(this.ctx, 1, 1, this.width - 2, this.height - 2, c, null,
                             `${Palette.main}80`, 2);
    this.ctx.fillStyle = Palette.foreground;
    this.ctx.font = `${14 * this.scale}px ${Palette.fontSans}`;
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText(this.currentPreset, this.width / 2, (this.height / 2) + this.scale);
    this.drawArrow(0, true);
    this.drawArrow(this.width - this.bw, false);
  }
}

class DrawerToggle extends CanvasBase {
  constructor() {
    super();
    this.canvas.setAttribute('role', 'button');
    this.canvas.style.cursor = 'pointer';
    this.canvas.addEventListener('mousedown', e => {
      if (e.button === 0) {
        this.canvas.focus();
        this.toggle();
      }
    });
    this.canvas.addEventListener(
      'keydown', e => ['Enter', ' '].includes(e.key) && (e.preventDefault(), this.toggle()));
    this.canvas.addEventListener(
      'mouseenter', () => UIUtils.updateStatus(`Drawer: ${this.isOpen ? 'Open' : 'Closed'}`));
    this.canvas.addEventListener('mouseleave', () => UIUtils.updateStatus(''));
  }
  init() {
    this.isOpen = this.getAttribute('open') !== 'false';
    this.name = this.getAttribute('name') || '';
    this.canvas.setAttribute('aria-expanded', this.isOpen);
    this.canvas.setAttribute('aria-label', `${this.name}, Drawer`);
    this.canvas.setAttribute('aria-description', 'Press Space or Enter to toggle the drawer.');
  }
  toggle() {
    this.isOpen = !this.isOpen;
    this.canvas.setAttribute('aria-expanded', this.isOpen);
    this.paint();
    this.dispatchEvent(new CustomEvent('toggle', {detail: {isOpen: this.isOpen}, bubbles: true}));
    UIUtils.updateStatus(`Drawer: ${this.isOpen ? 'Open' : 'Closed'}`);
  }
  paint() {
    if (!this.width) return;
    this.ctx.clearRect(0, 0, this.width, this.height);
    this.ctx.fillStyle = Palette.surface;
    this.ctx.fillRect(0, 0, this.width, this.height);

    if (this.isHovered || document.activeElement === this.canvas) {
      this.ctx.fillStyle = `${Palette.main}4D`;
      this.ctx.fillRect(0, 0, this.width, this.height);
    }

    this.ctx.strokeStyle = Palette.foreground;
    this.ctx.lineWidth = Palette.borderWidth * 1.5;
    this.ctx.lineJoin = 'miter';
    this.ctx.lineCap = 'round';

    const w = this.width, h = this.height, cx = w / 2, cy = h / 2, size = w * 0.25;
    this.ctx.beginPath();
    if (this.isOpen) {
      this.ctx.moveTo(cx + size, cy - size * 1.5);
      this.ctx.lineTo(cx - size, cy);
      this.ctx.lineTo(cx + size, cy + size * 1.5);
    } else {
      this.ctx.moveTo(cx - size, cy - size * 1.5);
      this.ctx.lineTo(cx + size, cy);
      this.ctx.lineTo(cx - size, cy + size * 1.5);
    }
    this.ctx.stroke();
  }
}

class HorizontalDrawer extends HTMLElement {
  connectedCallback() {
    if (this._init) return;
    this._init = true;

    this.isOpen = this.getAttribute('open') !== 'false';
    const name = this.getAttribute('name') || 'Drawer';

    this.contentContainer = document.createElement('div');
    this.contentContainer.className = 'drawer-content';
    while (this.firstChild) this.contentContainer.appendChild(this.firstChild);

    this.toggleBtn = document.createElement('uhhyou-drawer-toggle');
    this.toggleBtn.setAttribute('name', name);
    this.toggleBtn.setAttribute('open', this.isOpen ? 'true' : 'false');

    const updateBg = (active) => {
      if (this.isOpen && active) {
        this.contentContainer.style.backgroundColor
          = 'color-mix(in srgb, var(--component-main, #fcc04f) 10%, transparent)';
      } else {
        this.contentContainer.style.backgroundColor = 'transparent';
      }
    };

    this.toggleBtn.addEventListener('toggle', e => {
      this.isOpen = e.detail.isOpen;
      if (this.isOpen) {
        this.contentContainer.style.display = '';
      } else {
        this.contentContainer.style.display = 'none';
      }
      if (!this.isOpen && this.contentContainer.contains(document.activeElement)) {
        this.toggleBtn.canvas.focus();
      }
      updateBg(this.toggleBtn.isHovered || document.activeElement === this.toggleBtn.canvas);
    });

    this.toggleBtn.addEventListener('mouseenter', () => updateBg(true));
    this.toggleBtn.addEventListener(
      'mouseleave', () => updateBg(document.activeElement === this.toggleBtn.canvas));
    this.toggleBtn.addEventListener('focusin', () => {
      this.toggleBtn.paint();
      updateBg(true);
    });
    this.toggleBtn.addEventListener('focusout', () => {
      this.toggleBtn.paint();
      updateBg(this.toggleBtn.isHovered);
    });

    if (!this.isOpen) this.contentContainer.style.display = 'none';

    this.appendChild(this.toggleBtn);
    this.appendChild(this.contentContainer);
  }
}

class TabView extends CanvasBase {
  constructor() {
    super();
    this.activeTabIndex = 0;
    this.scale = 1.0;
    this.tabHeight = 28;
    this.canvas.setAttribute('role', 'group');
    this.canvas.setAttribute('aria-roledescription', 'Tab View');
    this.bindTabEvents();
  }
  init() {
    this.tabs = (this.getAttribute('tabs') || 'Tab 1')
                  .split(',')
                  .map(l => ({label: l.trim(), isHovered: false}));
    this.updateAria();
    this.wrapper = document.createElement('div');
    this.wrapper.id = 'content-wrapper';
    const userTabs = Array.from(this.children).filter(c => c !== this.canvas);
    this.tabs.forEach((_, i) => {
      const sw = document.createElement('div');
      sw.className = `slot-wrapper ${i === this.activeTabIndex ? 'active' : ''}`;
      const child = userTabs.find(c => c.getAttribute('slot') === `tab-${i}`);
      if (child) {
        child.removeAttribute('slot');
        sw.appendChild(child);
      }
      this.wrapper.appendChild(sw);
    });
    this.appendChild(this.wrapper);
  }
  onResize() {
    this.scale = parseFloat(window.getComputedStyle(this).getPropertyValue('--scale')) || 1.0;
    this.tabHeight = 28 * this.scale;
  }
  updateAria() {
    if (this.tabs?.length)
      this.canvas.setAttribute('aria-label',
                               `Tab: ${this.tabs[this.activeTabIndex].label} (${
                                 this.activeTabIndex + 1} of ${this.tabs.length})`);
  }
  setActiveTab(i) {
    if (this.activeTabIndex === i) return;
    const sw = this.querySelectorAll('.slot-wrapper');
    if (sw[this.activeTabIndex]) sw[this.activeTabIndex].classList.remove('active');
    this.activeTabIndex = i;
    if (sw[i]) sw[i].classList.add('active');
    this.updateAria();
    this.paint();
  }
  getTabRect(i) {
    return {
      x: i * (this.width / this.tabs.length),
      y: 0,
      w: this.width / this.tabs.length,
      h: this.tabHeight
    };
  }
  bindTabEvents() {
    this.canvas.addEventListener('mousemove', e => {
      let cur = false;
      this.tabs?.forEach((t, i) => {
        const r = this.getTabRect(i);
        t.isHovered = e.offsetX >= r.x && e.offsetX <= r.x + r.w && e.offsetY >= r.y
          && e.offsetY <= r.y + r.h;
        if (t.isHovered && i !== this.activeTabIndex) cur = true;
      });
      this.canvas.style.cursor = cur ? 'pointer' : 'default';
      this.paint();
    });
    this.canvas.addEventListener('mouseleave', () => {
      this.tabs?.forEach(t => t.isHovered = false);
      this.canvas.style.cursor = 'default';
      this.paint();
    });
    this.canvas.addEventListener('mousedown', e => {
      this.canvas.focus();
      this.tabs?.forEach((t, i) => {
        const r = this.getTabRect(i);
        if (e.offsetX >= r.x && e.offsetX <= r.x + r.w && e.offsetY >= r.y
            && e.offsetY <= r.y + r.h)
        {
          this.canvas.style.cursor = 'default';
          this.setActiveTab(i);
        }
      });
    });
    this.canvas.addEventListener('wheel', e => {
      if (e.offsetY > this.tabHeight || Math.abs(e.deltaY) < 0.001 || !this.tabs) return;
      e.preventDefault();
      this.setActiveTab((this.activeTabIndex + Math.sign(e.deltaY) + this.tabs.length)
                        % this.tabs.length);
    });
    this.canvas.addEventListener('keydown', e => {
      if (!this.tabs) return;
      if (['ArrowLeft', 'ArrowRight'].includes(e.key))
        return (e.preventDefault(),
                this.setActiveTab(
                  (this.activeTabIndex + (e.key === 'ArrowLeft' ? -1 : 1) + this.tabs.length)
                  % this.tabs.length));
    });
  }
  paint() {
    if (!this.width || !this.tabs) return;
    const lw = Palette.borderWidth * this.scale, lwh = lw / 2;
    this.ctx.clearRect(0, 0, this.width, this.height);
    this.ctx.lineWidth = lw;
    this.ctx.lineJoin = this.ctx.lineCap = 'round';
    this.ctx.font = `${14 * this.scale}px ${Palette.fontSans}`;
    this.ctx.textBaseline = 'middle';
    this.ctx.textAlign = 'center';

    this.tabs.forEach((t, i) => {
      if (i === this.activeTabIndex) return;
      const r = this.getTabRect(i);
      this.ctx.fillStyle = t.isHovered ? `${Palette.main}4D` : Palette.surface;
      this.ctx.fillRect(r.x, r.y, r.w, r.h);
      this.ctx.beginPath();
      this.ctx.moveTo(r.x + (i === 0 ? lwh : -lwh), r.y + lwh);
      this.ctx.lineTo(r.x + r.w + (i >= this.tabs.length - 1 ? -lwh : lwh), r.y + lwh);
      this.ctx.lineTo(r.x + r.w + lwh, r.h);
      this.ctx.lineTo(r.x - lwh, r.h);
      this.ctx.closePath();
      this.ctx.strokeStyle = Palette.border;
      this.ctx.stroke();
      this.ctx.fillStyle = `${Palette.foreground}80`;
      this.ctx.fillText(t.label, r.x + r.w / 2, r.y + r.h / 2);
    });

    const aR = this.getTabRect(this.activeTabIndex);
    this.ctx.beginPath();
    this.ctx.moveTo(lwh, this.tabHeight);
    this.ctx.lineTo(aR.x + lwh, this.tabHeight);
    this.ctx.lineTo(aR.x + lwh, lwh);
    this.ctx.lineTo(aR.x + aR.w - lwh, lwh);
    this.ctx.lineTo(aR.x + aR.w - lwh, this.tabHeight);
    this.ctx.lineTo(this.width - lwh, this.tabHeight);
    this.ctx.lineTo(this.width - lwh, this.height - lwh);
    this.ctx.lineTo(lwh, this.height - lwh);
    this.ctx.closePath();
    this.ctx.fillStyle = Palette.background;
    this.ctx.fill();
    this.ctx.strokeStyle = Palette.border;
    this.ctx.stroke();
    this.ctx.fillStyle = Palette.foreground;
    this.ctx.fillText(this.tabs[this.activeTabIndex].label, aR.x + aR.w / 2, aR.y + aR.h / 2);
    if (document.activeElement === this.canvas) {
      const ins = 3 * this.scale;
      this.ctx.strokeRect(aR.x + ins, aR.y + ins, aR.w - ins * 2, aR.h - ins * 2);
    }
  }
}

const components = {
  'uhhyou-drawer-toggle': DrawerToggle,
  'uhhyou-horizontal-drawer': HorizontalDrawer,
  'uhhyou-snap-toggle': SnapToggle,
  'uhhyou-toggle-button': ToggleButton,
  'uhhyou-momentary-button': MomentaryButton,
  'uhhyou-knob': Knob,
  'uhhyou-rotary-knob': RotaryKnob,
  'uhhyou-text-knob': TextKnob,
  'uhhyou-rotary-text-knob': RotaryTextKnob,
  'uhhyou-xy-pad': XYPad,
  'uhhyou-labeled-xypad': LabeledXYPad,
  'uhhyou-combo-box': ComboBox,
  'uhhyou-preset-manager': PresetManager,
  'uhhyou-tab-view': TabView,
  'uhhyou-labeled-widget': LabeledWidget
};

Object.entries(components).forEach(([tag, cls]) => customElements.define(tag, cls));
const componentSelector = Object.keys(components).join(',');

function repaintAllWidgets() {
  document.querySelectorAll(componentSelector).forEach(el => el.paint?.());
}

const themeObserver = new MutationObserver(m => {
  if (m.some(x => x.attributeName === 'data-theme'))
    requestAnimationFrame(() => {
      syncPalette();
      repaintAllWidgets();
    });
});
themeObserver.observe(document.documentElement,
                      {attributes: true, attributeFilter: ['data-theme']});

function randomizeUnlockedParameters(container) {
  if (!container) return;
  container.querySelectorAll('uhhyou-labeled-widget, uhhyou-labeled-xypad').forEach(w => {
    if (w.tagName.toLowerCase() === 'uhhyou-labeled-xypad') {
      const p = w.querySelector('uhhyou-xy-pad');
      if (p)
        p.setValues(w.isLockedX ? p.valueX : Math.random(), w.isLockedY ? p.valueY : Math.random());
      return;
    }
    if (w.isLocked) return;
    const c = w.querySelector(
      'uhhyou-text-knob, uhhyou-rotary-knob, uhhyou-knob, uhhyou-rotary-text-knob, uhhyou-combo-box, uhhyou-xy-pad, uhhyou-toggle-button');
    if (!c) return;
    const t = c.tagName.toLowerCase();
    if (t.includes('knob'))
      c.setValue(Math.random());
    else if (t === 'uhhyou-combo-box')
      c.setInternalValue(Math.floor(Math.random() * c.items.length));
    else if (t === 'uhhyou-xy-pad')
      c.setValues(Math.random(), Math.random());
    else if (t === 'uhhyou-toggle-button' && c.value !== (Math.random() > 0.5 ? 1 : 0))
      c.toggleValue();
  });
}
