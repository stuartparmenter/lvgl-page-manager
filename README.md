# LVGL Page Manager

An ESPHome component that provides page management functionality for LVGL displays. Switch between different LVGL pages through a Home Assistant select entity and optional navigation buttons.

## Features

- **Modular page registration** across multiple YAML files
- **LVGL component integration**
- **Home Assistant integration** with select entity
- **Optional navigation buttons** (next/previous)
- **Flexible sorting** (by order, name, or page ID)

## Basic Usage

### 1. Main Configuration

```yaml
# main.yaml
lvgl_page_manager:
  lvgl: my_lvgl  # Reference to your LVGL component (optional - auto-detects if only one)
  select:
    name: "Display Page"
    icon: "mdi:view-dashboard"
  default_page: page_home  # Can also be defined in a package file
  sort: by_order
  next_button:
    name: "Page Next"
  prev_button:
    name: "Page Previous"
```

### 2. Page Registration

Register pages in separate package files:

```yaml
# packages/page_dashboard.yaml
lvgl_page_manager:
  pages:
    - page: page_dashboard
      friendly_name: "Dashboard"
      order: 10

# packages/page_settings.yaml
lvgl_page_manager:
  pages:
    - page: page_settings
      friendly_name: "Settings"
      order: 20
```

### 3. Include Packages

```yaml
# main.yaml
packages:
  page_dashboard: !include packages/page_dashboard.yaml
  page_settings: !include packages/page_settings.yaml
```

## Configuration Options

### Page Manager

| Parameter | Type | Description |
|-----------|------|-------------|
| `id` | string | Component ID (optional - auto-generated if not specified) |
| `lvgl` | string | LVGL component ID (optional - auto-detects if only one) |
| `select` | object | Home Assistant select entity configuration |
| `default_page` | string | Initial page to display |
| `sort` | enum | Sort order: `by_order`, `by_name`, `by_page` |
| `next_button` | object | Optional next navigation button |
| `prev_button` | object | Optional previous navigation button |

### Page Definition

| Parameter | Type | Description |
|-----------|------|-------------|
| `page` | string | LVGL page ID |
| `friendly_name` | string | Display name for the page |
| `order` | integer | Sort order (when using `by_order`) |

## How It Works

1. **Configuration Merging**: ESPHome automatically merges all `lvgl_page_manager` configurations
2. **Page Management**: The component manages your defined LVGL pages and creates a selectable list
3. **Home Assistant Integration**: Exposes a select entity to choose pages
4. **Navigation**: Optional buttons provide next/previous functionality

> **Note**: Since configurations are merged, you can define `default_page` in any package file rather than the main configuration.

## Example Complete Setup

```yaml
# main.yaml
esphome:
  name: my-display

packages:
  dashboard: !include packages/dashboard.yaml
  settings: !include packages/settings.yaml

lvgl:
  # your LVGL configuration

lvgl_page_manager:
  lvgl: my_display  # Optional - will auto-detect if not specified
  select:
    name: "Display Page"
  default_page: dashboard_page
  sort: by_order
  next_button:
    name: "Next Page"
  prev_button:
    name: "Previous Page"
```

```yaml
# packages/dashboard.yaml
lvgl_page_manager:
  pages:
    - page: dashboard_page
      friendly_name: "Dashboard"
      order: 1
```

```yaml
# packages/settings.yaml
lvgl_page_manager:
  pages:
    - page: settings_page
      friendly_name: "Settings"
      order: 2
```

This creates a page manager with two pages that can be controlled from Home Assistant or the navigation buttons.

## Actions

The component provides actions that mimic ESPHome's built-in LVGL page actions but work specifically with your registered pages. These actions ensure your page manager stays synchronized with page changes.

### Available Actions

#### Next Page
Navigate to the next page in the list:

```yaml
# Simple syntax
on_...:
  - lvgl_page_manager.page.next:

# With animation and timing
on_...:
  - lvgl_page_manager.page.next:
      animation: over_left
      time: 300ms
```

#### Previous Page
Navigate to the previous page in the list:

```yaml
# Simple syntax
on_...:
  - lvgl_page_manager.page.previous:

# With animation and timing
on_...:
  - lvgl_page_manager.page.previous:
      animation: out_right
      time: 250ms
```

#### Show Specific Page
Navigate to a specific page by ID:

```yaml
# Simple syntax
on_...:
  - lvgl_page_manager.page.show: dashboard_page

# With animation and timing
on_...:
  - lvgl_page_manager.page.show:
      page: settings_page
      animation: fade_in
      time: 400ms
```

### Animation Options

All actions support the same animation types as ESPHome's built-in LVGL actions:

- `none` - No animation (immediate switch)
- `over_left`, `over_right`, `over_top`, `over_bottom` - New page slides over current
- `move_left`, `move_right`, `move_top`, `move_bottom` - Both pages move together
- `fade_in`, `fade_out` - Fade transitions
- `out_left`, `out_right`, `out_top`, `out_bottom` - Current page slides out

**Default values:**
- `animation`: `over_left` for next, `over_right` for previous, `none` for show
- `time`: `50ms`

### Why Use These Actions?

**Instead of using:**
```yaml
# This bypasses your page manager
on_...:
  - lvgl.page.next: my_lvgl
```

**Use:**
```yaml
# This keeps your page manager synchronized
on_...:
  - lvgl_page_manager.page.next:  # ID optional when only one page manager
```

The custom actions ensure that:
- Your page manager's select entity updates correctly
- Page navigation respects your registered pages
- You maintain control over page switching logic