#!/usr/bin/env python3
"""
Sinphasé Toolkit CLI Implementation
OBINexus Computing - Unified Governance Framework
"""

import sys
from pathlib import Path
from typing import Optional
import json

try:
    import typer
    from rich.console import Console
    from rich.table import Table
    from rich.panel import Panel
    from rich.progress import Progress, SpinnerColumn, TextColumn
except ImportError:
    print("❌ Missing dependencies. Install with: pip install typer rich")
    sys.exit(1)

# Import core governance components
from .core.checker import GovernanceChecker
from .core.reporter import GovernanceReporter
from .core.refactorer import GovernanceRefactorer

app = typer.Typer(
    name="sinphase",
    help="🔍 Sinphasé Governance Toolkit - OBINexus Computing Enterprise Framework",
    rich_markup_mode="rich",
    add_completion=False
)

console = Console()

@app.command()
def check(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    threshold: Optional[float] = typer.Option(
        None, "--threshold", "-t", help="Governance threshold (0.0-1.0)"
    ),
    verbose: bool = typer.Option(
        False, "--verbose", "-v", help="Enable verbose output"
    ),
) -> None:
    """🔍 Execute comprehensive governance checks"""
    
    if project_root is None:
        project_root = Path.cwd()
    
    project_root = project_root.resolve()
    
    if not project_root.exists():
        console.print(f"❌ [red]Project root does not exist: {project_root}[/red]")
        raise typer.Exit(1)
    
    console.print(f"🔍 [bold]Running governance check on:[/bold] {project_root}")
    
    try:
        checker = GovernanceChecker(project_root, threshold=threshold)
        results = checker.run_comprehensive_check()
        
        # Display results
        console.print(Panel(
            f"💰 [bold]Total Cost:[/bold] {results['total_cost']:.3f}\n"
            f"⚠️  [bold]Violations:[/bold] {len(results['violations'])}\n"
            f"📊 [bold]Status:[/bold] {results['compliance_status']}",
            title="🔍 Governance Analysis Results",
            border_style="blue"
        ))
        
        if results['has_violations']:
            console.print("❌ [red]Governance violations detected[/red]")
            raise typer.Exit(1)
        else:
            console.print("✅ [green]Governance check passed[/green]")
            
    except Exception as e:
        console.print(f"❌ [red]Governance check failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def status(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
) -> None:
    """📈 Display governance status overview"""
    
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"📈 [bold]Governance Status for:[/bold] {project_root}")
    
    try:
        checker = GovernanceChecker(project_root)
        status_data = checker.get_governance_status()
        
        # Display status table
        table = Table(title="Sinphasé Governance Status", show_header=True)
        table.add_column("Metric", style="cyan")
        table.add_column("Value", style="green")
        table.add_column("Status", style="yellow")
        
        for metric, data in status_data.items():
            table.add_row(metric.title(), data["value"], data["status"])
        
        console.print(table)
        
    except Exception as e:
        console.print(f"❌ [red]Status check failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def report(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    format: str = typer.Option(
        "markdown", "--format", "-f", help="Report format: markdown, console"
    ),
) -> None:
    """📊 Generate governance report"""
    
    if project_root is None:
        project_root = Path.cwd()
    
    try:
        reporter = GovernanceReporter(project_root)
        report_content = reporter.generate_comprehensive_report()
        console.print(report_content)
        
    except Exception as e:
        console.print(f"❌ [red]Report generation failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def refactor(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    target: str = typer.Option(
        "ffi", "--target", "-t", help="Refactor target: ffi, includes, structure"
    ),
    dry_run: bool = typer.Option(
        True, "--dry-run", help="Preview changes without applying"
    ),
) -> None:
    """🔧 Execute governance-driven refactoring"""
    
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"🔧 [bold]Refactoring {target} for:[/bold] {project_root}")
    console.print(f"🔍 [bold]Mode:[/bold] {'Preview' if dry_run else 'Live execution'}")
    
    try:
        refactorer = GovernanceRefactorer(project_root)
        results = refactorer.run_targeted_refactor(target=target, dry_run=dry_run)
        
        console.print(f"\n📊 [bold]Results:[/bold]")
        console.print(f"Files processed: {results['files_processed']}")
        console.print(f"Changes made: {results['changes_made']}")
        
        if dry_run:
            console.print("\n💡 [yellow]Run with --dry-run=false to apply changes[/yellow]")
        
    except Exception as e:
        console.print(f"❌ [red]Refactoring failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def version() -> None:
    """Display version information"""
    from . import __version__, __author__
    console.print(f"Sinphasé Toolkit v{__version__}")
    console.print(f"Author: {__author__}")
    console.print("OBINexus Computing - Enterprise Governance Framework")

def main():
    """Main CLI entry point"""
    app()

if __name__ == "__main__":
    main()
