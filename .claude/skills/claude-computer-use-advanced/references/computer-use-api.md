# Computer Use API Reference

Complete API documentation for Claude's computer use tool, including tool versions, action types, parameters, and model compatibility.

## Overview

The computer use tool enables Claude to interact with desktop environments by viewing screens, clicking elements, typing text, and controlling applications. This reference covers the complete API for all supported versions.

**Current Versions**:
- `computer_20251124`: Opus 4.5 with advanced zoom tool
- `computer_20250124`: Claude 4 / Sonnet 3.7 with full action support

**Beta Headers**:
- `computer-use-2025-11-24`: For computer_20251124 (Opus 4.5)
- `computer-use-2025-01-24`: For computer_20250124 (Claude 4/Sonnet 3.7)

---

## Tool Versions

### Opus 4.5 Tool (computer_20251124)

Latest version with zoom tool for enhanced vision accuracy.

**Configuration**:
```python
{
    "type": "computer_20251124",
    "name": "computer",
    "display_width_px": 1024,
    "display_height_px": 768,
    "display_number": ":1",
    "enable_zoom": True  # Enable zoom tool
}
```

**Model**: Claude Opus 4.5 (`claude-opus-4-5-20250929`)
**Beta Header**: `anthropic-beta: computer-use-2025-11-24`

**Supported Actions**:
- `screenshot` - Capture display
- `left_click` - Click at coordinates
- `right_click` - Right-click context menu
- `double_click` - Double-click element
- `middle_click` - Middle mouse button
- `click_holding` - Click and hold (for drag operations)
- `type` - Type text input
- `key` - Press keyboard keys/shortcuts
- `mouse_move` - Move cursor to position
- `scroll` - Scroll display
- `drag` - Drag from one point to another
- `zoom` - Zoom into screen region (Opus 4.5 exclusive)

**Improvements**:
- Enhanced vision capabilities with zoom tool
- Better text recognition in zoomed regions
- Improved element boundary detection
- More accurate coordinate identification

---

### Claude 4 / Sonnet 3.7 Tool (computer_20250124)

Stable version with comprehensive action support.

**Configuration**:
```python
{
    "type": "computer_20250124",
    "name": "computer",
    "display_width_px": 1024,
    "display_height_px": 768,
    "display_number": ":1"
}
```

**Models**:
- Claude 4 (`claude-4-20250522`)
- Claude Sonnet 3.7 (`claude-3-5-sonnet-20241022`)

**Beta Header**: `anthropic-beta: computer-use-2025-01-24`

**Supported Actions**:
- `screenshot` - Capture display
- `left_click` - Click at coordinates
- `right_click` - Right-click context menu
- `double_click` - Double-click element
- `middle_click` - Middle mouse button
- `click_holding` - Click and hold
- `type` - Type text input
- `key` - Press keyboard keys/shortcuts
- `mouse_move` - Move cursor
- `scroll` - Scroll display
- `drag` - Drag operation

**Note**: Zoom tool not available in this version

---

## Model Compatibility Matrix

| Model | Tool Version | Beta Header | Zoom Tool | Status |
|-------|--------------|-------------|-----------|--------|
| Claude Opus 4.5 | `computer_20251124` | `computer-use-2025-11-24` | ✅ Yes | Latest |
| Claude 4 | `computer_20250124` | `computer-use-2025-01-24` | ❌ No | Stable |
| Claude Sonnet 3.7 | `computer_20250124` | `computer-use-2025-01-24` | ❌ No | Stable |

---

## Action Types Reference

### Screenshot Action

Captures the current display and returns it as a base64-encoded image.

**Parameters**:
```python
{
    "type": "screenshot"
}
```

**Returns**:
- Base64-encoded PNG image of current display
- Respects configured display resolution
- Can be analyzed by Claude's vision capabilities

**Example**:
```python
action = {"type": "screenshot"}
```

**Use Cases**:
- Understand current application state
- Identify elements before clicking
- Verify action results
- Analyze visual content
- Check workflow progress

---

### Click Actions

#### Left Click

Clicks at specified coordinates.

**Parameters**:
```python
{
    "type": "left_click",
    "coordinate": [x, y]  # [640, 400]
}
```

**Coordinate System**:
- Origin (0, 0) at top-left
- X increases rightward
- Y increases downward
- Pixel precision

**Example**:
```python
# Click center of button at (640, 400)
{"type": "left_click", "coordinate": [640, 400]}
```

---

#### Right Click

Opens context menu at coordinates.

**Parameters**:
```python
{
    "type": "right_click",
    "coordinate": [x, y]
}
```

**Example**:
```python
# Right-click to open context menu
{"type": "right_click", "coordinate": [640, 400]}
```

---

#### Double Click

Double-clicks at coordinates (selects text, activates double-click actions).

**Parameters**:
```python
{
    "type": "double_click",
    "coordinate": [x, y]
}
```

**Example**:
```python
# Double-click to select text or activate action
{"type": "double_click", "coordinate": [400, 300]}
```

---

#### Middle Click

Clicks with middle mouse button.

**Parameters**:
```python
{
    "type": "middle_click",
    "coordinate": [x, y]
}
```

**Example**:
```python
# Middle-click at position
{"type": "middle_click", "coordinate": [640, 400]}
```

---

#### Click Holding

Clicks and holds at coordinates (for drag preparation).

**Parameters**:
```python
{
    "type": "click_holding",
    "coordinate": [x, y]
}
```

**Example**:
```python
# Click and hold (typically followed by drag)
{"type": "click_holding", "coordinate": [640, 400]}
```

---

### Text Input Action

Types text at current cursor position.

**Parameters**:
```python
{
    "type": "type",
    "text": "text to type"
}
```

**Notes**:
- Types at cursor position
- Typically follows a click on text field
- Supports special characters
- Character-by-character input

**Example**:
```python
# Type email address in form field
{"type": "type", "text": "user@example.com"}
```

**Special Characters**:
```python
# Use key action for special keys like Tab, Enter
{"type": "key", "key": "Return"}  # Enter key
{"type": "key", "key": "Tab"}     # Tab key
```

---

### Keyboard Action

Presses keyboard keys and key combinations.

**Parameters**:
```python
{
    "type": "key",
    "key": "key_name"  # Single key or combination
}
```

**Common Keys**:
```python
# Navigation
"Return"      # Enter key
"Tab"         # Tab key
"BackSpace"   # Backspace/Delete
"Delete"      # Delete forward
"Escape"      # Escape key

# Arrow keys
"Up"
"Down"
"Left"
"Right"

# Special keys
"Home"
"End"
"PageUp"
"PageDown"
"Insert"

# Modifiers (can combine)
"shift"
"ctrl"
"alt"
"cmd"         # Mac Command key
```

**Key Combinations**:
```python
# Lowercase with modifiers
{"type": "key", "key": "ctrl+a"}      # Select all
{"type": "key", "key": "ctrl+c"}      # Copy
{"type": "key", "key": "ctrl+v"}      # Paste
{"type": "key", "key": "ctrl+z"}      # Undo
{"type": "key", "key": "ctrl+s"}      # Save
{"type": "key", "key": "shift+Tab"}   # Shift-Tab (backwards)
{"type": "key", "key": "alt+Tab"}     # Switch windows
{"type": "key", "key": "cmd+q"}       # Quit (Mac)
```

**Example**:
```python
# Select all and delete
{"type": "key", "key": "ctrl+a"}
{"type": "key", "key": "Delete"}
```

---

### Mouse Move Action

Moves cursor to specified coordinates.

**Parameters**:
```python
{
    "type": "mouse_move",
    "coordinate": [x, y]
}
```

**Example**:
```python
# Move cursor to coordinate
{"type": "mouse_move", "coordinate": [640, 400]}
```

**Use Cases**:
- Hover over elements to show tooltips
- Position cursor before drag operations
- Mouse-over interactions in rich UIs

---

### Scroll Action

Scrolls the display.

**Parameters**:
```python
{
    "type": "scroll",
    "coordinate": [x, y],      # Position to scroll in
    "direction": "up" or "down",
    "amount": 3                 # Number of scroll units
}
```

**Example**:
```python
# Scroll down 5 units in center of screen
{"type": "scroll", "coordinate": [640, 400], "direction": "down", "amount": 5}

# Scroll up 3 units
{"type": "scroll", "coordinate": [640, 400], "direction": "up", "amount": 3}
```

---

### Drag Action

Drags from source to destination coordinates.

**Parameters**:
```python
{
    "type": "drag",
    "startCoordinate": [x1, y1],
    "endCoordinate": [x2, y2]
}
```

**Example**:
```python
# Drag element from one location to another
{"type": "drag", "startCoordinate": [400, 200], "endCoordinate": [600, 300]}
```

**Use Cases**:
- Reordering list items
- Dragging files in file explorer
- Slider adjustments
- Drawing or annotation tools

---

### Zoom Action (Opus 4.5 Only)

Zooms into a rectangular region of the screen for detailed inspection.

**Parameters**:
```python
{
    "type": "zoom",
    "coordinate": [x1, y1, x2, y2]  # [left, top, right, bottom]
}
```

**Coordinate System**:
- `x1, y1`: Top-left corner of region
- `x2, y2`: Bottom-right corner of region
- Full resolution for zoomed region

**Example**:
```python
# Zoom into region from (300, 200) to (700, 400)
{"type": "zoom", "coordinate": [300, 200, 700, 400]}
```

**Region Sizing**:
```python
# Small region for single element (50x50)
{"type": "zoom", "coordinate": [595, 375, 645, 425]}

# Medium region for control group (200x200)
{"type": "zoom", "coordinate": [440, 300, 640, 500]}

# Large region for analysis (400x300)
{"type": "zoom", "coordinate": [300, 200, 700, 500]}
```

**Returns**:
- High-resolution image of specified region
- Shows fine details and small text
- Better for coordinate precision

**Best Practices**:
- Zoom when uncertain about element location
- Use regions 20-30 pixels larger than target
- Combine with screenshots for efficiency
- Zoom before clicking small elements

---

## Tool Configuration Parameters

### Display Configuration

```python
{
    "type": "computer_20251124",           # or computer_20250124
    "name": "computer",
    "display_width_px": 1024,              # Recommended: 1024
    "display_height_px": 768,              # Recommended: 768
    "display_number": ":1",                # X11 display
    "enable_zoom": True                    # Optional, for Opus 4.5
}
```

**Parameters**:
- `type`: Tool version identifier
- `name`: Must be "computer"
- `display_width_px`: Screen width in pixels (max ~1280)
- `display_height_px`: Screen height in pixels (max ~800)
- `display_number`: X11 display number (e.g., ":1", ":0")
- `enable_zoom`: Enable zoom tool (Opus 4.5 only)

**Recommended Resolutions**:
- **Standard**: 1024x768 (good balance)
- **High Detail**: 1280x800 (maximum recommended)
- **Small Screens**: 800x600 (minimum for usability)

---

## Request Structure

### Basic Request

```python
import anthropic

client = anthropic.Anthropic(api_key="your-api-key")

response = client.messages.create(
    model="claude-opus-4-5-20250929",      # Model depends on tool version
    max_tokens=1024,
    tools=[
        {
            "type": "computer_20251124",
            "name": "computer",
            "display_width_px": 1024,
            "display_height_px": 768,
            "display_number": ":1",
            "enable_zoom": True
        }
    ],
    messages=[
        {
            "role": "user",
            "content": "Take a screenshot and describe what you see."
        }
    ],
    headers={"anthropic-beta": "computer-use-2025-11-24"}
)
```

**Key Points**:
- Include `headers` with appropriate beta header
- `tools` parameter defines computer use configuration
- `messages` contain user instructions
- Response includes tool use actions

### Response Handling

```python
for content_block in response.content:
    if content_block.type == "tool_use":
        action = content_block.input
        # action["type"] = "screenshot", "left_click", etc.
        # Execute action and get result
        # Add result back to messages and continue
```

---

## Agent Loop Pattern

Complete example of orchestrating multiple actions in sequence.

```python
import anthropic
import base64

client = anthropic.Anthropic(api_key="your-api-key")

# Define tool
tools = [
    {
        "type": "computer_20251124",
        "name": "computer",
        "display_width_px": 1024,
        "display_height_px": 768,
        "display_number": ":1",
        "enable_zoom": True
    }
]

# Initialize conversation
messages = [
    {
        "role": "user",
        "content": "Fill out the form with name 'John' and email 'john@example.com', then submit."
    }
]

# Agent loop
while True:
    # Get next action from Claude
    response = client.messages.create(
        model="claude-opus-4-5-20250929",
        max_tokens=1024,
        tools=tools,
        messages=messages,
        headers={"anthropic-beta": "computer-use-2025-11-24"}
    )

    # Process response
    if response.stop_reason == "tool_use":
        # Extract tool use action
        tool_use = next(
            (block for block in response.content if block.type == "tool_use"),
            None
        )

        if tool_use:
            action = tool_use.input

            # Execute action
            result = execute_action(action)

            # Add assistant response and result to messages
            messages.append({"role": "assistant", "content": response.content})
            messages.append({
                "role": "user",
                "content": [
                    {
                        "type": "tool_result",
                        "tool_use_id": tool_use.id,
                        "content": result
                    }
                ]
            })
    else:
        # Claude finished (stop_reason == "end_turn")
        break

# Get final response
final_text = next(
    (block.text for block in response.content if hasattr(block, "text")),
    None
)
print(final_text)
```

---

## Error Handling

### Common Errors and Solutions

**Missed Click**:
```python
# If click doesn't register:
# 1. Take screenshot
{"type": "screenshot"}

# 2. Identify actual coordinates
# 3. Retry click with correct coordinates
{"type": "left_click", "coordinate": [correct_x, correct_y]}
```

**Unclear Element Location**:
```python
# If unsure where element is:
# 1. Take screenshot
{"type": "screenshot"}

# 2. Use zoom to inspect region (Opus 4.5)
{"type": "zoom", "coordinate": [region_x1, region_y1, region_x2, region_y2]}

# 3. Click with precise coordinates
{"type": "left_click", "coordinate": [exact_x, exact_y]}
```

**Text Not Typed**:
```python
# If text doesn't appear:
# 1. Click text field to focus
{"type": "left_click", "coordinate": [field_x, field_y]}

# 2. Clear any existing text
{"type": "key", "key": "ctrl+a"}
{"type": "key", "key": "Delete"}

# 3. Type again
{"type": "type", "text": "new text"}
```

**Form Not Submitted**:
```python
# After filling form:
# 1. Take screenshot to find submit button
{"type": "screenshot"}

# 2. Click submit button
{"type": "left_click", "coordinate": [submit_x, submit_y]}

# 3. Wait for response
{"type": "screenshot"}
```

---

## Performance Considerations

### Token Usage

**Screenshot Cost**:
- Base cost: 100-200 tokens (vision processing)
- Varies by resolution and image complexity
- Include in max_tokens calculation

**Action Cost**:
- Non-visual actions (click, type, key): ~10-50 tokens
- With screenshot: 100-250+ tokens total

**Optimization**:
- Minimize screenshot frequency
- Use zoom for detail instead of full screenshots
- Combine multiple actions when possible
- Clear cached vision where applicable

### Display Resolution

**Impact on Accuracy**:
- **1024x768**: Good balance, reasonable token cost
- **1280x800**: Best detail, higher token cost
- **800x600**: Lower cost but harder to see details

**Recommendation**:
Use 1024x768 as default, zoom for detail when needed.

---

## Complete Example: Web Form Filling

```python
import anthropic
import base64
from pathlib import Path

def fill_web_form():
    """Complete example: fill web form and submit."""
    client = anthropic.Anthropic()

    tools = [
        {
            "type": "computer_20251124",
            "name": "computer",
            "display_width_px": 1024,
            "display_height_px": 768,
            "display_number": ":1",
            "enable_zoom": True
        }
    ]

    messages = [
        {
            "role": "user",
            "content": "Fill out the contact form with these details:\n- Name: Alice Johnson\n- Email: alice@example.com\n- Message: I'm interested in your services\nThen click the Submit button."
        }
    ]

    while True:
        response = client.messages.create(
            model="claude-opus-4-5-20250929",
            max_tokens=2048,
            tools=tools,
            messages=messages,
            headers={"anthropic-beta": "computer-use-2025-11-24"}
        )

        if response.stop_reason == "end_turn":
            # Task complete
            final_response = next(
                (block.text for block in response.content if hasattr(block, "text")),
                None
            )
            print(f"Complete: {final_response}")
            break

        # Handle tool use
        for block in response.content:
            if block.type == "tool_use":
                action = block.input

                # Execute action (simplified - your implementation needed)
                if action["type"] == "screenshot":
                    result = "Screenshot captured"
                elif action["type"] == "left_click":
                    result = "Clicked successfully"
                elif action["type"] == "type":
                    result = f"Typed: {action['text']}"
                else:
                    result = f"Action executed: {action['type']}"

                # Add to message history
                messages.append({"role": "assistant", "content": response.content})
                messages.append({
                    "role": "user",
                    "content": [
                        {
                            "type": "tool_result",
                            "tool_use_id": block.id,
                            "content": result
                        }
                    ]
                })
                break

if __name__ == "__main__":
    fill_web_form()
```

---

## References

- [Official Computer Use Documentation](https://platform.claude.com/docs/en/build-with-claude/computer-use)
- [Computer Use Demo GitHub Repository](https://github.com/anthropics/anthropic-quickstarts/tree/main/computer-use-demo)
- [Claude Opus 4.5 Announcement](https://www.anthropic.com/news/claude-opus-4-5)
