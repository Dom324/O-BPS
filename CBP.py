#!/usr/bin/env python3
import click
import scripts.print_results as print_results
import scripts.runall as runall
import scripts.cbp_vizier as vizier

from scripts import myconstants as mc

@click.group(context_settings=mc.CLIC_CONTEXT_SETTINGS)
def cli():
    pass

@click.command(context_settings=mc.CLIC_CONTEXT_SETTINGS)
def howto():
    """Quickstart guide how to use the CBP framework"""
    print("TEXT")

cli.add_command(runall.run_traces)
cli.add_command(print_results.print_results)
cli.add_command(vizier.vizier)
cli.add_command(howto)

if __name__ == '__main__':
    cli()

