import sys
from pathlib import Path

# Add python directory to path
python_dir = Path(__file__).parent / "python" if Path("python").exists() else Path(".")
sys.path.insert(0, str(python_dir))

import cye_module

def main() -> None:
    print("Hello from python!")

    config = cye_module.Config()
    config.instance_path = "../dataset/json/E-n22-k4.json"
    result = cye_module.run_optimization(config)
    print(result)
