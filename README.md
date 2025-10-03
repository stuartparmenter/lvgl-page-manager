# LVGL Page Manager

An ESPHome component that provides page management functionality for LVGL displays. Switch between different LVGL pages through a Home Assistant select entity and optional navigation buttons.

## Features

- **Modular page registration** across multiple YAML files
- **LVGL component integration**
- **Home Assistant integration** with select entity
- **Optional navigation buttons** (next/previous)
- **Flexible sorting** (by order, name, or page ID)
- **Push/pop temporary pages** with automatic return and cleanup triggers

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
| `push_mode` | enum | Push behavior: `stack` (default), `replace` |
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

## Push/Pop Temporary Pages

The component supports temporarily displaying pages with automatic return to the previous page. This is perfect for alerts, notifications, or time-sensitive information displays.

### Basic Usage

```yaml
# Show a page temporarily for 20 seconds, then automatically return
on_...:
  - lvgl_page_manager.page.push:
      page: alert_page
      duration: 20s
```

### With Setup and Cleanup Triggers

Use `on_push` and `on_pop` to perform actions when the page appears and disappears:

```yaml
# Doorbell camera example
on_...:
  - lvgl_page_manager.page.push:
      page: doorbell_camera
      duration: 30s
      animation: fade_in
      on_push:
        - switch.turn_on: camera_stream
        - light.turn_on: backlight
      on_pop:
        - switch.turn_off: camera_stream
        - light.turn_off: backlight
```

**Key behavior:** `on_pop` **always fires** when the page is removed, whether by timeout, manual pop, or navigation. This ensures reliable cleanup like a finally clause.

### Manual Pop

Return to the base page early:

```yaml
# Dismiss button on pushed page
on_click:
  - lvgl_page_manager.page.pop:
      animation: fade_out
```

### Clear Stack

Clear all pushed pages and return to base:

```yaml
on_...:
  - lvgl_page_manager.page.clear:
```

### Push Modes

Control how multiple pushes behave:

```yaml
lvgl_page_manager:
  push_mode: stack  # or "replace"
  select:
    name: "Display Page"
```

**Stack mode (default):**
- Multiple pushes create a stack
- New push pauses current pushed page
- Pops resume previous pushed pages with remaining time
- Example: Weather alert → Score alert → Doorbell (pops in reverse order)

**Replace mode:**
- New push cancels old push (fires `on_pop`)
- Only one push active at a time
- Simpler behavior for most use cases

### Manual Navigation During Push

Any manual navigation (select change, next, prev, show) during an active push:
1. Fires `on_pop` for all pushed pages (no visual flashing)
2. Clears the push stack
3. Navigates to the requested page normally

This ensures cleanup triggers always run and manual navigation takes precedence.

### Complete Example: Sports Score Alert

```yaml
# Home Assistant automation triggers this
on_...:
  - lvgl_page_manager.page.push:
      page: falcons_tracker
      duration: 20s
      animation: over_left
      on_push:
        - logger.log: "Showing score alert"
        - media_player.volume_set:
            volume_percent: 30
      on_pop:
        - logger.log: "Score alert dismissed"
        - media_player.volume_set:
            volume_percent: 80
```

**Timeline:**
1. User is on "Now Playing" page (base page saved)
2. Touchdown scored → push action executes
3. `on_push` fires, volume lowered
4. "Falcons Tracker" page shown for 20 seconds
5. Timeout expires
6. `on_pop` fires, volume restored
7. Returns to "Now Playing" page

### Action Parameters

#### Push Page

| Parameter | Type | Description |
|-----------|------|-------------|
| `page` | string | Page ID to push (templatable) |
| `duration` | time | How long to show page before auto-pop (templatable) |
| `animation` | enum | Transition animation (default: `none`) |
| `time` | time | Animation duration (default: `50ms`) |
| `on_push` | automation | Triggers when page appears (optional) |
| `on_pop` | automation | Triggers when page is removed (optional, always fires) |

#### Pop Page

| Parameter | Type | Description |
|-----------|------|-------------|
| `animation` | enum | Transition animation (default: `none`) |
| `time` | time | Animation duration (default: `50ms`) |

#### Clear Stack

| Parameter | Type | Description |
|-----------|------|-------------|
| `animation` | enum | Transition animation (default: `none`) |
| `time` | time | Animation duration (default: `50ms`) |

### Home Assistant Integration

To expose push/pop actions to Home Assistant, define them in your ESPHome YAML:

```yaml
api:
  actions:
    - action: display_push_page
      variables:
        page: string
        duration: int  # Milliseconds from Home Assistant
      then:
        - lvgl_page_manager.page.push:
            page: !lambda 'return page;'
            duration: !lambda 'return duration;'  # Pass ms directly
            on_push:
              - logger.log: "Page pushed from HA"
            on_pop:
              - logger.log: "Page popped"
```

Then call from Home Assistant:

```yaml
# Home Assistant automation
automation:
  - alias: "Show weather alert"
    trigger:
      - platform: state
        entity_id: sensor.severe_weather_alert
        to: "on"
    action:
      - action: esphome.display_push_page
        data:
          page: "weather_alert"
          duration: 15000  # Pass milliseconds (15 seconds)
```

> **Note**: You cannot pass `on_push`/`on_pop` triggers from Home Assistant - these must be defined in the ESPHome YAML configuration.

### Use Cases

- **Alerts and Notifications**: Show urgent information temporarily
- **Doorbell Camera**: Display camera feed when button pressed
- **Score Updates**: Interrupt current view for sports updates
- **Timer/Alarm Notifications**: Show timer completion briefly
- **Sensor Alerts**: Display warning when sensor threshold exceeded