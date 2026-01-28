# Advanced Computer Use Patterns

Sophisticated patterns for multi-step automation, complex workflows, zoom tool usage, and production-grade implementations.

## Overview

This guide covers advanced patterns beyond basic screenshot-click-type operations. Learn how to build reliable, maintainable automation workflows that handle complexity, errors, and edge cases.

---

## Multi-Step Automation Patterns

### Pattern 1: Stateful Agent Loop

The foundational pattern for all multi-step automation - maintain state between actions and use vision feedback to guide decisions.

**Architecture**:
```
State → Vision (screenshot) → Analyze → Decide Action → Execute → Update State → Repeat
```

**Implementation**:
```python
class AutomationAgent:
    def __init__(self, client, tools_config):
        self.client = client
        self.tools = [tools_config]
        self.state = {
            "step": 0,
            "data": {},
            "screenshots": [],
            "errors": []
        }
        self.messages = []

    def initialize(self, objective):
        """Start agent with objective."""
        self.state["objective"] = objective
        self.messages = [{"role": "user", "content": objective}]

    def step(self):
        """Execute one step of the automation."""
        response = self.client.messages.create(
            model="claude-opus-4-5-20250929",
            max_tokens=2048,
            tools=self.tools,
            messages=self.messages,
            headers={"anthropic-beta": "computer-use-2025-11-24"}
        )

        # Process response
        if response.stop_reason == "tool_use":
            action = self._extract_action(response)
            result = self._execute_action(action)
            self._record_state(action, result)
            self.messages.append({"role": "assistant", "content": response.content})
            self.messages.append({
                "role": "user",
                "content": [{"type": "tool_result", "tool_use_id": action.id, "content": result}]
            })
            return False  # Continue
        else:
            return True  # Complete

    def run_to_completion(self):
        """Run until completion."""
        self.state["step"] = 0
        while not self.step():
            self.state["step"] += 1
            if self.state["step"] > 100:  # Safety limit
                self.state["errors"].append("Max steps exceeded")
                break
        return self.state

    def _extract_action(self, response):
        return next(b for b in response.content if b.type == "tool_use")

    def _execute_action(self, action):
        # Execute and return result
        if action.input["type"] == "screenshot":
            return "screenshot_base64_data"
        elif action.input["type"] == "left_click":
            return "clicked"
        # ... other actions
        return "executed"

    def _record_state(self, action, result):
        self.state["data"][f"step_{self.state['step']}"] = {
            "action": action.input,
            "result": result
        }
```

**Key Benefits**:
- Maintains context across multiple steps
- Records state for debugging
- Handles continuation naturally
- Integrates error recovery

---

### Pattern 2: Conditional Workflows

Handle branching logic based on visual analysis of screenshots.

**Implementation**:
```python
class ConditionalWorkflow:
    def __init__(self, client, tools):
        self.client = client
        self.tools = tools
        self.paths = {}

    def define_path(self, name, condition_description, actions):
        """Define a conditional workflow path."""
        self.paths[name] = {
            "condition": condition_description,
            "actions": actions
        }

    def execute(self, initial_screenshot, objective):
        """Execute workflow with conditions."""
        messages = [
            {"role": "user", "content": f"Current objective: {objective}"}
        ]

        # Loop until complete
        while True:
            # Take screenshot to assess state
            messages.append({
                "role": "user",
                "content": [
                    {"type": "text", "text": "Analyze the current state and determine next action."}
                ]
            })

            response = self.client.messages.create(
                model="claude-opus-4-5-20250929",
                max_tokens=1024,
                tools=self.tools,
                messages=messages,
                headers={"anthropic-beta": "computer-use-2025-11-24"}
            )

            # Check if done
            if response.stop_reason == "end_turn":
                break

            # Process actions
            tool_use = self._extract_tool_use(response)
            if tool_use:
                result = self._execute(tool_use.input)
                messages.append({"role": "assistant", "content": response.content})
                messages.append({
                    "role": "user",
                    "content": [
                        {"type": "tool_result", "tool_use_id": tool_use.id, "content": result}
                    ]
                })

    def _extract_tool_use(self, response):
        return next((b for b in response.content if b.type == "tool_use"), None)

    def _execute(self, action):
        # Execute action and return result
        return "executed"
```

**Example Workflow**:
```python
workflow = ConditionalWorkflow(client, tools)

# Path 1: If login page shown
workflow.define_path(
    "login_required",
    "Username and password fields visible",
    [
        {"type": "left_click", "coordinate": [400, 300]},
        {"type": "type", "text": "username"},
        {"type": "key", "key": "Tab"},
        {"type": "type", "text": "password"},
        {"type": "key", "key": "Return"}
    ]
)

# Path 2: If already logged in
workflow.define_path(
    "already_logged_in",
    "Main dashboard visible, username shown in top right",
    [
        {"type": "left_click", "coordinate": [600, 400]}
    ]
)

workflow.execute(initial_screenshot, "Navigate to user profile")
```

---

### Pattern 3: Error Recovery with Retries

Implement resilient workflows that recover from transient failures.

**Implementation**:
```python
class ResilientWorkflow:
    def __init__(self, client, tools, max_retries=3):
        self.client = client
        self.tools = tools
        self.max_retries = max_retries

    def execute_with_retry(self, action, context=""):
        """Execute action with automatic retry on failure."""
        for attempt in range(self.max_retries):
            try:
                result = self._attempt_action(action)
                if self._is_success(result):
                    return result
            except Exception as e:
                if attempt < self.max_retries - 1:
                    # Retry with adjusted parameters
                    action = self._adjust_action(action, attempt)
                else:
                    raise

        raise RuntimeError(f"Failed after {self.max_retries} attempts")

    def _attempt_action(self, action):
        """Attempt to execute an action."""
        messages = [
            {"role": "user", "content": f"Execute this action: {action}"}
        ]

        response = self.client.messages.create(
            model="claude-opus-4-5-20250929",
            max_tokens=1024,
            tools=self.tools,
            messages=messages,
            headers={"anthropic-beta": "computer-use-2025-11-24"}
        )

        # Extract and execute
        tool_use = next((b for b in response.content if b.type == "tool_use"), None)
        if tool_use:
            return self._execute(tool_use.input)
        return None

    def _is_success(self, result):
        """Check if action succeeded."""
        return result and "success" in str(result).lower()

    def _adjust_action(self, action, attempt):
        """Adjust action based on retry attempt."""
        # For clicks, adjust coordinates slightly
        if action["type"] == "left_click":
            action["coordinate"] = [
                action["coordinate"][0] + (attempt * 5),
                action["coordinate"][1] + (attempt * 5)
            ]
        return action

    def _execute(self, action):
        """Execute action and return result."""
        return "executed"
```

**Usage**:
```python
workflow = ResilientWorkflow(client, tools, max_retries=3)

try:
    result = workflow.execute_with_retry(
        {"type": "left_click", "coordinate": [640, 400]},
        context="Click Submit button"
    )
    print("Action succeeded")
except Exception as e:
    print(f"Action failed after retries: {e}")
```

---

### Pattern 4: Data Extraction and Validation

Extract structured data from screens and validate accuracy.

**Implementation**:
```python
class DataExtractor:
    def __init__(self, client, tools):
        self.client = client
        self.tools = tools
        self.extracted_data = {}

    def extract_form_data(self, field_descriptors):
        """Extract data from form based on field descriptions."""
        messages = [
            {
                "role": "user",
                "content": f"Extract the following data from the visible form: {field_descriptors}"
            }
        ]

        response = self.client.messages.create(
            model="claude-opus-4-5-20250929",
            max_tokens=2048,
            tools=self.tools,
            messages=messages,
            headers={"anthropic-beta": "computer-use-2025-11-24"}
        )

        # Process response to extract data
        return self._parse_extraction(response)

    def extract_with_zoom(self, regions):
        """Extract data by zooming into specific regions."""
        extracted = {}

        for region_name, coordinates in regions.items():
            # Zoom into region
            action = {
                "type": "zoom",
                "coordinate": coordinates
            }
            # Execute zoom and analyze result
            extracted[region_name] = self._analyze_region(action)

        return extracted

    def validate_extraction(self, extracted_data, validation_rules):
        """Validate extracted data against rules."""
        validation_results = {}

        for field, rules in validation_rules.items():
            value = extracted_data.get(field)
            is_valid = all(rule(value) for rule in rules)
            validation_results[field] = {
                "value": value,
                "valid": is_valid
            }

        return validation_results

    def _parse_extraction(self, response):
        """Parse Claude's response for extracted data."""
        # Process response content
        return {}

    def _analyze_region(self, action):
        """Analyze zoomed region."""
        return "region_data"
```

**Example**:
```python
extractor = DataExtractor(client, tools)

# Define what to extract
fields = {
    "customer_name": "Full name field",
    "email": "Email address field",
    "phone": "Phone number field",
    "address": "Mailing address"
}

# Extract data
extracted = extractor.extract_form_data(fields)

# Validate
rules = {
    "email": [lambda x: "@" in x, lambda x: len(x) > 5],
    "phone": [lambda x: len(x) >= 10],
    "customer_name": [lambda x: len(x) > 0]
}

results = extractor.validate_extraction(extracted, rules)
print(results)
```

---

## Zoom Tool Advanced Usage

### Zoom for Precision Clicking

Use zoom to find exact coordinates of elements, especially in crowded interfaces.

**Workflow**:
```python
def precise_click(coordinate_estimate, region_margin=50):
    """Click element with zoom-enhanced precision."""
    messages = []

    # 1. Take screenshot
    messages.append({
        "role": "user",
        "content": [{"type": "text", "text": "Take a screenshot"}]
    })

    response = client.messages.create(
        model="claude-opus-4-5-20250929",
        max_tokens=1024,
        tools=tools,
        messages=messages,
        headers={"anthropic-beta": "computer-use-2025-11-24"}
    )

    # 2. Zoom into region around estimated coordinate
    zoom_region = [
        coordinate_estimate[0] - region_margin,
        coordinate_estimate[1] - region_margin,
        coordinate_estimate[0] + region_margin,
        coordinate_estimate[1] + region_margin
    ]

    messages.append({
        "role": "assistant",
        "content": response.content
    })
    messages.append({
        "role": "user",
        "content": [
            {"type": "tool_result", "tool_use_id": "screenshot_id", "content": "screenshot"}
        ]
    })

    # 3. Zoom for precision
    response = client.messages.create(
        model="claude-opus-4-5-20250929",
        max_tokens=1024,
        tools=tools,
        messages=[{
            "role": "user",
            "content": f"Zoom into region {zoom_region} to find exact coordinates of the element."
        }],
        headers={"anthropic-beta": "computer-use-2025-11-24"}
    )

    # 4. Click with precise coordinates from zoomed view
    # Claude will identify exact position in zoomed image
    return response
```

### Zoom for Text Reading

Read fine-print or small text that's hard to see in full screenshot.

**Implementation**:
```python
def read_small_text(screen_region):
    """Use zoom to read small text accurately."""
    # Zoom into text region
    action = {
        "type": "zoom",
        "coordinate": screen_region
    }

    response = client.messages.create(
        model="claude-opus-4-5-20250929",
        max_tokens=1024,
        tools=tools,
        messages=[{
            "role": "user",
            "content": [
                {
                    "type": "text",
                    "text": f"Zoom into region {screen_region} and read the text."
                }
            ]
        }],
        headers={"anthropic-beta": "computer-use-2025-11-24"}
    )

    # Extract text from response
    return response
```

### Zoom for Layout Analysis

Understand complex interface layouts by zooming into specific sections.

**Pattern**:
```python
def analyze_layout():
    """Analyze interface layout by examining sections."""
    sections = {
        "header": [0, 0, 1024, 100],
        "navigation": [0, 100, 200, 600],
        "content": [200, 100, 1024, 600],
        "footer": [0, 600, 1024, 768]
    }

    layout_analysis = {}

    for section_name, region in sections.items():
        # Zoom into each section
        action = {"type": "zoom", "coordinate": region}
        # Analyze structure
        layout_analysis[section_name] = analyze_region(action)

    return layout_analysis
```

---

## Three Complex Automation Examples

### Example 1: Web Form Filling with Validation

**Scenario**: Fill multi-field form, validate inputs, and submit.

```python
def fill_and_validate_form(form_data):
    """Fill form with validation."""
    client = anthropic.Anthropic()
    tools = [get_computer_tool()]
    state = {"step": 0, "errors": []}

    messages = [
        {
            "role": "user",
            "content": f"""
            Fill out the web form with this data:
            - Name: {form_data['name']}
            - Email: {form_data['email']}
            - Phone: {form_data['phone']}
            - Address: {form_data['address']}

            Validate that:
            1. All fields are filled
            2. Email contains '@'
            3. Phone is 10+ digits
            4. Address is not empty

            Then submit the form.
            """
        }
    ]

    while state["step"] < 50:
        response = client.messages.create(
            model="claude-opus-4-5-20250929",
            max_tokens=2048,
            tools=tools,
            messages=messages,
            headers={"anthropic-beta": "computer-use-2025-11-24"}
        )

        if response.stop_reason == "end_turn":
            return {"success": True, "state": state}

        tool_use = next((b for b in response.content if b.type == "tool_use"), None)
        if tool_use:
            action = tool_use.input
            result = execute_action(action)

            messages.append({"role": "assistant", "content": response.content})
            messages.append({
                "role": "user",
                "content": [{
                    "type": "tool_result",
                    "tool_use_id": tool_use.id,
                    "content": result
                }]
            })

        state["step"] += 1

    return {"success": False, "error": "Max steps exceeded", "state": state}
```

### Example 2: Multi-Application Workflow

**Scenario**: Copy data from one application, paste into another.

```python
def copy_between_applications(source_app, source_field, dest_app, dest_field):
    """Copy data from source to destination application."""
    client = anthropic.Anthropic()
    tools = [get_computer_tool()]

    messages = [
        {
            "role": "user",
            "content": f"""
            1. Take a screenshot to see current state
            2. If {source_app} is not visible, click to bring it to focus
            3. Click on the {source_field} field
            4. Select all (Ctrl+A) and copy (Ctrl+C)
            5. Switch to {dest_app} (Alt+Tab or click taskbar)
            6. Click on the {dest_field} field
            7. Paste the data (Ctrl+V)
            8. Screenshot to confirm
            """
        }
    ]

    for step in range(50):
        response = client.messages.create(
            model="claude-opus-4-5-20250929",
            max_tokens=2048,
            tools=tools,
            messages=messages,
            headers={"anthropic-beta": "computer-use-2025-11-24"}
        )

        if response.stop_reason == "end_turn":
            return True

        tool_use = next((b for b in response.content if b.type == "tool_use"), None)
        if tool_use:
            result = execute_action(tool_use.input)
            messages.append({"role": "assistant", "content": response.content})
            messages.append({
                "role": "user",
                "content": [{
                    "type": "tool_result",
                    "tool_use_id": tool_use.id,
                    "content": result
                }]
            })

    return False
```

### Example 3: Application Testing with Assertions

**Scenario**: Test UI functionality and verify expected behavior.

```python
def test_application_flow(test_steps):
    """Execute test flow with assertions."""
    client = anthropic.Anthropic()
    tools = [get_computer_tool()]
    test_results = []

    for test in test_steps:
        messages = [
            {
                "role": "user",
                "content": f"""
                Execute this test step: {test['description']}

                Expected result: {test['expected']}

                Actions to perform:
                {json.dumps(test['actions'], indent=2)}

                After performing actions, take a screenshot and verify
                that the expected result is visible.
                """
            }
        ]

        for step_count in range(30):
            response = client.messages.create(
                model="claude-opus-4-5-20250929",
                max_tokens=2048,
                tools=tools,
                messages=messages,
                headers={"anthropic-beta": "computer-use-2025-11-24"}
            )

            if response.stop_reason == "end_turn":
                # Extract test result
                result_text = next(
                    (b.text for b in response.content if hasattr(b, "text")),
                    None
                )
                test_results.append({
                    "test": test['description'],
                    "passed": "passed" in result_text.lower(),
                    "details": result_text
                })
                break

            tool_use = next((b for b in response.content if b.type == "tool_use"), None)
            if tool_use:
                result = execute_action(tool_use.input)
                messages.append({"role": "assistant", "content": response.content})
                messages.append({
                    "role": "user",
                    "content": [{
                        "type": "tool_result",
                        "tool_use_id": tool_use.id,
                        "content": result
                    }]
                })

    return test_results
```

---

## Best Practices

### 1. Screenshot Efficiency
- **Minimize Screenshots**: Each screenshot costs tokens (vision processing)
- **Strategic Placement**: Take screenshots at decision points, not after every action
- **Batch Actions**: Group related actions before screenshots
- **Use Zoom**: For detail inspection instead of full-screen reshots

### 2. Coordinate Management
- **Record Positions**: Save successful coordinates for reuse
- **Use Zoom for Precision**: When uncertain, zoom before clicking
- **Test Coordinates**: Verify coordinates on first use
- **Handle Resolution Changes**: Don't hardcode absolute positions

### 3. Error Handling
- **Retry Logic**: Implement exponential backoff for transient failures
- **State Recovery**: Maintain state to recover from interruptions
- **Logging**: Record all actions for debugging
- **Timeout Protection**: Set maximum iterations to prevent infinite loops

### 4. Workflow Design
- **Clear Objectives**: Start with well-defined goals
- **State Machine Approach**: Model as states with transitions
- **Modular Actions**: Break complex workflows into reusable steps
- **Progress Tracking**: Monitor and report progress regularly

### 5. Performance Optimization
- **Model Selection**: Use Opus 4.5 for complex tasks, Sonnet for simple ones
- **Token Budget**: Monitor token usage and plan accordingly
- **Parallel Steps**: When possible, combine actions to reduce round trips
- **Cache Management**: Reuse screenshots and analysis results

### 6. Robustness
- **Validate Inputs**: Check data before sending
- **Verify Actions**: Confirm successful execution
- **Handle Edge Cases**: Plan for unexpected states
- **Graceful Degradation**: Provide fallback strategies

### 7. Security & Limitations
- **Sandbox Isolation**: Run in isolated environments
- **Credential Handling**: Avoid storing or logging sensitive data
- **Permission Control**: Minimize application privileges
- **Input Validation**: Sanitize user inputs to prevent injection

### 8. Testing & Debugging
- **Dry Run**: Test workflows on safe test environments first
- **Screenshot Inspection**: Review screenshots to understand AI decisions
- **Step-by-Step Execution**: Execute manually to identify issues
- **Logging & Monitoring**: Capture detailed execution logs

---

## References

- [Computer Use Tool Documentation](https://platform.claude.com/docs/en/build-with-claude/computer-use)
- [Reference Implementation on GitHub](https://github.com/anthropics/anthropic-quickstarts/tree/main/computer-use-demo)
- [Claude Opus 4.5 Technical Details](https://www.anthropic.com/news/claude-opus-4-5)
- [Advanced Tool Use](https://www.anthropic.com/engineering/advanced-tool-use)
