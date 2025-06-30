"""
Sinphasé Toolkit CLI - Main Interface
"""

import sys
from pathlib import Path
from typing import Optional

try:
    import typer
    from rich.console import Console
except ImportError:
    print("❌ Missing dependencies. Install with: pip install typer rich")
    sys.exit(1)

from .core.checker import GovernanceChecker
from .core.reporter import GovernanceReporter
from .core.refactorer import GovernanceRefactorer

app = typer.Typer(
    name="sinphase",
    help="🔍 Sinphasé Governance Toolkit - OBINexus Computing",
)

console = Console()

@app.command()
def check(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    threshold: Optional[float] = typer.Option(
        None, "--threshold", "-t", help="Governance threshold override"
    ),
    fail_on_violations: bool = typer.Option(
        False, "--fail-on-violations", help="Exit with error code on violations"
    ),
):
    """🔍 Run comprehensive governance checks"""
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"[blue]🔍 Running Sinphasé governance checks...[/blue]")
    console.print(f"[dim]Project: {project_root}[/dim]")
    
    try:
        checker = GovernanceChecker(project_root)
        results = checker.run_comprehensive_check(threshold=threshold)
        
        if results.get("violations", 0) > 0:
            console.print("[red]❌ Governance violations detected[/red]")
            for violation in results.get("violation_details", []):
                console.print(f"  • {violation}")
        else:
            console.print("[green]✅ No governance violations[/green]")
        
        console.print(f"Total cost: {results.get('total_cost', 0):.3f}")
        console.print(f"Threshold: {results.get('threshold', 0.6)}")
        
        if fail_on_violations and results.get("violations", 0) > 0:
            raise typer.Exit(1)
            
    except Exception as e:
        console.print(f"[red]❌ Error during governance check: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def report(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    output_file: Optional[Path] = typer.Option(
        None, "--output", "-o", help="Output file path"
    ),
):
    """📊 Generate comprehensive governance report"""
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"[blue]📊 Generating Sinphasé governance report...[/blue]")
    
    try:
        reporter = GovernanceReporter(project_root)
        report_content = reporter.generate_comprehensive_report()
        
        if output_file:
            output_file.write_text(report_content)
            console.print(f"[green]✅ Report saved to {output_file}[/green]")
        else:
            console.print(report_content)
            
    except Exception as e:
        console.print(f"[red]❌ Error generating report: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def refactor(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
    target: str = typer.Option(
        "ffi", "--target", "-t", help="Refactor target"
    ),
    dry_run: bool = typer.Option(
        True, "--dry-run/--execute", help="Show changes without applying"
    ),
):
    """🔧 Run automated governance-driven refactoring"""
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"[blue]🔧 Running Sinphasé refactoring...[/blue]")
    console.print(f"[dim]Target: {target}, Dry run: {dry_run}[/dim]")
    
    try:
        refactorer = GovernanceRefactorer(project_root)
        results = refactorer.run_targeted_refactor(target=target, dry_run=dry_run)
        
        console.print(f"Files processed: {results.get('files_processed', 0)}")
        console.print(f"Changes made: {results.get('changes_made', 0)}")
        
        if results.get("changes"):
            console.print("\n[yellow]Changes:[/yellow]")
            for change in results["changes"]:
                console.print(f"  • {change}")
                
    except Exception as e:
        console.print(f"[red]❌ Error during refactoring: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def status(
    project_root: Optional[Path] = typer.Option(
        None, "--project-root", "-p", help="Project root directory"
    ),
):
    """📈 Show current governance status"""
    if project_root is None:
        project_root = Path.cwd()
    
    console.print(f"[blue]📈 Sinphasé Governance Status[/blue]")
    
    try:
        checker = GovernanceChecker(project_root)
        status = checker.get_governance_status()
        
        for metric, data in status.items():
            console.print(f"{metric}: {data['value']} {data['status']}")
            
    except Exception as e:
        console.print(f"[red]❌ Error getting status: {e}[/red]")

def main():
    """CLI entry point"""
    app()

if __name__ == "__main__":
    main()
