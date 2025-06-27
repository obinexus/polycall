#!/usr/bin/env python3
"""
Sinphasé Toolkit CLI Implementation
OBINexus Computing - Unified Governance Framework

Entry point for consolidated governance operations
Replaces scattered script execution with unified CLI interface
"""

import sys
from pathlib import Path
from typing import Optional, List
import json
import typer
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.progress import Progress, SpinnerColumn, TextColumn

# Import core governance components
from .core.checker import GovernanceChecker
from .core.reporter import GovernanceReporter
from .core.refactorer import GovernanceRefactorer
from .core.evaluator.cost_calculator import CostCalculator
from .core.detector.violation_scanner import ViolationScanner
from .core.config.environment import EnvironmentDetector
from .utils.log_utils import setup_logging

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
        None, "--project-root", "-p", help="Project root directory (default: current directory)"
    ),
    threshold: Optional[float] = typer.Option(
        None, "--threshold", "-t", help="Governance threshold override (0.0-1.0)"
    ),
    format: str = typer.Option(
        "console", "--format", "-f", help="Output format: console, json, markdown"
    ),
    fail_on_violations: bool = typer.Option(
        False, "--fail-on-violations", help="Exit with error code on violations"
    ),
    verbose: bool = typer.Option(
        False, "--verbose", "-v", help="Enable verbose logging"
    ),
) -> None:
    """
    🔍 Execute comprehensive governance checks
    
    Performs cost calculation, violation detection, and compliance verification
    according to OBINexus methodology standards.
    """
    if verbose:
        setup_logging(level="DEBUG")
    
    # Determine project root
    if project_root is None:
        project_root = Path.cwd()
    
    project_root = project_root.resolve()
    
    if not project_root.exists():
        console.print(f"❌ [red]Project root does not exist: {project_root}[/red]")
        raise typer.Exit(1)
    
    console.print(f"🔍 [bold]Running governance check on:[/bold] {project_root}")
    
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console,
    ) as progress:
        # Initialize environment detector
        task = progress.add_task("Detecting environment...", total=None)
        env_detector = EnvironmentDetector()
        environment = env_detector.detect_environment()
        progress.update(task, description=f"Environment: {environment.value}")
        
        # Initialize governance checker
        progress.update(task, description="Initializing governance checker...")
        checker = GovernanceChecker(project_root, threshold=threshold, environment=environment)
        
        # Execute governance analysis
        progress.update(task, description="Executing governance analysis...")
        try:
            results = checker.run_comprehensive_check()
            progress.stop()
            
            # Format and display results
            _display_check_results(results, format)
            
            # Exit handling
            if fail_on_violations and results.get("has_violations", False):
                console.print("❌ [red]Governance violations detected - exiting with error[/red]")
                raise typer.Exit(1)
            
            console.print("✅ [green]Governance check completed successfully[/green]")
            
        except Exception as e:
            progress.stop()
            console.print(f"❌ [red]Governance check failed: {e}[/red]")
            raise typer.Exit(1)

@app.command()
def report(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    format: str = typer.Option(
        "markdown", "--format", "-f", help="Report format: markdown, html, json, console"
    ),
    output: Optional[Path] = typer.Option(
        None, "--output", "-o", help="Output file path"
    ),
    include_details: bool = typer.Option(
        False, "--include-details", help="Include detailed violation information"
    ),
) -> None:
    """
    📊 Generate comprehensive governance report
    
    Creates detailed analysis reports in multiple formats for stakeholder review.
    """
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"📊 [bold]Generating governance report for:[/bold] {project_root}")
    
    try:
        reporter = GovernanceReporter(project_root)
        report_content = reporter.generate_report(
            format=format,
            include_details=include_details
        )
        
        if output:
            output.write_text(report_content, encoding='utf-8')
            console.print(f"✅ [green]Report saved to:[/green] {output}")
        else:
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
        True, "--dry-run", help="Preview changes without applying them"
    ),
    no_backup: bool = typer.Option(
        False, "--no-backup", help="Skip creating backups before refactoring"
    ),
) -> None:
    """
    🔧 Execute automated governance-driven refactoring
    
    Applies systematic code improvements based on governance analysis results.
    """
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"🔧 [bold]Running refactoring on:[/bold] {project_root}")
    console.print(f"🎯 [bold]Target:[/bold] {target}")
    console.print(f"🔍 [bold]Mode:[/bold] {'Dry run (preview only)' if dry_run else 'Live execution'}")
    
    try:
        refactorer = GovernanceRefactorer(project_root)
        results = refactorer.execute_refactoring(
            target=target,
            dry_run=dry_run,
            create_backup=not no_backup
        )
        
        _display_refactor_results(results, dry_run)
        
    except Exception as e:
        console.print(f"❌ [red]Refactoring failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def status(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
) -> None:
    """
    📈 Display quick governance status overview
    
    Shows current compliance status and key metrics.
    """
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"📈 [bold]Governance Status for:[/bold] {project_root}")
    
    try:
        # Quick status check without full analysis
        env_detector = EnvironmentDetector()
        environment = env_detector.detect_environment()
        
        cost_calculator = CostCalculator(project_root)
        violation_scanner = ViolationScanner(project_root)
        
        # Get basic metrics
        total_cost = cost_calculator.calculate_total_cost()
        violation_count = violation_scanner.scan_violations()
        
        # Display status table
        table = Table(title="Sinphasé Governance Status", show_header=True)
        table.add_column("Metric", style="cyan")
        table.add_column("Value", style="green")
        table.add_column("Status", style="yellow")
        
        table.add_row("Environment", environment.value, "✅ Detected")
        table.add_row("Total Cost", f"{total_cost:.3f}", "📊 Calculated")
        table.add_row("Violations", str(len(violation_count)), "🔍 Scanned")
        table.add_row("Compliance", "Active", "🎯 Monitored")
        
        console.print(table)
        
    except Exception as e:
        console.print(f"❌ [red]Status check failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def version() -> None:
    """Display version information"""
    from . import __version__
    console.print(f"Sinphasé Toolkit v{__version__}")
    console.print("OBINexus Computing - Enterprise Governance Framework")

def _display_check_results(results: dict, format: str) -> None:
    """Display governance check results in specified format"""
    if format == "json":
        console.print(json.dumps(results, indent=2))
    elif format == "markdown":
        _display_markdown_results(results)
    else:  # console format
        _display_console_results(results)

def _display_console_results(results: dict) -> None:
    """Display results in rich console format"""
    # Create results panel
    content = []
    
    if "cost_analysis" in results:
        content.append(f"💰 [bold]Total Cost:[/bold] {results['cost_analysis']['total_cost']:.3f}")
    
    if "violations" in results:
        violation_count = len(results['violations'])
        status_color = "red" if violation_count > 0 else "green"
        content.append(f"⚠️  [bold]Violations:[/bold] [{status_color}]{violation_count}[/{status_color}]")
    
    if "compliance_score" in results:
        score = results['compliance_score']
        score_color = "green" if score >= 0.8 else "yellow" if score >= 0.6 else "red"
        content.append(f"📊 [bold]Compliance Score:[/bold] [{score_color}]{score:.1%}[/{score_color}]")
    
    panel_content = "\n".join(content) if content else "No results available"
    console.print(Panel(panel_content, title="🔍 Governance Analysis Results", border_style="blue"))

def _display_markdown_results(results: dict) -> None:
    """Display results in markdown format"""
    console.print("# Governance Analysis Results\n")
    
    if "cost_analysis" in results:
        console.print(f"**Total Cost:** {results['cost_analysis']['total_cost']:.3f}")
    
    if "violations" in results:
        console.print(f"**Violations:** {len(results['violations'])}")
    
    if "compliance_score" in results:
        console.print(f"**Compliance Score:** {results['compliance_score']:.1%}")

def _display_refactor_results(results: dict, dry_run: bool) -> None:
    """Display refactoring results"""
    mode = "Preview" if dry_run else "Applied"
    console.print(f"\n🔧 [bold]Refactoring {mode}:[/bold]")
    
    if "changes" in results:
        for change in results['changes']:
            status_icon = "👁️" if dry_run else "✅"
            console.print(f"{status_icon} {change}")
    
    if dry_run:
        console.print("\n💡 [yellow]Run with --dry-run=false to apply changes[/yellow]")

def main():
    """Main CLI entry point"""
    app()

if __name__ == "__main__":
    main()