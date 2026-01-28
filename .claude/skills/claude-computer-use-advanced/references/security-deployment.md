# Security & Deployment Guide

Production-grade security considerations, deployment patterns, and responsible use guidelines for Claude computer use.

## Overview

Computer use is a powerful capability that requires careful consideration of security, isolation, and responsible deployment. This guide covers best practices for safe, secure implementations.

---

## Security Considerations

### 1. Environment Isolation

**Critical**: Always deploy computer use agents in isolated, sandboxed environments with minimal privileges.

**Virtual Machine Isolation**:
```dockerfile
# Dockerfile example: Isolated computer use environment
FROM ubuntu:22.04

# Install minimal dependencies only
RUN apt-get update && apt-get install -y \
    xvfb \
    python3.11 \
    python3-pip \
    curl

# Create unprivileged user
RUN useradd -m -s /bin/bash automation

# Install dependencies
COPY requirements.txt /home/automation/
RUN pip install --no-cache-dir -r /home/automation/requirements.txt

# Copy application
COPY --chown=automation:automation app.py /home/automation/

# Run as unprivileged user
USER automation
WORKDIR /home/automation

# Run in restricted environment
RUN echo "ulimit -n 1024" >> ~/.bashrc
RUN echo "ulimit -u 10" >> ~/.bashrc

ENTRYPOINT ["python3", "app.py"]
```

**Network Isolation**:
```python
# Restrict network access to whitelisted domains only
ALLOWED_DOMAINS = [
    "example.com",
    "trusted-service.com",
    "internal-api.company.com"
]

def validate_network_access(url):
    """Ensure only whitelisted domains are accessible."""
    from urllib.parse import urlparse
    domain = urlparse(url).netloc
    if domain not in ALLOWED_DOMAINS:
        raise PermissionError(f"Access to {domain} not allowed")
    return True
```

**Permission Restrictions**:
```bash
#!/bin/bash
# Run container with minimal privileges

docker run \
    --rm \
    --read-only \
    --cap-drop=ALL \
    --cap-add=SYS_ADMIN \
    --security-opt=no-new-privileges:true \
    --ulimit nofile=1024:2048 \
    --ulimit nproc=256:512 \
    --memory=2g \
    --cpus=1 \
    computer-use-agent:latest
```

---

### 2. Credential Management

**Never Hardcode Credentials**:
```python
# WRONG - Never do this
API_KEY = "sk-..."
PASSWORD = "admin123"

# RIGHT - Use environment variables
import os
from dotenv import load_dotenv

load_dotenv()
api_key = os.getenv("ANTHROPIC_API_KEY")
db_password = os.getenv("DB_PASSWORD")

# BETTER - Use secrets manager
import subprocess

def get_credential(secret_name):
    """Retrieve secret from secure vault."""
    result = subprocess.run(
        ["aws", "secretsmanager", "get-secret-value",
         "--secret-id", secret_name],
        capture_output=True,
        text=True
    )
    return json.loads(result.stdout)["SecretString"]

# For computer use: only provide credentials when necessary
credentials = get_credential("web-form-credentials")
```

**Credential Handling in Automation**:
```python
class SecureAutomation:
    def __init__(self):
        self.credentials = {}
        self.requires_auth = False

    def minimize_credential_exposure(self):
        """Best practices for credential handling."""
        # 1. Request credentials only when needed
        if self.requires_auth:
            username = os.getenv("TEST_USERNAME")
            password = os.getenv("TEST_PASSWORD")
            # Use immediately, don't store
            return {"username": username, "password": password}

    def clear_after_use(self, credential_data):
        """Clear sensitive data after use."""
        import gc
        # Clear from memory
        for key in credential_data:
            credential_data[key] = None
        del credential_data
        gc.collect()

    def log_safely(self, message):
        """Log without exposing credentials."""
        # Strip credentials from logs
        safe_message = message.replace(
            os.getenv("TEST_PASSWORD", ""),
            "***REDACTED***"
        )
        print(f"[LOG] {safe_message}")
```

---

### 3. Audit Logging

Comprehensive logging for security, compliance, and debugging.

**Logging Implementation**:
```python
import logging
import json
from datetime import datetime

# Configure logging with security in mind
logging.basicConfig(
    filename="/var/log/automation/audit.log",
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)

class AuditLogger:
    def __init__(self, user_id, session_id):
        self.user_id = user_id
        self.session_id = session_id
        self.logger = logging.getLogger(__name__)

    def log_action(self, action_type, details, redact=False):
        """Log action with optional redaction."""
        log_entry = {
            "timestamp": datetime.utcnow().isoformat(),
            "user_id": self.user_id,
            "session_id": self.session_id,
            "action_type": action_type,
            "details": details if not redact else "***REDACTED***"
        }
        self.logger.info(json.dumps(log_entry))

    def log_tool_use(self, action):
        """Log computer use tool actions."""
        self.log_action(
            action_type="tool_use",
            details={
                "tool": "computer",
                "action_type": action.get("type"),
                # Don't log coordinates or sensitive details
                "timestamp": datetime.utcnow().isoformat()
            }
        )

    def log_error(self, error_type, message):
        """Log errors for investigation."""
        self.log_action(
            action_type="error",
            details={
                "error_type": error_type,
                "message": message
            }
        )

    def log_completion(self, success, result_summary):
        """Log workflow completion."""
        self.log_action(
            action_type="completion",
            details={
                "success": success,
                "result": result_summary
            }
        )

# Usage
logger = AuditLogger(user_id="user123", session_id="sess456")
logger.log_action("workflow_started", {"objective": "Fill form"})
logger.log_tool_use({"type": "screenshot"})
logger.log_completion(True, "Form submitted successfully")
```

---

### 4. Rate Limiting & Quotas

Prevent abuse and manage resource consumption.

**Rate Limiting**:
```python
from datetime import datetime, timedelta
from collections import defaultdict

class RateLimiter:
    def __init__(self, max_requests=100, time_window_minutes=60):
        self.max_requests = max_requests
        self.time_window = timedelta(minutes=time_window_minutes)
        self.requests = defaultdict(list)

    def check_rate_limit(self, user_id):
        """Check if user has exceeded rate limit."""
        now = datetime.utcnow()
        cutoff = now - self.time_window

        # Clean old requests
        self.requests[user_id] = [
            req_time for req_time in self.requests[user_id]
            if req_time > cutoff
        ]

        # Check limit
        if len(self.requests[user_id]) >= self.max_requests:
            remaining_time = (
                self.requests[user_id][0] + self.time_window - now
            ).total_seconds()
            raise RateLimitError(
                f"Rate limit exceeded. Try again in {remaining_time:.0f} seconds"
            )

        # Record request
        self.requests[user_id].append(now)
        return True

# Usage
rate_limiter = RateLimiter(max_requests=10, time_window_minutes=60)

def execute_automation(user_id, objective):
    """Execute with rate limiting."""
    try:
        rate_limiter.check_rate_limit(user_id)
        return run_automation(objective)
    except RateLimitError as e:
        return {"error": str(e)}
```

**Token Quota Management**:
```python
class TokenQuotaManager:
    def __init__(self, user_id, monthly_token_limit):
        self.user_id = user_id
        self.monthly_limit = monthly_token_limit
        self.current_usage = 0

    def can_execute(self, estimated_tokens):
        """Check if execution would exceed quota."""
        if self.current_usage + estimated_tokens > self.monthly_limit:
            remaining = self.monthly_limit - self.current_usage
            raise QuotaExceededError(
                f"Insufficient quota. {remaining} tokens remaining"
            )
        return True

    def record_usage(self, tokens_used):
        """Record token usage."""
        self.current_usage += tokens_used

# Usage
quota = TokenQuotaManager(user_id="user123", monthly_token_limit=100000)
quota.can_execute(estimated_tokens=5000)
# ... execute automation ...
quota.record_usage(tokens_used=4850)
```

---

## Deployment Patterns

### Pattern 1: Docker-Based Isolated Environment

Production-grade Docker deployment with security hardening.

**Dockerfile**:
```dockerfile
FROM ubuntu:22.04

# Install base dependencies
RUN apt-get update && apt-get install -y \
    xvfb \
    x11-utils \
    python3.11 \
    python3-pip \
    supervisor

# Install X11 utilities for display
RUN apt-get install -y xterm xauth

# Create non-root user
RUN useradd -m -s /bin/bash automation
RUN mkdir -p /home/automation/.Xauthority

# Install Python dependencies
WORKDIR /home/automation
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY --chown=automation:automation app.py .
COPY --chown=automation:automation config.yaml .

# Supervisor configuration for process management
COPY --chown=automation:automation supervisord.conf /etc/supervisor/conf.d/

# Restrict file system
RUN chmod -R 750 /home/automation

USER automation

# Set display
ENV DISPLAY=:1
ENV XAUTHORITY=/home/automation/.Xauthority

ENTRYPOINT ["supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]
```

**Docker Compose**:
```yaml
version: '3.8'

services:
  computer-use-agent:
    build: .
    image: computer-use-agent:latest
    container_name: automation-worker

    # Security settings
    read_only: true
    cap_drop:
      - ALL
    cap_add:
      - SYS_ADMIN
    security_opt:
      - no-new-privileges:true

    # Resource limits
    mem_limit: 2g
    cpus: '1'
    pids_limit: 256

    # Environment
    environment:
      - ANTHROPIC_API_KEY=${ANTHROPIC_API_KEY}
      - DISPLAY=:1
      - XAUTHORITY=/home/automation/.Xauthority

    # Networking
    networks:
      - isolated

    # Logging
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"

    # Volumes (minimal)
    volumes:
      - /tmp/.X11-unix:/tmp/.X11-unix
      - automation_logs:/home/automation/logs:rw
      - automation_state:/home/automation/.state:rw

networks:
  isolated:
    driver: bridge
    ipam:
      config:
        - subnet: 172.20.0.0/16

volumes:
  automation_logs:
  automation_state:
```

### Pattern 2: Agent Loop Implementation

Production-ready agent loop with error handling and monitoring.

**Agent Loop**:
```python
import anthropic
import logging
from datetime import datetime
from typing import Optional

class ProductionAgentLoop:
    def __init__(self, user_id, max_iterations=50):
        self.client = anthropic.Anthropic()
        self.user_id = user_id
        self.max_iterations = max_iterations
        self.audit_logger = AuditLogger(user_id, session_id=str(datetime.utcnow()))
        self.iteration_count = 0
        self.start_time = datetime.utcnow()

    def execute(self, objective: str, tools_config: dict) -> dict:
        """Execute automation with monitoring."""
        try:
            self.audit_logger.log_action("execution_started", {"objective": objective})

            messages = [{"role": "user", "content": objective}]

            while self.iteration_count < self.max_iterations:
                # Check timeout (15 minute max)
                elapsed = (datetime.utcnow() - self.start_time).total_seconds()
                if elapsed > 900:
                    raise TimeoutError("Execution timeout exceeded")

                # Get next action
                response = self.client.messages.create(
                    model="claude-opus-4-5-20250929",
                    max_tokens=2048,
                    tools=[tools_config],
                    messages=messages,
                    headers={"anthropic-beta": "computer-use-2025-11-24"}
                )

                # Check for completion
                if response.stop_reason == "end_turn":
                    result_text = next(
                        (b.text for b in response.content if hasattr(b, "text")),
                        None
                    )
                    self.audit_logger.log_completion(True, result_text or "Success")
                    return {
                        "success": True,
                        "result": result_text,
                        "iterations": self.iteration_count
                    }

                # Process tool use
                tool_use = next(
                    (b for b in response.content if b.type == "tool_use"),
                    None
                )

                if tool_use:
                    action = tool_use.input
                    self.audit_logger.log_tool_use(action)

                    # Execute action
                    try:
                        result = self._execute_action(action)
                        messages.append({"role": "assistant", "content": response.content})
                        messages.append({
                            "role": "user",
                            "content": [{
                                "type": "tool_result",
                                "tool_use_id": tool_use.id,
                                "content": result
                            }]
                        })
                    except Exception as e:
                        self.audit_logger.log_error("action_failed", str(e))
                        # Notify Claude of error for recovery
                        messages.append({
                            "role": "user",
                            "content": f"Action failed: {str(e)}. Please retry or use alternate approach."
                        })

                self.iteration_count += 1

            raise RuntimeError(f"Max iterations ({self.max_iterations}) exceeded")

        except Exception as e:
            self.audit_logger.log_error("execution_failed", str(e))
            return {
                "success": False,
                "error": str(e),
                "iterations": self.iteration_count
            }

    def _execute_action(self, action: dict) -> str:
        """Execute action and return result."""
        action_type = action.get("type")

        if action_type == "screenshot":
            return "screenshot_data"
        elif action_type == "left_click":
            return "clicked"
        elif action_type == "type":
            return "typed"
        elif action_type == "key":
            return "key pressed"
        elif action_type == "zoom":
            return "zoomed"
        else:
            return "executed"

# Usage
agent = ProductionAgentLoop(user_id="user123", max_iterations=50)
result = agent.execute(
    objective="Fill out the contact form with sample data",
    tools_config={
        "type": "computer_20251124",
        "name": "computer",
        "display_width_px": 1024,
        "display_height_px": 768,
        "display_number": ":1",
        "enable_zoom": True
    }
)
print(result)
```

### Pattern 3: Monitoring & Observability

Track performance, errors, and resource usage.

**Monitoring Implementation**:
```python
import time
from prometheus_client import Counter, Histogram, Gauge

# Define metrics
execution_counter = Counter(
    'automation_executions_total',
    'Total executions',
    ['status', 'user_id']
)

execution_duration = Histogram(
    'automation_execution_duration_seconds',
    'Execution duration',
    ['user_id']
)

active_executions = Gauge(
    'automation_active_executions',
    'Currently active executions'
)

tokens_used = Counter(
    'automation_tokens_used_total',
    'Total tokens used',
    ['model', 'user_id']
)

class MonitoredAgent:
    def __init__(self, user_id):
        self.user_id = user_id
        active_executions.inc()

    def execute(self, objective):
        """Execute with monitoring."""
        start_time = time.time()

        try:
            # Run automation
            result = self._run_automation(objective)

            # Record success
            duration = time.time() - start_time
            execution_counter.labels(status='success', user_id=self.user_id).inc()
            execution_duration.labels(user_id=self.user_id).observe(duration)
            tokens_used.labels(model='opus-4-5', user_id=self.user_id).inc(4850)

            return result

        except Exception as e:
            execution_counter.labels(status='error', user_id=self.user_id).inc()
            raise

        finally:
            active_executions.dec()

    def _run_automation(self, objective):
        return {"status": "complete"}
```

---

## Limitations & Constraints

### Known Limitations

**Vision Accuracy**:
- Text recognition not 100% accurate in all scenarios
- Small UI elements may be misidentified
- Color-dependent interfaces can be unreliable
- Use zoom tool (Opus 4.5) for critical elements

**Performance**:
- Latency unsuitable for real-time interactive tasks
- Each screenshot incurs vision processing overhead
- Network delays can affect responsiveness
- Not suitable for time-critical operations

**Application Compatibility**:
- Spreadsheet automation unreliable
- Legacy/proprietary applications may not work
- JavaScript-heavy SPAs have variable reliability
- Some custom UI frameworks unsupported

**Account Management**:
- Cannot reliably create new accounts
- Social platform content sharing problematic
- Account security features (CAPTCHA) may block
- Email verification flows challenging

**Security**:
- Vulnerable to prompt injection via web content
- Cannot fully isolate from malicious web content
- Screenshots may contain sensitive data
- Credential handling requires careful management

### Mitigation Strategies

**For Vision Accuracy**:
```python
# Use zoom for critical elements
if need_precision:
    action = {"type": "zoom", "coordinate": region}
    # Get high-resolution view
```

**For Latency**:
```python
# Don't use for time-sensitive tasks
if is_time_critical:
    use_api_instead()
else:
    use_computer_automation()
```

**For Application Compatibility**:
```python
# Test thoroughly before production
test_applications = [
    "firefox",      # Well-supported
    "chrome",       # Well-supported
    "terminal",     # Works well
    "spreadsheet"   # May have issues
]
```

---

## Responsible Use Guidelines

### Ethical Considerations

1. **User Consent**: Always obtain explicit consent before automating
2. **Transparency**: Disclose automated nature of interactions
3. **Data Protection**: Handle personal/sensitive data responsibly
4. **Terms of Service**: Respect application TOSs
5. **Accessibility**: Don't interfere with accessibility features

### Legal Compliance

1. **Jurisdiction**: Understand local automation laws
2. **CFAA Compliance**: Ensure you have authorization
3. **GDPR/Privacy**: Comply with data protection regulations
4. **Audit Trails**: Maintain compliance-ready logs
5. **Documentation**: Document automation purposes and scope

### Best Practices

1. **Purpose Limitation**: Only automate authorized tasks
2. **Minimal Privilege**: Request only necessary permissions
3. **Monitoring**: Actively monitor for misuse
4. **Training**: Educate users on responsible use
5. **Review**: Regularly audit automation activities

---

## References

- [Computer Use Documentation](https://platform.claude.com/docs/en/build-with-claude/computer-use)
- [Reference Implementation](https://github.com/anthropics/anthropic-quickstarts/tree/main/computer-use-demo)
- [OWASP Security Guidelines](https://owasp.org/)
- [Container Security Best Practices](https://kubernetes.io/docs/concepts/security/)
