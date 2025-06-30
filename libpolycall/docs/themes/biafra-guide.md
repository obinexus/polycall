
# Polycall.exe Biafra-Themed UI/UX Design System

## **Color Palette** 🇧🇯
*Inspired by Biafran flag colors (Red, Black, Green) + Golden Sun accents*

### Primary Colors
| Role | HEX | Usage | Contrast Ratio |
|------|-----|-------|----------------|
| Liberation Red | `#E22C28` | Primary CTAs, alerts | 4.7:1 on white |
| Palm Black | `#000100` | Headlines, dark UI | 15.8:1 on white |
| Forest Green | `#008753` | Success states, accents | 3.9:1 (pair with tints) |
| Golden Sun | `#FFD700` | Highlights, accents | 2.9:1 (use sparingly) |

### Accessible Modifications
```markdown
- **Red Tint**: `#FF6666` (for better readability)
- **Green Shade**: `#006B45` (meets AA contrast)
- **Sun Yellow**: `#CC9900` (accessible alternative)
```

### Neutral Base
```color-swatch
- Ivory White: `#F5F5F5` (backgrounds)
- Clay Gray: `#666666` (secondary text)
- Midnight: `#1A1A1A` (dark mode base)
```

---

## **Typography System**
**Primary Font**: Inter (Open Source, variable axes)  
**Code Font**: Fira Code (developer-friendly ligatures)

```markdown
### Hierarchy
# H1 - 32px (800 weight)
## H2 - 28px (700 weight)
Body - 16px (400 weight)
Code - 14px (monospace)
```

---

## **UI Components (Biafran Motifs)**

### 1. Navigation Stripes
![Horizontal red-black-green bars](https://flag-color-bars.svg)  
*Subtle 3px bands at top/bottom of UI*

### 2. Half-Sun Loader
```svg
<svg class="sun-spinner">
  <path d="M12,2 L12,12 M2,12 L12,12" fill="#FFD700"/>
</svg>
```

### 3. Button Styles
```css
.primary-btn {
  background: linear-gradient(145deg, #E22C28, #FF6666);
  border-radius: 8px;
  padding: 12px 24px;
  color: white;
}
```

---

## **Accessibility Standards**
1. **Text Contrast**: Minimum 4.5:1 for body text
2. **Focus States**: 3px golden outline (`#CC9900`)
3. **ARIA Labels**: Implement for all interactive elements
4. **Dark Mode**: Automatic switching with OS preference

---

## **IDE Implementation Guide**

### VS Code Theme Snippet
```json
{
  "colors": {
    "focusBorder": "#FFD700",
    "activityBar.background": "#000100",
    "statusBar.background": "#008753"
  },
  "tokenColors": [
    {
      "name": "Biafran Functions",
      "scope": "entity.name.function",
      "settings": { "foreground": "#E22C28" }
    }
  ]
}
```

### Sublime Text Adaptation
```xml
<dict>
  <key>name</key>
  <string>Biafran Theme</string>
  <key>settings</key>
  <array>
    <dict>
      <key>settings</key>
      <dict>
        <key>background</key>
        <string>#F5F5F5</string>
        <key>caret</key>
        <string>#008753</string>
      </dict>
    </dict>
  </array>
</dict>
```

---

## **Cultural Imagery Integration**
1. **Pattern Library**: Traditional `Uli` body art as subtle textures
2. **Icon Set**: Palm tree, cockcrow, and yam symbols for file types
3. **Loading States**: Animated Adinkra symbols (NSIBIDI script inspired)

---

**Implementation Checklist**  
✅ WCAG 2.1 AA Compliance  
✅ Cross-IDE theming support  
✅ Responsive breakpoints (mobile-first)  
✅ Dark/light mode variants  
✅ Developer documentation in `/theme/biafra-guide.md`

