# NBS Pro Dark

> **Vibe:** Near-black mastering studio hardware with dual accent color coding — precision meets warmth.
>
> **Source:** Created from NBS_DynaDrive mockup v8
>
> **Best For:** Saturation, dynamics, EQ, channel strip, mix bus processors — anything that demands studio authority.

---

## Visual Identity

A near-black panel that feels like premium outboard mastering gear with hardware-level density. The defining visual concepts are a **dual accent color system** (functional sections coded with distinct accent colors, e.g., red for saturation, blue for dynamics) and **zero wasted space** — every pixel either contains a control, a display, or serves a visual grouping purpose. Backlit arc knobs with section-colored glow rings give it a modern hardware crossover feel — like an API 2500 or Neve channel strip designed for the screen. The overall impression is dark, precise, dense, powerful, and expensive.

---

## Color System

### Primary Palette

**Background Colors:**
- Main background: Near-black with a faint warmth, not pure black — `#141414`
- Surface/panel background: Slightly lifted dark gray for sections — `#1e1e1e`
- Elevated surfaces: Deep scope/display background, darker than panel — `#0e0e0e`

**Accent Colors:**
- Primary accent: Deep warm red for the first functional section — `#c0392b`
- Secondary accent: Cool medium blue for the second functional section — `#4a90d9`
- Neutral accent: Muted gray for utility controls (gain, mix, output) — `#888888`
- Toggle active (primary): Matches secondary accent when in primary state — `#4a90d9`
- Toggle active (alternate): Deep dark red for alternate state — `#8b1a1a`

**Text Colors:**
- Primary text: Bright off-white for plugin name and prominent labels — `#e8e8e8`
- Secondary/muted text: Mid-gray for knob labels and section headers — `#888888`
- Value readout: Dim gray for parameter value displays — `#555555`
- Faint text: Very dim for taglines and decorative text — `#424242`

### Control Colors

**Knobs/Rotary Controls:**
- Base color: Near-black circular body with subtle radial gradient from `#2c2c2c` center highlight to `#141414` edge
- Pointer/indicator: Small colored dot on the knob face matching section accent
- Active state: Glowing arc ring around knob in section accent color with subtle blur glow

**Sliders:**
- Not used in this aesthetic — all continuous controls are rotary knobs

**Buttons/Toggles:**
- Default state: Dark gray rounded pill (`#2a2a2a`) with subtle border (`#3a3a3a`)
- Active/on state: Filled with accent color, white text, colored glow shadow
- Hover state: Slightly brighter background, tinted border

### Philosophy

High contrast dual-accent on near-black. The red and blue accents are never mixed — they define separate functional zones. Utility controls (gain, mix) use neutral gray to avoid competing with the section colors. The overall palette is monochromatic (grays) with two carefully chosen accent colors that create visual hierarchy through color association rather than size or position alone.

---

## Typography

**Font Families:**
- Headings: Clean system sans-serif — `Inter`, `SF Pro Display`, system stack
- Body text: Same sans-serif family, lighter weight
- Values/numbers: Same family — no monospace needed, values are short

**Font Sizing:**
- Plugin title: Medium-large, prominent but not oversized — `17-18px`
- Section labels: Very small uppercase with wide tracking — `9-10px`
- Parameter labels: Small uppercase — `10-11px`
- Parameter values: Smaller than labels, dimmer — `9-10px`

**Font Styling:**
- Title weight: Semi-bold — `600`
- Label weight: Medium to bold — `500-700`
- Letter spacing: Wide tracking on labels (`0.10-0.18em`), moderate on title (`0.08em`)
- Text transform: Uppercase for all section headers and control labels — lowercase only for taglines

**Philosophy:**
Technical and precise. Typography resembles silk-screened panel markings on hardware — small, tracked-out uppercase labels that feel etched rather than displayed. The hierarchy is created through size, weight, and color opacity rather than decorative treatment.

---

## Controls

### Knob Style

**Visual Design:**
- Shape: Perfect circle
- Size: Three tiers — large (55px) for primary controls, medium (40px) for secondary, small (35px) for utility
- Indicator style: 270° backlit arc ring around the knob body, glowing in section accent color. The arc fills from bottom-left (135°) clockwise to bottom-right (45°) as value increases.
- Border treatment: Subtle 1.2px stroke in very dark gray (`#0a0a0a`) on knob body

**Surface Treatment:**
- Base appearance: Radial gradient — slightly brighter center-left (`#2c2c2c`) fading to near-black (`#141414`). Creates subtle 3D depth without skeuomorphism.
- Depth: Subtle 3D — drop shadow on the SVG container (`0 2px 5px rgba(0,0,0,0.7)`)
- Tick marks: None — the arc ring IS the value indicator
- Center indicator: Small colored dot (1.8px radius) on the knob face, positioned at the current value angle, matching the arc accent color

**Interaction Feel:**
Smooth vertical drag — drag up to increase, drag down to decrease. Tooltip appears near cursor showing parameter name and current value. Double-click resets to default. The arc ring updates in real-time during drag with a subtle glow filter, creating a "backlit LED" feel.

**Special Variant — Circle Fill Knob:**
For specific controls (Mix, Input, Output), a 360° full-circle indicator replaces the standard 270° arc. The fill sweeps clockwise from 12 o'clock. At 0% the ring is dim/empty; at 100% the full circle is lit. This visually distinguishes gain/blend controls from processing knobs.

### Slider Style

**Layout:**
- Not applicable — this aesthetic uses rotary knobs exclusively

### Button Style

**Shape & Size:**
- Shape: Rounded pill (border-radius: 50px)
- Padding: Comfortable — 140×36px for primary toggles
- Aspect ratio: Wide horizontal pill

**Visual Design:**
- Default state: Dark fill (`#2a2a2a`), subtle border (`#3a3a3a`), muted text
- Active state: Filled with accent color, white text, colored glow box-shadow (`0 0 14px rgba(accent, 0.45)`)
- Border: 1px solid, color matches state

**Typography:**
- Label style: Semi-bold, 11px, slight letter spacing (`0.08em`)
- Icon treatment: Arrow symbol (→) between state labels (e.g., "DYN → SAT")

---

## Spacing & Layout Philosophy

**Overall Density:**
Dense and hardware-tight — every pixel serves a purpose. No blank space anywhere. Controls are packed like a hardware channel strip where every millimeter of faceplate has a function. This is the defining characteristic of the aesthetic: maximum information density without feeling cluttered.

**Control Spacing:**
- Between controls: Tight — 10-14px gap between knobs in a row
- Vertical rhythm: Primary knob at top, secondary knobs below, visual display (curve/scope) fills remaining space. No empty padding between elements.
- Grouping strategy: Functional sections defined by vertical divider lines with section-colored gradient wash at the top (`rgba(accent, 0.04)` fading to transparent). Frequently-used controls promoted from advanced panel to main panel to reduce reliance on collapsible sections.

**Padding & Margins:**
- Edge margins: 8-12px padding inside sections
- Section padding: 8-10px top, 6-8px bottom
- Label-to-control gap: 3px between knob and label, 2px between label and value
- Advanced panel: 8px top/bottom padding, 5px title-to-knobs gap

**Advanced Panel Layout:**
- All section titles horizontally centered within their column
- All knob groups centered as rows beneath their centered title
- Compact height (130px or less) — just enough for controls, no dead space
- Toggle bar between main and advanced is a thin 18px hairline with chevron indicator

**Layout Flexibility:**
Sections are defined by function, not by parameter count. Each section gets a fixed width (determined by the largest control it contains), and the center section flexes to fill remaining space. With fewer parameters, sections are narrower; with more, they expand proportionally. Priority controls should always be on the main panel — the advanced panel is for fine-tuning, not for hiding essential controls.

---

## Surface Treatment

### Textures

**Background:**
- Texture type: None — pure flat color
- Intensity: None
- Implementation: Solid color `#141414`

**Control Surfaces:**
- Control texture: Subtle radial gradient on knob bodies — no noise or grain
- Consistency: All knobs share the same gradient treatment

### Depth & Dimensionality

**Shadow Strategy:**
- Shadow presence: Subtle on knob SVGs, dramatic on plugin shell
- Shadow color: Pure black at varying opacities
- Shadow blur: Soft — `5px` on knobs, `40px` on shell
- Typical shadow values: Knobs: `drop-shadow(0 2px 5px rgba(0,0,0,0.7))`. Shell: `0 8px 40px rgba(0,0,0,0.7), 0 2px 8px rgba(0,0,0,0.5)`

**Elevation System:**
- Layers: Three levels — background (`#0a0a0a`), panel (`#141414`), section wash (gradient tint)
- How elevation is shown: Primarily through color lightness and colored glow filters, not shadows

**Borders:**
- Border presence: Selective — between sections (vertical dividers), around plugin shell, around scope displays
- Border style: 1px solid subtle lines
- Border color approach: Dark gray (`#2a2a2a`), slightly lighter than background

---

## Details & Embellishments

**Special Features:**
- Backlit arc glow on knobs — a Gaussian blur filter (`stdDeviation: 1.4-1.5`) applied to the active arc, creating a LED backlight effect
- Live SVG scope displays — small dark graphs showing transfer curves with glowing colored paths. Red glow for saturation curves, blue glow for dynamics curves.
- Section gradient wash — a barely perceptible colored gradient at the top of each functional section, tinting the background toward the section accent

**Active State Feedback:**
Controls show value tooltip near cursor during drag. Arc ring updates in real-time. Toggle buttons shift color and gain glow shadow.

**Hover States:**
Buttons brighten slightly on hover (`filter: brightness(1.15)` or background lightens). Knobs use `cursor: ns-resize` to indicate vertical drag behavior.

**Focus Indicators:**
Not prominently styled — this is a mouse/touch-first interface.

**Decorative Elements:**
- Section header labels in accent color with wide letter spacing — feel silk-screened
- Thin tagline below plugin name in very dim text
- Separator line below title block

---

## Technical Patterns

**CSS Patterns:**
- Border radius: None on panels (0), subtle on displays (3px), full pill on toggles (50px)
- Transition speed: Fast — 150ms for toggle states, 80ms for meter fills, 280ms for panel expand
- Easing: `ease` for most, `cubic-bezier(0.4, 0, 0.2, 1)` for panel expand/collapse

**Layout Techniques:**
- Preferred layout: Flexbox — horizontal sections with flex-direction: row, internal stacking with column
- Responsive strategy: Fixed pixel sizes — WebView plugins have known dimensions
- Alignment: Centered within sections

**Performance Considerations:**
SVG glow filters use Gaussian blur which can be expensive. Keep `stdDeviation` under 2.0. Meter animations use `requestAnimationFrame` at native refresh rate. Limit active animations to meters only — knob arcs only re-render on interaction.

---

## Interaction Feel

**Responsiveness:**
Immediate — knob arcs redraw on every mousemove during drag, no debouncing. Value tooltip follows cursor in real-time.

**Feedback:**
Visual only — arc fill, glow intensity, tooltip position, and scope curve updates all respond instantly to input.

**Tactility:**
Feels like modern hardware with digital precision. The backlit arcs and dark body evoke physical knobs on a Neve console, but the smooth vertical-drag interaction is purely software. The scope displays add a technical lab instrument quality.

**Overall UX Personality:**
Professional/technical. This is a tool for engineers who value precision and efficiency. No playfulness, no decoration for decoration's sake. Every visual element communicates function.

---

## Best Suited For

**Plugin Types:**
Compressors, saturators, EQs, channel strips, limiters, mastering processors, dynamics processors. Anything where precision and authority matter.

**Design Contexts:**
Professional plugin development. Commercial-grade releases. When you want the UI to communicate "this belongs in a mastering studio."

**Not Recommended For:**
Experimental/creative effects where playfulness matters. Synths or sound design tools that benefit from colorful, expressive interfaces. Very simple utilities where the dark aesthetic would feel heavy.

---

## Application Guidelines

### When Applying to New Plugin

**Parameter Count Adaptation:**
- **1-3 parameters:** Single centered section, large knobs, no section dividers. Plugin name above, controls below.
- **4-6 parameters:** Two sections (processing + output), subtle divider. Primary control larger, secondary controls in a row.
- **7-9 parameters:** Two or three functional sections with accent colors. Collapsible advanced panel for overflow parameters.
- **10+ parameters:** Three functional sections + collapsible advanced panel. Use the full dual-accent system. Scope displays for visual feedback.

**Control Type Mapping:**
- Float parameters → Rotary knob (270° arc, section-colored)
- Boolean parameters → Pill toggle with accent glow
- Choice parameters → Rotary knob with text labels (Soft/Medium/Hard style) or pill toggle for 2-state

**Prominent Parameter Handling:**
Mix, Input, Output, and other gain/blend controls should use the 360° circle-fill variant with neutral gray accent. This visually separates them from processing controls.

### Customization Points

**Easy to Adjust:**
- Section accent colors (swap red/blue for any two contrasting hues)
- Number of sections (1-3 on main panel)
- Presence/absence of scope displays
- Advanced panel contents and whether to include it at all

**Core Identity Elements:**
- Near-black background (#141414)
- Backlit arc knobs with glow filter
- Dual accent color system (functional color coding)
- Clean sans-serif uppercase labels with wide tracking
- The "dark precision" feeling

### Integration Notes

Uses SVG for all knob rendering — no raster images needed. Glow filters are defined inline within each SVG. The aesthetic is fully self-contained in HTML/CSS/JS with no external dependencies. WebView-compatible: no viewport units, `html,body { height: 100% }`, `user-select: none`, and contextmenu prevention all built in.

---

## Example Color Codes

```css
/* Backgrounds */
--bg-main: #141414;
--bg-surface: #1e1e1e;
--bg-scope: #0e0e0e;
--bg-outer: #0a0a0a;

/* Accents */
--accent-primary: #c0392b;
--accent-secondary: #4a90d9;
--accent-neutral: #888888;
--accent-toggle-alt: #8b1a1a;

/* Text */
--text-primary: #e8e8e8;
--text-secondary: #888888;
--text-value: #555555;
--text-faint: #424242;

/* Controls */
--control-body-light: #2c2c2c;
--control-body-dark: #141414;
--control-track: #232323;
--control-border: #0a0a0a;

/* Borders & Dividers */
--border-section: #2a2a2a;
--border-button: #3a3a3a;

/* Meters */
--meter-green: #22c55e;
--meter-amber: #f59e0b;
--meter-red: #ef4444;
```

---

## Implementation Checklist

When applying this aesthetic to a new plugin:

- [ ] Extract core color palette and define CSS variables
- [ ] Apply typography hierarchy (fonts, sizes, weights)
- [ ] Style each control type according to specifications
- [ ] Implement spacing system (gaps, padding, margins)
- [ ] Add surface treatments (textures, shadows, borders)
- [ ] Apply interaction states (hover, active, focus)
- [ ] Test with different parameter counts
- [ ] Verify visual consistency with source aesthetic
- [ ] Validate WebView constraints (no viewport units, etc.)
- [ ] Test in both Debug and Release builds
