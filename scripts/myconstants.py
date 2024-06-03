#!/usr/bin/env python3
from pathlib import Path
import click

CLIC_CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

CLICK_W_DIR  = click.Path(             file_okay=False, writable=True, resolve_path=True)
CLICK_R_DIR  = click.Path(exists=True, file_okay=False, readable=True, resolve_path=True)
CLICK_R_FILE = click.Path(exists=True,  dir_okay=False, readable=True, resolve_path=True)

CBP_DIR               = Path(__file__).parent.absolute().joinpath('..').resolve()
DEFAULT_TRACE_DIR     = CBP_DIR.joinpath('traces', 'evaluationTraces.Final', 'evaluationTraces')
DEFAULT_RESULT_DIR    = CBP_DIR.joinpath('results')
SIM_DIR               = CBP_DIR.joinpath('sim')

BUILD_DIR             = SIM_DIR.joinpath('build')
DEFAULT_PREDICTOR_DIR = SIM_DIR.joinpath('cbp-predictors', 'gshare')
DEFAULT_CONFIG_FILE   = DEFAULT_PREDICTOR_DIR.joinpath('config', 'predictor.yml')

