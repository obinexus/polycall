from typer import Typer

app = Typer(name="sinphase", help="Sinphasé Toolkit")

@app.command()
def dummy() -> None:
    """Dummy command."""
    pass

def main() -> None:
    app()
