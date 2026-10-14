// Small in-budget controller for the headless campaign diagnostic. Hashing,
// child execution, timeout handling and result validation stay in the cgroup.
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
namespace {
namespace fs = std::filesystem;
using Json = nlohmann::json;
void require_memory_budget() {
  const auto *marker = std::getenv("HALO2_TAG_MATCH_MEMORY_UNIT");
  std::ifstream source("/proc/self/cgroup");
  std::string group;
  std::getline(source, group);
  if (!marker || std::string(marker) != "halo2-tag-match-validation.service" ||
      !group.starts_with("0::") ||
      fs::path(group.substr(3)).filename() != marker)
    throw std::runtime_error(
        "Campaign controller requires the verified 64 MiB memory unit");
  const auto directory =
      fs::path("/sys/fs/cgroup") / fs::path(group.substr(3)).relative_path();
  for (const auto &[name, expected] :
       std::array<std::pair<const char *, const char *>, 4>{
           {{"memory.max", "62914560"},
            {"memory.high", "max"},
            {"memory.swap.max", "0"},
            {"memory.oom.group", "1"}}}) {
    std::ifstream file(directory / name);
    std::string value;
    file >> value;
    if (value != expected)
      throw std::runtime_error("Campaign controller memory controls differ");
  }
}
std::string digest(const fs::path &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("Cannot hash campaign input: " + path.string());
  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context(
      EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!context || EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1)
    throw std::runtime_error("Cannot initialize campaign input hash");
  std::array<char, 65536> bytes;
  while (file) {
    file.read(bytes.data(), bytes.size());
    if (file.gcount() && EVP_DigestUpdate(context.get(), bytes.data(),
                                          std::size_t(file.gcount())) != 1)
      throw std::runtime_error("Cannot hash campaign input");
  }
  if (!file.eof())
    throw std::runtime_error("Campaign input read failed: " + path.string());
  std::array<unsigned char, EVP_MAX_MD_SIZE> hash{};
  unsigned size{};
  if (EVP_DigestFinal_ex(context.get(), hash.data(), &size) != 1)
    throw std::runtime_error("Cannot finish campaign input hash");
  std::ostringstream text;
  for (unsigned i = 0; i < size; ++i)
    text << std::hex << std::setw(2) << std::setfill('0') << unsigned(hash[i]);
  return text.str();
}
int execute(const std::vector<std::string> &command, const fs::path &log,
            double timeout, bool &timed_out) {
  const int output = ::open(log.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
  if (output < 0)
    throw std::runtime_error("Cannot create fresh campaign log");
  std::vector<char *> arguments;
  for (const auto &word : command)
    arguments.push_back(const_cast<char *>(word.c_str()));
  arguments.push_back(nullptr);
  const auto child = ::fork();
  if (child < 0) {
    ::close(output);
    throw std::runtime_error("Cannot start campaign diagnostic");
  }
  if (child == 0) {
    ::setpgid(0, 0);
    if (::dup2(output, STDOUT_FILENO) < 0 || ::dup2(output, STDERR_FILENO) < 0)
      _exit(126);
    ::close(output);
    ::execv(arguments.front(), arguments.data());
    _exit(127);
  }
  ::close(output);
  ::setpgid(child, child);
  const auto end =
      std::chrono::steady_clock::now() + std::chrono::duration<double>(timeout);
  int status{};
  for (;;) {
    const auto waited = ::waitpid(child, &status, WNOHANG);
    if (waited == child)
      break;
    if (waited < 0) {
      if (errno == EINTR)
        continue;
      ::kill(-child, SIGKILL);
      ::waitpid(child, &status, 0);
      throw std::runtime_error("Cannot wait for campaign diagnostic");
    }
    if (std::chrono::steady_clock::now() >= end) {
      timed_out = true;
      ::kill(-child, SIGKILL);
      while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {
      }
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  return WIFSIGNALED(status) ? -WTERMSIG(status) : -1;
}
} // namespace
int main(int argc, char **argv) {
  try {
    require_memory_budget();
    if (argc < 7)
      throw std::runtime_error("Supply NATIVE MAP SHARED XBE TICKS OUTPUT "
                               "[campaign options] [--timeout=SECONDS]");
    const auto output = fs::absolute(argv[6]);
    if (fs::exists(output / "report.json"))
      throw std::runtime_error("Refusing to overwrite campaign evidence");
    fs::create_directories(output);
    std::vector<std::string> command;
    for (int i = 1; i <= 6; ++i)
      command.emplace_back(i == 5 ? argv[i] : fs::absolute(argv[i]).string());
    double timeout = 120;
    bool require_completion = false, allow_skip = true;
    for (int i = 7; i < argc; ++i) {
      const std::string option = argv[i];
      if (option.starts_with("--timeout=")) {
        std::size_t used{};
        timeout = std::stod(option.substr(10), &used);
        if (used != option.size() - 10 || !(timeout >= 10 && timeout <= 600))
          throw std::runtime_error("Invalid campaign timeout");
      } else {
        command.push_back(option);
        require_completion |= option == "--require-completion";
        if (option == "--no-skip")
          allow_skip = false;
      }
    }
    std::map<std::string, fs::path> inputs{
        {"native", command[0]}, {"map", command[1]}, {"xbe", command[3]}};
    std::map<std::string, std::string> hashes;
    std::vector<std::string> errors, faults;
    Json measurements = Json::object();
    Json native_status = nullptr;
    int status = -1;
    const auto started = std::chrono::steady_clock::now();
    try {
      for (const auto &[name, path] : inputs)
        hashes[name] = digest(path);
      bool timed_out = false;
      status = execute(command, output / "campaign.log", timeout, timed_out);
      if (timed_out)
        errors.emplace_back("Campaign simulation exceeded its timeout");
      std::ifstream log(output / "campaign.log", std::ios::binary);
      std::ostringstream contents;
      contents << log.rdbuf();
      const auto text = contents.str();
      std::smatch match;
      if (std::regex_search(
              text, match,
              std::regex("PASS bounded campaign simulation: ([^\\r\\n]*)"))) {
        const auto line = match[1].str();
        native_status = line;
        const std::regex metric("(?:^|\\s)(\\w+)=(\\d+)(?=\\s|$)");
        for (auto it = std::sregex_iterator(line.begin(), line.end(), metric);
             it != std::sregex_iterator(); ++it)
          measurements[(*it)[1].str()] = std::stoull((*it)[2].str());
      }
      std::istringstream lines(text);
      for (std::string line; std::getline(lines, line);)
        if (line.starts_with("Script fault: "))
          faults.push_back(line);
      if (status != 0)
        errors.emplace_back("Campaign simulation exited with status " +
                            std::to_string(status));
      if (native_status.is_null())
        errors.emplace_back("Strict campaign simulation checks did not pass");
      if (require_completion && measurements.value("level_finished", 0) != 1)
        errors.emplace_back("Authored level completion was not reached");
      for (const auto &[name, path] : inputs)
        if (digest(path) != hashes.at(name))
          errors.emplace_back("A campaign input changed during the run");
    } catch (const std::exception &error) {
      errors.push_back(error.what());
    }
    const auto seconds = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - started)
                             .count();
    Json report{
        {"passed", errors.empty()},
        {"scope", "Serial headless campaign simulation; all native controller "
                  "and child allocations share the guarded 64 MiB job"},
        {"command", command},
        {"input_sha256", hashes},
        {"measurements", measurements},
        {"errors", errors},
        {"native_status", native_status},
        {"child_exit_status", status},
        {"script_faults", faults},
        {"elapsed_seconds", seconds},
        {"completion_required", require_completion},
        {"cinematic_skip_allowed", allow_skip},
        {"native_validation_runner", true},
        {"rendering_tested", false},
        {"audio_playback_tested", false},
        {"original_gameplay_parity_tested", false}};
    // Original legacy print bytes remain exact in campaign.log. JSON replaces
    // invalid UTF-8 only in diagnostics, as the Python controller does.
    const auto encoded =
        report.dump(2, ' ', false, Json::error_handler_t::replace);
    std::ofstream file(output / "report.json");
    file << encoded << '\n';
    file.close();
    if (!file)
      throw std::runtime_error("Cannot save campaign validation report");
    std::cout << encoded << '\n';
    return errors.empty() ? 0 : 1;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
