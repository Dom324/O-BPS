# O-BPS - Open Branch Predictor Simulation framework
O-
Improvements compared to CBP2016:
1. The simulation speed has been greatly increased. For simple predictors such as Gshare or Bimodal, the simulation time went down from ~30 minutes down to ~1 minute.
2. The predictors now have a configuration file `predictor.yml` which describes the parameters avalaible in predictor.
3. The framework supports AI optimization of predictor parameters with [Google Vizier](https://github.com/google/vizier)

# Usage

# Usage

# Installation
Unfortunately the installation is a bit rough currently with several dependencies. If you experience difficulties, please don't hesitate and create a ticket :)

1. Clone this repository
2. Download the testing traces

    As the testing trace files are too big to be hosted on Git, please download them from [here]() and extract them such that your directory structure looks like this: `traces/evaluationTraces.Final/evaluationTraces/*.zst`.
3. Install a C++ compiler

    Compiler with support of C++20 is required. Clang-17 and GCC-13 were tested.
4. Install Boost library

    For example on Ubuntu:
    ```
    sudo apt install libboost-all-dev
    ```
    The library is expected to be installed in `/usr/include/boost`.
5. [Task](https://github.com/go-task/task) (make replacement)

    See the installation steps https://taskfile.dev/installation/ and install either with your preferred package manager, or by downloading binary from [Github releases](https://taskfile.dev/installation/) and adding it to PATH.
6. Python libraries

    Install Python libraries with:
    ```
    pip install -r /path/to/requirements.txt
    ```
    Note: Because Python is terrible, you will probably first need to create a [Virtual Enviroment](https://docs.python.org/3/library/venv.html) with `python -m venv /path/to/new/virtual/environment`. After that you can run the `pip install` command. Don't forget that you then need to run `source <path-to-venv>/bin/activate` on each new shell.
