#!/usr/bin/env python3
import os
import sys
import hashlib
from datetime import datetime
from pathlib import Path
import subprocess
import multiprocessing
from functools import partial
import signal
import shutil

import cloup
from cloup.constraints import mutually_exclusive
import yaml
#import git
import click

from . import myconstants as mc
from .utils import get_mpki

@cloup.command(context_settings=mc.CLIC_CONTEXT_SETTINGS)
@cloup.option_group(
    'Directories and config file',
    cloup.option("-t", "--trace_dir",   type=mc.CLICK_R_DIR,  show_default=True, default=mc.DEFAULT_TRACE_DIR,   help="Trace directory"),
    cloup.option("-r", "--result_dir",  type=mc.CLICK_W_DIR,  show_default=True, default=mc.DEFAULT_RESULT_DIR,  help="Where to create directory with simulation results"),
    cloup.option("-p", "--predictor_folder",    type=mc.CLICK_R_DIR, show_default=True, default=mc.DEFAULT_PREDICTOR_DIR, help="Default folder with predictor code"),
    cloup.option("-y", "--yml_file",    type=mc.CLICK_R_FILE, show_default=True, default=None, help="Override the .yml configuration file for predictor")
)
@cloup.option_group(
    'Options for conflicting result directories',
    cloup.option("-f", "--force",      is_flag=True,   help="Force running the simulator even if result folder already contains simulation results with matching hash"),
    cloup.option("-s", "--silence",    is_flag=True,   help="Silence warnings if result folder already contains simulation results with matching hash"),
    constraint=mutually_exclusive
)
@cloup.option("-u", "--user_stats",  show_default=True, is_flag=True,            help="Whether to collect predictor defined statistics. If True, will define USER_STATS symbol")
@cloup.option("-n", "--num_threads",       show_default=True, default=os.cpu_count(),  help="How many simulator threads to run in parallel. Default: number of threads")
@cloup.option('-v', '--verbose',        count=True)
def run_traces(verbose, trace_dir, result_dir, yml_file, user_stats, num_threads, force, silence, predictor_folder):
    """Run predictor with the given configuration file on all traces"""

    print_commands = verbose > 0
    log_commands = verbose > 1

    if yml_file == None:
        yml_file = predictor_folder + "/config/predictor.yml"

    with open(yml_file, 'r') as f:
        config_yaml = yaml.safe_load(f)

    config_yaml.pop("reproduction", None)

    config_hash, param_hash = generate_hashes(config_yaml)
    predictor_name    = config_yaml['predictor']['name']
    predictor_version = str(config_yaml['predictor']['version'])
    result_directory  = Path(result_dir).joinpath(predictor_name, predictor_version, str(config_hash))

    skip_simulation = False
    if result_directory.exists():
        with open(str(Path(result_directory).joinpath('predictor.yml')), 'r') as f:
            result_yaml = yaml.safe_load(f)
        mpki = result_yaml['reproduction']['MPKI']
        if silence:
            skip_simulation = True
        elif not force:
            print("MPKI is: " + str(mpki))
            sys.exit("Warning: result directory already contains a directory with the same configuration hash: \""
                     + str(config_hash) + "\".\nRun with -f/--force if you want to overwrite the "
                     + "simulation results, or with -s/--silence to silence this warning.")

    if not skip_simulation:
        def exit_gracefuly(signum, frame):
            signal.signal(signum, signal.SIG_IGN)       # Ignore additional signals
            shutil.rmtree(result_directory)
            sys.exit("SIGINT received. Directory \"" + str(result_directory) + "\" was removed to avoid incomplete simulation results")

        original_sigint_handler = signal.getsignal(signal.SIGINT)
        # Register function for cleanup in case of interrupt
        signal.signal(signal.SIGINT, exit_gracefuly)

        print("Running predictor " + predictor_name + " version " + predictor_version + " configuration " + config_hash + " ...")

        compile_predictor(predictor_folder, config_yaml, print_commands, log_commands, user_stats)
        simulate_predictor(result_directory, config_yaml, print_commands, log_commands, trace_dir, num_threads, original_sigint_handler)
        simulator_num_traces, simulator_mpki = get_mpki(result_directory)

        reproduction_dict = generate_reproduction_dict(config_hash, param_hash, simulator_mpki, simulator_num_traces)
        result_yaml = { **reproduction_dict, **config_yaml }

        # Write predictor.yml into the result directory
        with open( str(Path(result_directory).joinpath('predictor.yml')) ,'w') as f:
            yaml.dump(result_yaml, f, sort_keys=False)

        signal.signal(signal.SIGINT, original_sigint_handler)       # Restore signal handler

    num_traces = result_yaml['reproduction']['num_traces']
    mpki       = result_yaml['reproduction']['MPKI']

    print("MPKI for " + predictor_name + " version " + predictor_version + " configuration " + config_hash + " is " + f'{mpki:.3f}')

    return num_traces, mpki, str(result_directory)

def simulate_predictor(result_directory, config_yaml, print_commands, log_commands, trace_dir, parallel, original_sigint_handler):
    sim_exe = mc.BUILD_DIR.joinpath('predictor')

    result_directory.mkdir(parents=True, exist_ok=True)

    # Weird code because multiprocessing function cannot handle output of scandir directly
    traces = [entry.path for entry in os.scandir(trace_dir) if entry.is_file()]
    with multiprocessing.Pool(processes=parallel) as pool:
        partial_func = partial(simulate_trace, sim_exe=sim_exe, result_directory=result_directory, original_sigint_handler=original_sigint_handler)
        sim_threads = pool.imap_unordered(partial_func, traces)

        with click.progressbar(sim_threads, length=len(traces)) as bar:
            for result in bar:
                pass

    result_file = os.path.join(result_directory, "result.json")
    # Concatenate all result files
    subprocess.run("jq -c -s add " + str(result_directory) + "/*.json > " + result_file, shell=True)
    subprocess.run("find " + str(result_directory) + " ! -name 'result.json' -type f -exec rm -f {} +", shell=True)

def simulate_trace(trace, sim_exe, result_directory, original_sigint_handler):
    signal.signal(signal.SIGINT, original_sigint_handler)       # Restore signal handler
    command = os.path.relpath(sim_exe) + " " + os.path.relpath(trace) + " > " + os.path.relpath(result_directory) + "/" + os.path.basename(trace) + ".json"
    subprocess.run(command, shell=True)

def generate_hashes(dictionary):

    config_string = ""
    param_string = ""
    for param in dictionary['parameters'].items():
        config_string += param[0] + '=' + str(param[1]['val']) + ' '
        param_string += param[0] + ' '

    m1 = hashlib.sha3_256()
    m1.update(bytes(config_string, 'ascii'))
    config_hash = m1.hexdigest()

    m2 = hashlib.sha3_256()
    m2.update(bytes(param_string, 'ascii'))
    param_hash = m2.hexdigest()

    return config_hash, param_hash

def compile_predictor(predictor_folder, dictionary, print_commands, log_commands, user_stats):

    header_string = ""

    for param in dictionary['parameters'].items():
        p_name = param[0]
        p_type = param[1]['type']
        p_val  = param[1]['val']

        if p_type  == 'categorical':
            possible_values = param[1]['possible_values']
            if p_val not in possible_values:
                sys.exit("Error: Value \"" + p_val + "\" is not possible for parameter " + p_name)
            header_string += "#define " + p_val + '\n'
        elif p_type == 'bool':
            if not isinstance(p_val, bool):
                sys.exit("Error: Parameter " + p_name + " of type 'bool' does not have a bool value")
            if p_val == True:
                header_string += "#define " + p_name + '\n'
        elif (p_type == 'int') | (p_type == 'float'):
            header_string += "#define " + p_name + " " + str(p_val) + '\n'
        else:
            sys.exit("Error: Unknown parameter type \"" + p_type + "\"")

    header_file = mc.BUILD_DIR.joinpath('parameters.h')
    with open(str(header_file), "w") as f:
        f.write(header_string)

    statistics_string = ""
    if user_stats:
        statistics_string += "#define USER_STATS\n"

    statistics_file = mc.BUILD_DIR.joinpath('statistics.h')
    with open(str(statistics_file), "w") as f:
        f.write(statistics_string)

    paths_file = mc.BUILD_DIR.joinpath('paths.h')
    with open(str(paths_file), "w") as f:
        f.write("#define PREDICTOR_H_PATH \"" + predictor_folder + "/src/predictor.h\"")

    task_cmd = "PREDICTOR_FOLDER=" + predictor_folder + " task -d " + str(mc.SIM_DIR)
    if print_commands:
        print("Generating ../sim/build/parameters.h")
        print(task_cmd)

    try:
        subprocess.run(task_cmd, capture_output=not log_commands, text=True, shell=True, check=True)
    except subprocess.CalledProcessError as compile_exception:
        sys.exit("Error: Failed to compile, return code " + str(compile_exception.returncode))

def generate_reproduction_dict(config_hash, param_hash, mpki, num_traces):

    reproduction_dict = {}
    reproduction_dict['date_of_run'] = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
    reproduction_dict['MPKI'] = mpki
    reproduction_dict['num_traces'] = num_traces

#    reproduction_dict.update(get_git_repo_info(mc.DEFAULT_CONFIG_FILE, "predictor"))
#    reproduction_dict.update(get_git_repo_info(mc.CBP_DIR, "CBP"))

    reproduction_dict['config_hash'] = config_hash
    reproduction_dict['param_hash'] = param_hash

    return { 'reproduction' : reproduction_dict }

#def get_git_repo_info(path, name):
#
#    repo = git.Repo(path, search_parent_directories=True)
#    remote_url = repo.remotes[0].config_reader.get("url")
#    repo_name = remote_url.split('.git')[0].split(':')[1].split(".com/")[-1]
#
#    unpushed = repo.iter_commits(repo.active_branch.name + '@{u}..' + repo.active_branch.name)
#    branch_name = repo.active_branch.name
#    if repo.is_dirty():
#        branch_name += " (dirty)"
#    if len(list(unpushed)) != 0:
#        branch_name += " (unpushed)"
#
#    dictionary = {}
#    dictionary[name + '_repo_name'] = repo_name
#    dictionary[name + '_branch'] = branch_name
#    dictionary[name + '_commit'] = repo.head.commit.hexsha
#
#    return dictionary
