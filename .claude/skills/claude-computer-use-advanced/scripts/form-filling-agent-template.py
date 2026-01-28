#!/usr/bin/env python3
"""
Computer Use Agent Template: Web Form Filling

Complete working example of a multi-step automation workflow using Claude's
computer use tool. This agent can fill out web forms with provided data and submit them.

Usage:
    python form-filling-agent-template.py --objective "Fill out the contact form..."

Requires:
    - ANTHROPIC_API_KEY environment variable
    - anthropic Python package
    - X11 display for screenshot capture

Author: Claude Code
Date: 2025-11-25
Based on: https://platform.claude.com/docs/en/build-with-claude/computer-use
"""

import anthropic
import json
import logging
import sys
import argparse
from datetime import datetime
from typing import Optional, Dict, Any


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)


class ComputerUseAgent:
    """Multi-step automation agent using Claude's computer use tool."""

    def __init__(
        self,
        model: str = "claude-opus-4-5-20250929",
        max_iterations: int = 50,
        display_width: int = 1024,
        display_height: int = 768,
        display_number: str = ":1",
        enable_zoom: bool = True
    ):
        """Initialize the agent.

        Args:
            model: Claude model to use
            max_iterations: Maximum number of iterations before stopping
            display_width: Screen width in pixels
            display_height: Screen height in pixels
            display_number: X11 display number
            enable_zoom: Enable zoom tool (Opus 4.5 only)
        """
        self.client = anthropic.Anthropic()
        self.model = model
        self.max_iterations = max_iterations
        self.display_width = display_width
        self.display_height = display_height
        self.display_number = display_number
        self.enable_zoom = enable_zoom
        self.iteration = 0
        self.tool_use_count = 0

    def get_tool_config(self) -> Dict[str, Any]:
        """Get computer use tool configuration."""
        config = {
            "type": "computer_20251124",
            "name": "computer",
            "display_width_px": self.display_width,
            "display_height_px": self.display_height,
            "display_number": self.display_number
        }

        if self.enable_zoom:
            config["enable_zoom"] = True

        return config

    def execute(self, objective: str) -> Dict[str, Any]:
        """Execute automation workflow.

        Args:
            objective: The task to accomplish (e.g., "Fill out contact form with...")

        Returns:
            Dictionary with execution results including success status and summary
        """
        logger.info(f"Starting execution: {objective}")

        messages = [
            {
                "role": "user",
                "content": objective
            }
        ]

        start_time = datetime.utcnow()

        while self.iteration < self.max_iterations:
            logger.info(f"Iteration {self.iteration + 1}/{self.max_iterations}")

            try:
                # Call Claude with computer use tool
                response = self.client.messages.create(
                    model=self.model,
                    max_tokens=2048,
                    tools=[self.get_tool_config()],
                    messages=messages,
                    headers={"anthropic-beta": "computer-use-2025-11-24"}
                )

                # Check if Claude is done
                if response.stop_reason == "end_turn":
                    result_text = self._extract_text(response.content)
                    duration = (datetime.utcnow() - start_time).total_seconds()

                    logger.info(f"Execution completed in {duration:.1f}s after {self.iteration} iterations")

                    return {
                        "status": "success",
                        "result": result_text,
                        "iterations": self.iteration,
                        "tool_uses": self.tool_use_count,
                        "duration_seconds": duration
                    }

                # Process tool use actions
                tool_use_block = self._extract_tool_use(response.content)

                if tool_use_block:
                    action = tool_use_block.input
                    logger.info(f"Tool action: {action.get('type', 'unknown')}")

                    # Execute the action
                    result = self._execute_action(action)

                    # Add assistant response and result to conversation
                    messages.append({"role": "assistant", "content": response.content})
                    messages.append({
                        "role": "user",
                        "content": [
                            {
                                "type": "tool_result",
                                "tool_use_id": tool_use_block.id,
                                "content": result
                            }
                        ]
                    })

                    self.tool_use_count += 1
                else:
                    # Unexpected response without tool use
                    logger.warning("Response without tool use block")
                    break

            except Exception as e:
                logger.error(f"Error during execution: {e}")
                return {
                    "status": "error",
                    "error": str(e),
                    "iterations": self.iteration,
                    "tool_uses": self.tool_use_count
                }

            self.iteration += 1

        # Max iterations exceeded
        logger.warning(f"Max iterations ({self.max_iterations}) exceeded")
        return {
            "status": "incomplete",
            "error": "Max iterations exceeded",
            "iterations": self.iteration,
            "tool_uses": self.tool_use_count
        }

    def _extract_text(self, content) -> str:
        """Extract text content from response blocks."""
        for block in content:
            if hasattr(block, "text"):
                return block.text
        return "No text response"

    def _extract_tool_use(self, content):
        """Extract tool use block from response."""
        for block in content:
            if block.type == "tool_use":
                return block
        return None

    def _execute_action(self, action: Dict[str, Any]) -> str:
        """Execute a computer use action.

        In a real implementation, this would interface with an actual display
        and input system. For this template, we return simulated results.

        Args:
            action: The action to execute

        Returns:
            Result string describing what happened
        """
        action_type = action.get("type")

        if action_type == "screenshot":
            # In real implementation: capture screenshot from X11 display
            logger.debug("Would capture screenshot")
            return "Screenshot captured (base64_image_data_would_be_here)"

        elif action_type == "left_click":
            coordinate = action.get("coordinate", [0, 0])
            logger.debug(f"Would click at {coordinate}")
            return f"Clicked at coordinates {coordinate}"

        elif action_type == "right_click":
            coordinate = action.get("coordinate", [0, 0])
            logger.debug(f"Would right-click at {coordinate}")
            return f"Right-clicked at coordinates {coordinate}"

        elif action_type == "double_click":
            coordinate = action.get("coordinate", [0, 0])
            logger.debug(f"Would double-click at {coordinate}")
            return f"Double-clicked at coordinates {coordinate}"

        elif action_type == "type":
            text = action.get("text", "")
            logger.debug(f"Would type: {text[:50]}...")
            return f"Typed: {text}"

        elif action_type == "key":
            key = action.get("key", "")
            logger.debug(f"Would press key: {key}")
            return f"Pressed key: {key}"

        elif action_type == "mouse_move":
            coordinate = action.get("coordinate", [0, 0])
            logger.debug(f"Would move mouse to {coordinate}")
            return f"Moved mouse to {coordinate}"

        elif action_type == "scroll":
            direction = action.get("direction", "down")
            amount = action.get("amount", 1)
            logger.debug(f"Would scroll {direction} {amount} units")
            return f"Scrolled {direction} {amount} units"

        elif action_type == "drag":
            start = action.get("startCoordinate", [0, 0])
            end = action.get("endCoordinate", [0, 0])
            logger.debug(f"Would drag from {start} to {end}")
            return f"Dragged from {start} to {end}"

        elif action_type == "zoom":
            coordinate = action.get("coordinate", [0, 0, 100, 100])
            logger.debug(f"Would zoom into region {coordinate}")
            return f"Zoomed into region {coordinate}"

        else:
            logger.warning(f"Unknown action type: {action_type}")
            return f"Unknown action: {action_type}"


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Computer Use Agent: Fill web forms and automate tasks",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Fill out a contact form
  %(prog)s --objective "Fill the contact form with Name: John Doe, Email: john@example.com"

  # Test login form
  %(prog)s --objective "Log in with username 'admin' and password 'test123'"

  # Extract data from table
  %(prog)s --objective "Take a screenshot and tell me what's in the table"
        """
    )

    parser.add_argument(
        "--objective",
        type=str,
        required=False,
        default="Take a screenshot and describe what you see.",
        help="The task objective for the agent"
    )

    parser.add_argument(
        "--model",
        type=str,
        default="claude-opus-4-5-20250929",
        help="Claude model to use"
    )

    parser.add_argument(
        "--max-iterations",
        type=int,
        default=50,
        help="Maximum number of iterations"
    )

    parser.add_argument(
        "--display-width",
        type=int,
        default=1024,
        help="Display width in pixels"
    )

    parser.add_argument(
        "--display-height",
        type=int,
        default=768,
        help="Display height in pixels"
    )

    parser.add_argument(
        "--display-number",
        type=str,
        default=":1",
        help="X11 display number"
    )

    parser.add_argument(
        "--no-zoom",
        action="store_true",
        help="Disable zoom tool (for non-Opus 4.5 models)"
    )

    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose logging"
    )

    args = parser.parse_args()

    # Configure logging
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    # Create and run agent
    agent = ComputerUseAgent(
        model=args.model,
        max_iterations=args.max_iterations,
        display_width=args.display_width,
        display_height=args.display_height,
        display_number=args.display_number,
        enable_zoom=not args.no_zoom
    )

    logger.info("Initializing Computer Use Agent")
    logger.info(f"Model: {args.model}")
    logger.info(f"Display: {args.display_width}x{args.display_height} ({args.display_number})")
    logger.info(f"Zoom enabled: {not args.no_zoom}")
    logger.info("")

    result = agent.execute(args.objective)

    # Print results
    print("\n" + "=" * 60)
    print("EXECUTION RESULT")
    print("=" * 60)
    print(json.dumps(result, indent=2))
    print("=" * 60)

    # Return appropriate exit code
    return 0 if result["status"] == "success" else 1


if __name__ == "__main__":
    sys.exit(main())
