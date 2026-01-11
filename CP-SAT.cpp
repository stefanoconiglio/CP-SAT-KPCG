#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ortools/sat/cp_model.h"

namespace {

struct Result {
  std::vector<int> picked;
  long long total_weight = 0;
  long long total_value = 0;
  double time_taken = 0.0;
};

std::string Trim(const std::string& s) {
  const char* whitespace = " \t\r\n";
  const auto start = s.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return "";
  }
  const auto end = s.find_last_not_of(whitespace);
  return s.substr(start, end - start + 1);
}

bool StartsWith(const std::string& s, const std::string& prefix) {
  return s.rfind(prefix, 0) == 0;
}

std::string FilenameFromUrl(const std::string& url) {
  const auto slash = url.find_last_of('/');
  const auto query = url.find_first_of('?', slash == std::string::npos ? 0 : slash);
  if (slash == std::string::npos) {
    return url;
  }
  if (query == std::string::npos) {
    return url.substr(slash + 1);
  }
  return url.substr(slash + 1, query - slash - 1);
}

std::string MakeDirectDownloadUrl(const std::string& url) {
  std::string direct = url;
  const std::string from = "dl=0";
  const std::string to = "dl=1";
  const auto pos = direct.find(from);
  if (pos != std::string::npos) {
    direct.replace(pos, from.size(), to);
  }
  return direct;
}

int RunCommand(const std::string& cmd) {
  const int rc = std::system(cmd.c_str());
  if (rc != 0) {
    std::cerr << "Command failed (" << rc << "): " << cmd << "\n";
  }
  return rc;
}

std::vector<std::string> Glob(const std::string& pattern) {
  glob_t glob_result;
  std::vector<std::string> paths;
  if (glob(pattern.c_str(), 0, nullptr, &glob_result) == 0) {
    for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
      paths.emplace_back(glob_result.gl_pathv[i]);
    }
  }
  globfree(&glob_result);
  return paths;
}

bool ParseKnapsackFile(
    const std::string& filepath,
    std::vector<int>* weights,
    std::vector<int>* values,
    int* capacity,
    std::vector<std::pair<int, int>>* conflicts) {
  std::ifstream infile(filepath);
  if (!infile) {
    return false;
  }

  std::string line;
  std::string state;
  int num_items = 0;

  while (std::getline(infile, line)) {
    line = Trim(line);
    if (line.empty() || StartsWith(line, "#")) {
      continue;
    }
    if (StartsWith(line, "param n :=")) {
      const auto pos = line.find(":=");
      std::string cleaned = line.substr(pos + 2);
      cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ';'), cleaned.end());
      num_items = std::stoi(Trim(cleaned));
    } else if (StartsWith(line, "param c :=")) {
      const auto pos = line.find(":=");
      std::string cleaned = line.substr(pos + 2);
      cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ';'), cleaned.end());
      *capacity = std::stoi(Trim(cleaned));
    } else if (StartsWith(line, "param : V : p w :=")) {
      state = "items";
      continue;
    } else if (StartsWith(line, "set E :=")) {
      state = "conflicts";
      continue;
    } else if (line == ";") {
      state.clear();
      continue;
    }

    if (state == "items") {
      std::istringstream iss(line);
      int idx = 0;
      int value = 0;
      int weight = 0;
      if (iss >> idx >> value >> weight) {
        values->push_back(value);
        weights->push_back(weight);
      }
    } else if (state == "conflicts") {
      std::istringstream iss(line);
      int i = 0;
      int j = 0;
      if (iss >> i >> j) {
        conflicts->emplace_back(i, j);
      }
    }
  }

  if (num_items != 0 && static_cast<int>(values->size()) != num_items) {
    std::cerr << "Warning: Parsed " << values->size()
              << " items, but n states " << num_items << ".\n";
  }
  return true;
}

std::optional<Result> SolveKnapsackWithConflicts(
    const std::vector<int>& weights,
    const std::vector<int>& values,
    int capacity,
    const std::vector<std::pair<int, int>>& conflicts) {
  using operations_research::sat::BoolVar;
  using operations_research::sat::CpModelBuilder;
  using operations_research::sat::CpSolverResponse;
  using operations_research::sat::CpSolverStatus;
  using operations_research::sat::LinearExpr;
  using operations_research::sat::Model;
  using operations_research::sat::SatParameters;
  using operations_research::sat::SolutionIntegerValue;
  using operations_research::sat::SolveCpModel;
  using operations_research::sat::NewSatParameters;

  const int n = static_cast<int>(weights.size());
  if (values.size() != weights.size()) {
    return std::nullopt;
  }

  CpModelBuilder cp_model;
  std::vector<BoolVar> x;
  x.reserve(n);
  for (int i = 0; i < n; ++i) {
    x.push_back(cp_model.NewBoolVar());
  }

  LinearExpr weight_expr;
  for (int i = 0; i < n; ++i) {
    weight_expr += weights[i] * x[i];
  }
  cp_model.AddLessOrEqual(weight_expr, capacity);

  for (const auto& conflict : conflicts) {
    const int i = conflict.first;
    const int j = conflict.second;
    if (i < 0 || j < 0 || i >= n || j >= n) {
      continue;
    }
    if (i == j) {
      cp_model.AddEquality(x[i], 0);
    } else {
      cp_model.AddLessOrEqual(x[i] + x[j], 1);
    }
  }

  LinearExpr objective;
  for (int i = 0; i < n; ++i) {
    objective += values[i] * x[i];
  }
  cp_model.Maximize(objective);

  Model model;
  SatParameters params;
  params.set_max_time_in_seconds(30.0);
  model.Add(NewSatParameters(params));

  const CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);
  if (response.status() != CpSolverStatus::OPTIMAL &&
      response.status() != CpSolverStatus::FEASIBLE) {
    return std::nullopt;
  }

  Result result;
  for (int i = 0; i < n; ++i) {
    if (SolutionIntegerValue(response, x[i]) == 1) {
      result.picked.push_back(i);
      result.total_weight += weights[i];
      result.total_value += values[i];
    }
  }
  return result;
}

}  // namespace

int main(int argc, char** argv) {
  bool no_download = false;
  bool no_install = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--no-download") {
      no_download = true;
    } else if (arg == "--no-install") {
      no_install = true;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: " << argv[0]
                << " [--no-download] [--no-install]\n";
      return 0;
    }
  }

  if (no_install) {
    std::cout << "--no-install ignored in C++ port; ensure OR-Tools is installed.\n";
  }

  const std::vector<std::string> dropbox_urls = {
      "https://www.dropbox.com/scl/fi/y739yds3givrzuoao1b0u/C1.zip?rlkey=z22clyzvalloof4335cdosbea&dl=0",
      "https://www.dropbox.com/scl/fi/geoymn3ndrmp2rbkrrv6o/C3.zip?rlkey=bkee32fj67mlgg664vfm1jrlx&dl=0",
      "https://www.dropbox.com/scl/fi/254dl7d1vqh7o3fj5zxpg/C10.zip?rlkey=2ugfkzuo7tzro0kb1whplammv&dl=0",
      "https://www.dropbox.com/scl/fi/8v8d5mhf01vbvs29n9bh4/R3.zip?rlkey=9haa9ryoykfno81jy3u15q438&dl=0",
      "https://www.dropbox.com/scl/fi/57790j528scwlngdfiz1q/R10.zip?rlkey=owiwvl25j6h03qoi0i54705gf&dl=0",
      "https://www.dropbox.com/scl/fi/irs32pobjzxs9t6arym8o/sparse_corr.zip?rlkey=fsl6y7p2z2asg5ugc8e152xl2&dl=0",
      "https://www.dropbox.com/scl/fi/f9sznsnp78g5lgcn0ws77/sparse_rand.zip?rlkey=nh7bjqwvh7etd3v4y6jtcrrf3&dl=0"};

  if (!no_download) {
    for (const auto& url : dropbox_urls) {
      const std::string direct = MakeDirectDownloadUrl(url);
      const std::string filename = FilenameFromUrl(direct);
      std::cout << "Downloading " << filename << "...\n";
      const std::string cmd = "curl -L -o '" + filename + "' '" + direct + "'";
      RunCommand(cmd);
      std::cout << "Download complete.\n";
    }

    std::cout << "All specified zip files have been downloaded.\n";

    for (const auto& url : dropbox_urls) {
      const std::string filename = FilenameFromUrl(url);
      std::cout << "Unzipping " << filename << "...\n";
      const std::string cmd = "unzip -o '" + filename + "'";
      RunCommand(cmd);
      std::cout << "Finished unzipping " << filename << ".\n";
    }

    std::cout << "All zip files have been unzipped.\n";
  }

  const std::vector<std::string> instance_file_patterns = {
      "C1/BPPC_*.txt_*",
      "C3/BPPC_*.txt_*",
      "C10/BPPC_*.txt_*",
      "R3/BPPC_*.txt_*",
      "R10/BPPC_*.txt_*",
      "sparse_corr/test_*.dat",
      "sparse_rand/test_*.dat"};

  std::vector<std::string> instance_files;
  for (const auto& pattern : instance_file_patterns) {
    auto matches = Glob(pattern);
    instance_files.insert(instance_files.end(), matches.begin(), matches.end());
  }
  std::sort(instance_files.begin(), instance_files.end());

  std::cout << "Found " << instance_files.size() << " knapsack instance files.\n\n";

  std::vector<std::pair<std::string, std::string>> results_summary;
  for (const auto& filepath : instance_files) {
    std::cout << "Solving instance: " << filepath << "\n";
    auto start = std::chrono::steady_clock::now();

    std::vector<int> weights;
    std::vector<int> values;
    std::vector<std::pair<int, int>> conflicts;
    int capacity = 0;

    if (!ParseKnapsackFile(filepath, &weights, &values, &capacity, &conflicts)) {
      std::cout << "  Error processing " << filepath << ": unable to read file.\n";
      results_summary.emplace_back(filepath, "Error: unable to read file");
      continue;
    }

    auto result = SolveKnapsackWithConflicts(weights, values, capacity, conflicts);
    auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double> duration = end - start;

    if (result.has_value()) {
      Result res = result.value();
      res.time_taken = duration.count();
      std::cout << "  Picked items: ";
      for (size_t i = 0; i < res.picked.size(); ++i) {
        std::cout << res.picked[i] << (i + 1 == res.picked.size() ? "" : ", ");
      }
      std::cout << "\n";
      std::cout << "  Total weight: " << res.total_weight << "\n";
      std::cout << "  Total value: " << res.total_value << "\n";
      std::cout << "  Time taken: " << res.time_taken << " seconds\n";
      std::ostringstream summary;
      summary << "Value=" << res.total_value
              << ", Weight=" << res.total_weight
              << ", Picked=" << res.picked.size()
              << ", Time Taken=" << res.time_taken << "s";
      results_summary.emplace_back(filepath, summary.str());
    } else {
      std::cout << "  No feasible solution found or solver timed out.\n";
      std::cout << "  Time taken: " << duration.count() << " seconds\n";
      results_summary.emplace_back(filepath, "No solution");
    }
  }

  std::cout << "\n--- Summary of all instances ---\n";
  for (const auto& entry : results_summary) {
    std::cout << entry.first << ": " << entry.second << "\n";
  }

  return 0;
}
