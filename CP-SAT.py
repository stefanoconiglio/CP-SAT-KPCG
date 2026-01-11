# DOWNLOAD INSTANCES

import argparse
import os
import sys
import urllib.parse
import urllib.request
from pathlib import Path

parser = argparse.ArgumentParser(description="Solve knapsack instances with conflicts using CP-SAT.")
parser.add_argument(
    "--no-download",
    action="store_true",
    help="Skip downloading and unzipping instance archives."
)
parser.add_argument(
    "--no-install",
    action="store_true",
    help="Skip local ortools installation/venv setup."
)
args, extras = parser.parse_known_args()
# Be tolerant of callers that insert `--` before flags (argparse stops option parsing after `--`).
if "--no-download" in extras:
    args.no_download = True
if "--no-install" in extras:
    args.no_install = True

dropbox_urls = [
    "https://www.dropbox.com/scl/fi/y739yds3givrzuoao1b0u/C1.zip?rlkey=z22clyzvalloof4335cdosbea&dl=0",
    "https://www.dropbox.com/scl/fi/geoymn3ndrmp2rbkrrv6o/C3.zip?rlkey=bkee32fj67mlgg664vfm1jrlx&dl=0",
    "https://www.dropbox.com/scl/fi/254dl7d1vqh7o3fj5zxpg/C10.zip?rlkey=2ugfkzuo7tzro0kb1whplammv&dl=0",
    "https://www.dropbox.com/scl/fi/8v8d5mhf01vbvs29n9bh4/R3.zip?rlkey=9haa9ryoykfno81jy3u15q438&dl=0",
    "https://www.dropbox.com/scl/fi/57790j528scwlngdfiz1q/R10.zip?rlkey=owiwvl25j6h03qoi0i54705gf&dl=0",
    "https://www.dropbox.com/scl/fi/irs32pobjzxs9t6arym8o/sparse_corr.zip?rlkey=fsl6y7p2z2asg5ugc8e152xl2&dl=0",
    "https://www.dropbox.com/scl/fi/f9sznsnp78g5lgcn0ws77/sparse_rand.zip?rlkey=nh7bjqwvh7etd3v4y6jtcrrf3&dl=0"
]

processed_urls = []
for url in dropbox_urls:
    parsed_url = urllib.parse.urlparse(url)
    query_params = urllib.parse.parse_qs(parsed_url.query)
    query_params['dl'] = ['1'] # Force direct download
    new_query = urllib.parse.urlencode(query_params, doseq=True)
    direct_download_url = urllib.parse.urlunparse(parsed_url._replace(query=new_query))

    filename = os.path.basename(parsed_url.path)

    processed_urls.append((direct_download_url, filename))

if not args.no_download:
    for url, filename in processed_urls:
        print("Downloading {}...".format(filename))
        try:
            urllib.request.urlretrieve(url, filename)
            print("Download complete.")
        except Exception as e:
            print("Error downloading {}: {}".format(filename, e))

    print("All specified zip files have been downloaded.")

    # UNZIP
    import zipfile

    for url, filename in processed_urls:
        print("Unzipping {}...".format(filename))
        try:
            with zipfile.ZipFile(filename, 'r') as zip_ref:
                zip_ref.extractall()
            print("Finished unzipping {}.".format(filename))
        except zipfile.BadZipFile:
            print("Error: {} is not a valid zip file.".format(filename))
        except FileNotFoundError:
            print("Error: {} not found. Was it downloaded successfully?".format(filename))
        except Exception as e:
            print("An unexpected error occurred while unzipping {}: {}".format(filename, e))

    print("All zip files have been unzipped.")

# INSTALL ORTOOLS (self-contained)

import subprocess

def ensure_ortools():
    """Install ortools into a local venv if missing, then import it."""
    try:
        import ortools  # noqa: F401
        return
    except ImportError:
        print("ortools not found; creating local venv and installing...")

    project_root = Path(__file__).resolve().parent
    venv_dir = project_root / ".venv"
    is_windows = sys.platform.startswith("win")
    python_in_venv = venv_dir / ("Scripts" if is_windows else "bin") / ("python.exe" if is_windows else "python")
    pip_in_venv = venv_dir / ("Scripts" if is_windows else "bin") / ("pip.exe" if is_windows else "pip")

    if not python_in_venv.exists():
        subprocess.check_call([sys.executable, "-m", "venv", str(venv_dir)])

    # Upgrade pip first to improve install reliability
    subprocess.check_call([str(pip_in_venv), "install", "--upgrade", "pip"])
    subprocess.check_call([str(pip_in_venv), "install", "ortools"])

    # Add venv site-packages to sys.path so imports work in this process
    py_ver = "python{}.{}".format(sys.version_info.major, sys.version_info.minor)
    site_packages = venv_dir / ("Lib" if is_windows else "lib") / py_ver / "site-packages"
    sys.path.insert(0, str(site_packages))
    print("ortools installed in local venv.")

if not args.no_install:
    ensure_ortools()

# SETUP CP-SAT MODEL

from ortools.sat.python import cp_model

def solve_knapsack_with_conflicts(weights, values, capacity, conflicts):
    """
    weights: list[int]
    values:  list[int]
    capacity: int
    conflicts: iterable[tuple[int,int]]  # pairs (i,j) that cannot both be 1
    """
    n = len(weights)
    assert len(values) == n

    model = cp_model.CpModel()

    x = [model.NewBoolVar("x[{}]".format(i)) for i in range(n)]

    # Capacity
    model.Add(sum(weights[i] * x[i] for i in range(n)) <= capacity)

    # Conflicts (pairwise)
    for i, j in conflicts:
        if i == j:
            model.Add(x[i] == 0)  # degenerate, if ever present
        else:
            model.Add(x[i] + x[j] <= 1)

    # Objective
    model.Maximize(sum(values[i] * x[i] for i in range(n)))

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = 30.0
    # solver.parameters.num_search_workers = 8  # set if you want parallel

    status = solver.Solve(model)

    if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        return None

    picked = [i for i in range(n) if solver.Value(x[i]) == 1]
    total_w = sum(weights[i] for i in picked)
    total_v = sum(values[i] for i in picked)
    return {"picked": picked, "total_weight": total_w, "total_value": total_v}

# Example
if __name__ == "__main__":
    weights = [2, 3, 4, 5, 9]
    values  = [3, 4, 5, 8, 10]
    capacity = 10
    conflicts = [(0, 1), (2, 3)]  # can't take (0&1) together, nor (2&3)
    print(solve_knapsack_with_conflicts(weights, values, capacity, conflicts))


# SOLVE EVERYTHING

import glob
import time

def parse_knapsack_file(filepath):
    """
    Parses a knapsack problem instance file with conflicts
    according to the observed format.
    """
    weights = []
    values = []
    conflicts = []
    capacity = 0
    num_items = 0

    with open(filepath, 'r') as f:
        lines = f.readlines()

    state = ""  # 'header', 'items', 'conflicts'

    for line in lines:
        line = line.strip()
        if not line or line.startswith('#'):  # Skip empty lines or comments
            continue

        if line.startswith('param n :='):
            num_items = int(line.split(':=')[1].replace(';', '').strip())
        elif line.startswith('param c :='):
            capacity = int(line.split(':=')[1].replace(';', '').strip())
        elif line.startswith('param : V : p w :='):
            state = 'items'
            continue
        elif line.startswith('set E :='):
            state = 'conflicts'
            continue
        elif line == ';': # End of section marker
            state = 'header' # Reset state or handle as needed, assuming sections are distinct
            continue

        if state == 'items':
            parts = line.split()
            if len(parts) == 3: # Expecting item_idx, value, weight
                # item_idx = int(parts[0]) # Not strictly needed if we just append
                value = int(parts[1])
                weight = int(parts[2])
                values.append(value)
                weights.append(weight)
        elif state == 'conflicts':
            parts = line.split()
            if len(parts) == 2: # Expecting item_i, item_j
                item_i = int(parts[0])
                item_j = int(parts[1])
                conflicts.append((item_i, item_j))

    if not values: # Fallback for cases where 'param n' is not explicit and 'items' section defines item count
        # If num_items was read, ensure consistency or adjust length of values/weights if parsing logic is off
        # For now, ensure consistency:
        if num_items and len(values) != num_items:
            print("Warning: Parsed {} items, but n states {}.".format(len(values), num_items))

    return weights, values, capacity, conflicts

# Find all instance files in the unzipped directories
instance_file_patterns = [
    'C1/BPPC_*.txt_*',
    'C3/BPPC_*.txt_*',
    'C10/BPPC_*.txt_*',
    'R3/BPPC_*.txt_*',
    'R10/BPPC_*.txt_*',
    'sparse_corr/test_*.dat',
    'sparse_rand/test_*.dat'
]

instance_files = []
for pattern in instance_file_patterns:
    instance_files.extend(glob.glob(pattern))
instance_files.sort() # for consistent order

print("Found {} knapsack instance files.\n".format(len(instance_files)))

results = {}
for filepath in instance_files:
    print("Solving instance: {}".format(filepath))
    start_time = time.time()
    try:
        weights, values, capacity, conflicts = parse_knapsack_file(filepath)
        # The solve_knapsack_with_conflicts function was defined in a previous cell
        result = solve_knapsack_with_conflicts(weights, values, capacity, conflicts)
        end_time = time.time()
        duration = end_time - start_time
        if result:
            print("  Picked items: {}".format(result['picked']))
            print("  Total weight: {}".format(result['total_weight']))
            print("  Total value: {}".format(result['total_value']))
            print("  Time taken: {:.4f} seconds".format(duration))
            results[filepath] = result
            results[filepath]['time_taken'] = duration
        else:
            print("  No feasible solution found or solver timed out.")
            print("  Time taken: {:.4f} seconds".format(duration))
            results[filepath] = "No solution"
    except Exception as e:
        end_time = time.time()
        duration = end_time - start_time
        print("  Error processing {}: {}".format(filepath, e))
        print("  Time taken before error: {:.4f} seconds".format(duration))
        results[filepath] = "Error: {}".format(e)

print("\n--- Summary of all instances ---")
for filepath, res in results.items():
    if isinstance(res, dict):
        print("{}: Value={}, Weight={}, Picked={}, Time Taken={:.4f}s".format(
            filepath, res['total_value'], res['total_weight'], res['picked'], res['time_taken']
        ))
    else:
        print("{}: {}".format(filepath, res))
