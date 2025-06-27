# Note: This code requires a full terminal environment that supports Textual. It may not work in all sandboxed or restricted environments.
# If `termios` errors persist, consider running it in a proper Unix-based terminal.

from datetime import datetime
import asyncio

try:
    from textual.app import App, ComposeResult
    from textual.widgets import TextArea, Footer, Header, Static
    from textual.containers import Container
except ModuleNotFoundError:
    print("[ERROR] Textual is not available in this environment. Please install it using 'pip install textual'.")
    raise

class MetaPanel(Static):
    """Display for current cursor position and time."""
    def update_meta(self, line, col):
        self.update(f"\U0001F4CD Cursor: ({line+1}, {col+1}) | \u23F0 {datetime.now().strftime('%H:%M:%S')}")
        def __init__(self, id: str | None = None) -> None:
            super().__init__(id=id)
            
class WSYSEditor(App):

    CSS = """
    Screen {
        layout: vertical;
    }

    #editor {
        border: round white;
        height: 1fr;
    }

    #meta {
        background: $accent-darken-1;
        color: $text;
        padding: 1 2;
    }
    """

    def compose(self) -> ComposeResult:
        yield Header()
        self.text_area = TextArea(id="editor")
        self.meta = MetaPanel(id="meta")
        yield Container(self.text_area)
        yield self.meta
        yield Footer()

    def on_mount(self):
        self.text_area.focus()
        self.set_interval(1, self.update_meta_info)

    def update_meta_info(self):
        try:
            line, col = self.text_area.cursor_position
            self.meta.update_meta(line, col)
        except Exception as e:
            self.meta.update(f"[Error reading cursor: {e}]")

if __name__ == "__main__":
    try:
        WSYSEditor().run()
    except Exception as e:
        print("[ERROR] Failed to run WSYSEditor:", e)
        print("Hint: This app requires a full terminal environment to function properly.")

