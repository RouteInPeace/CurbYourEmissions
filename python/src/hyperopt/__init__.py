import sys
from pathlib import Path
import optuna

# Add python directory to path
python_dir = Path(__file__).parent / "python" if Path("python").exists() else Path(".")
sys.path.insert(0, str(python_dir))

import cye_module

def objective(trial):
    config = cye_module.Config()

    config.instance_path = "../dataset/json/E-n22-k4.json"
    config.population_size = trial.suggest_int("population_size", 5, 100)
    config.generation_cnt = 100
    config.elite_cnt = trial.suggest_int("elite_cnt", 0, config.population_size-1)
    
    return cye_module.stat_measurement(config, 16).min

def main() -> None:
    study = optuna.create_study(direction="minimize")
    study.optimize(objective, n_trials=10, n_jobs=1)
    print("Best params:", study.best_params)
