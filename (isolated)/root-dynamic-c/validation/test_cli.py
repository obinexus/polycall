"""Test CLI Module"""
import pytest
from typer.testing import CliRunner
from sinphase_toolkit.cli import app

runner = CliRunner()

def test_cli_help():
    """Test CLI help command"""
    result = runner.invoke(app, ["--help"])
    assert result.exit_code == 0
    assert "Sinphasé" in result.stdout
