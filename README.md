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