#!/usr/bin/env python3
import csv
import sys
from pathlib import Path
import datetime
import copy
import yaml
import os

from vizier.service import clients
from vizier.service import pyvizier as vz
import click

from . import runall
from . import myconstants as mc

def run_configuration(predictor_config, result_dir, user_stats, predictor_folder):

    predictor_config_file = mc.BUILD_DIR.joinpath('predictor.yml')
    with open(str(predictor_config_file ), 'w') as yamlfile:
        yaml.dump(predictor_config, yamlfile, sort_keys=False)

    run_options = ["--silence"]
    run_options += ["--yml_file", str(predictor_config_file)]
    run_options += ["--result_dir", str(result_dir)]
    run_options += ["--predictor_folder", str(predictor_folder)]

    if user_stats:
        run_options += ["--user_stats"]

    num_traces, mpki, folder = runall.run_traces(run_options, standalone_mode=False)

    return num_traces, mpki, folder

def compare_configs(predictor_config, loaded_config_dict, params_to_optimize):

    optimized_parameters = {}
    for param in predictor_config['parameters']:
        param_type = predictor_config['parameters'][param]['type']
        if param_type == 'int':
            param_val = int(predictor_config['parameters'][param]['val'])
            loaded_param_val = int(loaded_config_dict['parameters'][param]['val'])
        elif param_type == 'bool':
            param_val = predictor_config['parameters'][param]['val'] in ["true", "True", True]
            loaded_param_val = loaded_config_dict['parameters'][param]['val'] in ["true", "True", True]
        elif param_type == 'categorical':
            param_val = str(predictor_config['parameters'][param]['val'])
            loaded_param_val = str(loaded_config_dict['parameters'][param]['val'])
        else:
            sys.exit("Unknown param type!")

        if type(param_val) != type(loaded_param_val):
            sys.exit("Error during loading previous results, not matching type")

        if param in params_to_optimize:
            optimized_parameters[param] = loaded_param_val
            continue

        # If loaded config differs from config of this study, skip to next dir
        if param_val != loaded_param_val:
            return None, None

    mpki = loaded_config_dict['reproduction']['MPKI']

    return optimized_parameters, mpki

def vizier_func(vizier_iterations, result_dir, predictor_config, params_to_optimize, user_stats, extend_study, predictor_folder):

    # Algorithm, search space, and metrics.
    study_config = vz.StudyConfig(algorithm='GAUSSIAN_PROCESS_BANDIT')

    for param in params_to_optimize:
        param_type = predictor_config['parameters'][param]['type']
        if param_type == 'int':
            param_min = int(predictor_config['parameters'][param]['min'])
            param_max = int(predictor_config['parameters'][param]['max'])
            study_config.search_space.root.add_int_param(param, param_min, param_max)
        elif param_type == 'float':
            param_min = predictor_config['parameters'][param]['min']
            param_max = predictor_config['parameters'][param]['max']
            study_config.search_space.root.add_float_param(param, param_min, param_max)
        elif param_type == 'bool':
            study_config.search_space.root.add_bool_param(param)
        elif param_type == 'categorical':
            values = predictor_config['parameters'][param]['possible_values']
            study_config.search_space.root.add_categorical_param(param, feasible_values=values)
        else:
            sys.exit("Error: Unexpected parameter type, parameter \"" + param + "\" type \"" + param_type + "\"")

    study_config.metric_information.append(vz.MetricInformation('MPKI', goal=vz.ObjectiveMetricGoal.MINIMIZE))

    current_time = datetime.datetime.now()
    with open(str(current_time) + '.csv', 'w', newline='') as f:
        writer = csv.writer(f)

        header = ['Trial n°', 'MPKI'] + list(study_config.search_space.parameter_names) + ['Result folder']
        writer.writerow(header)

    study = clients.Study.from_study_config(study_config, owner='my_name', study_id='test')

    if extend_study:
        print("Loading previous results...")
        loaded_results = 0
        lowest_mpki = 100000000.0
        load_dir = result_dir.joinpath(predictor_config['predictor']['name']).joinpath(str(predictor_config['predictor']['version']))

        subfolders = [ Path(f) for f in os.scandir(load_dir) if f.is_dir() ]
        for folder in subfolders:
            loaded_config_file = folder.joinpath('predictor.yml')

            with open(loaded_config_file, 'r') as f:
                # This is a perf bottleneck, Python SUCKS
                loaded_config_dict = yaml.load(f, Loader=yaml.CBaseLoader)

            optimized_parameters, mpki = compare_configs(predictor_config, loaded_config_dict, params_to_optimize)

            if optimized_parameters != None:
                if float(mpki) < float(lowest_mpki):
                    lowest_mpki = mpki

                loaded_results += 1
                trial = vz.Trial(
                    parameters=optimized_parameters,
                    final_measurement=vz.Measurement(metrics={ 'MPKI': mpki }),
                )
                study._add_trial(trial)

        print("Number of loaded results: " + str(loaded_results))
        print("Lowest MPKI: " + str(lowest_mpki) + "\n")

    for i in range(vizier_iterations):
        suggestions = study.suggest(count=1)

        for suggestion in suggestions:
            params_to_optimize = suggestion.parameters

            current_config = copy.deepcopy(predictor_config)

            for param in params_to_optimize.items():
                param_type = current_config['parameters'][param[0]]['type']
                if param_type == 'int':
                    current_config['parameters'][param[0]]['val'] = int(param[1])
                elif param_type == 'float':
                    current_config['parameters'][param[0]]['val'] = float(param[1])
                elif param_type == 'bool':
                    current_config['parameters'][param[0]]['val'] = param[1] in ["true", "True"]
                else:
                    current_config['parameters'][param[0]]['val'] = param[1]

            num_traces, mpki, result_folder = run_configuration(current_config, result_dir, user_stats, predictor_folder)
            print("Trial: " + str(i) + "    MPKI: " + f'{mpki:.3f}' + "    Number of traces: " + str(num_traces) + "\n")
            suggestion.complete(vz.Measurement( {'MPKI': mpki } ))

            with open(str(current_time) + '.csv', 'a', newline='') as f:
                trial = [i, mpki] + list(params_to_optimize.values()) + [result_folder]
                writer = csv.writer(f)
                writer.writerow(trial)

    for optimal_trial in study.optimal_trials():
        optimal_trial = optimal_trial.materialize()
        mpki =             optimal_trial.final_measurement.metrics['MPKI'].value

        print("Optimal Trial Suggestion and Objective:", optimal_trial.parameters, mpki)
        return num_traces, mpki, str(current_time) + '.csv', optimal_trial.parameters


click_r_file = click.Path(exists=True, dir_okay=False, readable=True, resolve_path=True)

sim_dir = Path(__file__).parent.absolute().joinpath('..', 'sim')
default_study_config = sim_dir.joinpath('cbp-predictors', 'config', 'study_config.csv')

@click.command(context_settings=mc.CLIC_CONTEXT_SETTINGS)
@click.option("-s", "--study-config",   show_default=True, type=click_r_file, default=None, help="Override default study config file \"predictor/config/study_config.csv\"")
@click.option("-p", "--predictor_folder",    type=mc.CLICK_R_DIR, show_default=True, default=mc.DEFAULT_PREDICTOR_DIR, help="Default folder with predictor code")
@click.option("-u", "--user_stats",  show_default=True, is_flag=True,    help="Whether to collect predictor defined statistics. If True, will define USER_STATS symbol")
@click.option("-e", "--extend_study",   show_default=True, is_flag=True,    help="If true, before Vizier optimization is started CBP will go through the result folder and load all results corresponding to this configuration.")
def vizier(study_config, user_stats, extend_study, predictor_folder):
    """Run batch simulations and optimization studies defined in a csv file. If any of the parameters in csv file are marked as \"optimize\", will start an optimization study with Google Vizier."""

    cbp_vizier_API_version = 2.0            # TODO

    if study_config == None:
        study_config = predictor_folder + "/config/study_config.csv"

    with open(str(study_config), 'r') as f:
        csv_list = list(csv.reader(f, delimiter=","))

    header = csv_list[0]

    report_header = ['Trial n˚', 'MPKI'] + header
    with open('study_report.csv', 'w') as f:
        csv_writer = csv.writer(f)
        csv_writer.writerow(report_header)

    assert header[0] == 'Config file'
    assert header[1] == 'Result dir'
    assert header[2] == 'Vizier iterations'

    for i, csv_row in enumerate(csv_list[1:]):

        if csv_row[0] != '':
            config_file = csv_row[0]
        else:
            config_file = predictor_folder + "/config/predictor.yml"

        with open(config_file, 'r') as f:
            predictor_config = yaml.safe_load(f)

        result_dir = csv_row[1]
        if result_dir == '':
            result_dir = mc.DEFAULT_RESULT_DIR

        vizier_iterations = int(csv_row[2])

        params_to_optimize = []

        for name, value in zip(header[3:], csv_row[3:]):
            if name == '':
                continue
            if value == 'optimize':
                params_to_optimize += [name]
            elif value != '':
                if predictor_config['parameters'][name]['type'] == 'int':
                    predictor_config['parameters'][name]['val'] = int(value)
                elif predictor_config['parameters'][name]['type'] == 'bool':
                    predictor_config['parameters'][name]['val'] = bool(value)
                else:
                    predictor_config['parameters'][name]['val'] = value

        if params_to_optimize:
            if vizier_iterations == '0':
                sys.exit("Error: Number of iterations cannot be '0' when there is any parameter to optimize")
            num_traces, mpki, result_folder, optimized_params = vizier_func(vizier_iterations, result_dir, predictor_config, params_to_optimize, user_stats, extend_study, predictor_folder)
        else:
            num_traces, mpki, result_folder = run_configuration(predictor_config, result_dir, user_stats, predictor_folder)

        with open('study_report.csv', 'a') as f:
            csv_writer = csv.writer(f)

            row_to_write = [i, mpki, config_file, result_folder, vizier_iterations]
            for name, value in zip(header[3:], csv_row[3:]):
                if value == 'optimize':
                    row_to_write += [optimized_params[name]]
                else:
                    row_to_write += [predictor_config['parameters'][name]['val']]

            csv_writer.writerow(row_to_write)

