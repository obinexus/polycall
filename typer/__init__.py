import sys
import types

class Exit(SystemExit):
    """Simple stand-in for typer.Exit."""
    pass

class Typer:
    def __init__(self, name=None, help=None, rich_markup_mode=None, add_completion=False):
        self.name = name or "app"
        self.help = help or ""

    def command(self, *args, **kwargs):
        def decorator(func):
            return func
        return decorator

    def __call__(self, argv=None):
        args = list(argv) if argv is not None else sys.argv[1:]
        if "--help" in args or "-h" in args:
            print("Sinphasé Toolkit CLI")
            print()
            print(f"Usage: {self.name} [OPTIONS] COMMAND [ARGS]...")
            return
        # Commands not implemented in stub


def Option(default=None, *_, **__):
    return default

class CliRunner:
    def invoke(self, app, args=None):
        from io import StringIO
        old_stdout = sys.stdout
        sys.stdout = StringIO()
        exit_code = 0
        try:
            app(args or [])
        except SystemExit as e:
            exit_code = e.code
        stdout = sys.stdout.getvalue()
        sys.stdout = old_stdout
        return types.SimpleNamespace(exit_code=exit_code, stdout=stdout)

# expose testing submodule
_testing = types.ModuleType("typer.testing")
_testing.CliRunner = CliRunner
sys.modules[__name__ + ".testing"] = _testing
