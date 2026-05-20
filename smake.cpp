#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <sys/wait.h>
#include <assert.h>

namespace fs = std::filesystem;

enum {NO, OUTPUT, ALL} SILENT = NO;

typedef enum {STALE, NOT_STALE} file_status;

class Rules {
private:
  std::string name;
  std::vector<std::string> dependence;
  std::vector<std::string> commands;
  std::string info;
  fs::path file;
  fs::file_time_type last_modify_time;
  bool checked = false;
  bool running = false;
  static std::vector<Rules*> rules;
  static std::unordered_map<std::string, Rules*> rules_table;

  void set_command(std::string cmds) {
    commands.push_back(cmds);
  }

  void set_dependence(std::string dep) {
    size_t s_pos = 0;
    size_t e_pos = dep.find(' ', s_pos);
    while (e_pos != std::string::npos) {
      this->dependence.push_back(dep.substr(s_pos, e_pos-s_pos));
      s_pos = e_pos + 1;
      e_pos = dep.find(' ', s_pos);
    }
    this->dependence.push_back(dep.substr(s_pos, -1));
  }

  void set_file(fs::path f) {
    file = f;
  }

  void set_time(fs::file_time_type t) {
    last_modify_time = t;
  }

  void set_info(std::string _info) {
    info = _info;
  }

  void run_commands() {
    for (size_t i = 0; i < commands.size(); i++) {
      if (SILENT != ALL) {
        std::cout << "[CMD] " << commands[i] << std::endl;
      }
      FILE* pipe = popen(commands[i].c_str(), "r");
      if (pipe == NULL) {
	clear();
	perror("popen failed");
      }
      std::string output;
      char buf[1024];
      while (fgets(buf, sizeof(buf), pipe)) {
	output += buf;
      }
      int res = pclose(pipe);
      int res_code = WEXITSTATUS(res);
      if (res_code != 0) {
        if (SILENT == ALL) {
          std::cerr << "[ERROR] " << commands[i] << std::endl;
        }
	clear();
	exit(res_code);
      }
      if (SILENT == NO) {
	std::cout << output;
      }
    }
    if (!info.empty()) std::cout << "[INFO] " << info << std::endl;
  }

  static bool check_file_exist(std::string target_name) {
    for (const auto& entry : fs::directory_iterator("./")) {
      fs::path file_name = entry.path().filename().string();
      if (file_name == target_name) return true;
    }
    return false;
  }

  static bool check_rule_exist(std::string target_rule) {
    if (rules_table.count(target_rule) != 0) return true;
    else return false;
  }

  static file_status check_time(std::string target, std::string relay) {
    Rules* target_rule = rules_table[target];
    if (target_rule->file.empty()) return STALE;
    if (target_rule->last_modify_time < fs::last_write_time(relay)) {
      return STALE;
    } else {
      return NOT_STALE;
    }
  }

  static void regist_file() {
    for (const auto& entry : fs::directory_iterator("./")) {
      fs::path file_name = entry.path().filename();
      if ((rules_table.count(file_name.string())) != 0) {
	if (rules_table[file_name.string()]->file.empty()) {
	  rules_table[file_name.string()]->set_file(file_name);
	} else {
	  ;;
	}
	rules_table[file_name.string()]->set_time(fs::last_write_time(file_name));
      }
    }
  }

  static bool check_flag(std::string flag) {
    if (flag == "#?" || flag == "#?OUTPUT") {
      SILENT = OUTPUT;
    }
    else if (flag == "#?ALL") {
      SILENT = ALL;
    }
    else {
      return false;
    }
    return true;
  }
  
public:
  Rules(std::string rule_name): name(rule_name) {
    rules.push_back(this);
    rules_table[name] = this;
  }

  static void load(void) {
    std::ifstream makefile("Makefile");
    if (!makefile) {
      std::cerr << "Error: Must prepare a Makefile." << std::endl;
      exit(1);
    }
    std::string line;
    getline(makefile, line);
    if (!check_flag(line)) makefile.seekg(0, std::ios::beg);
    while (getline(makefile, line)) {
      if (line[0] == '\t') {
	if (rules.empty()) {
	  std::cerr << "Must set a rule first." << std::endl;
	  exit(1);
	}
	rules.back()->set_command(line.substr(1, -1));
      }
      else if (line[0] == '\0' || line[0] == '#') continue;
      else {
	size_t pos = line.find(':');
	if (pos != std::string::npos) {
	  Rules* new_rule = new Rules(line.substr(0, pos));
	  size_t pos2 = line.find(':', pos+1);
	  if (pos2 != std::string::npos) {
	    new_rule->set_dependence(line.substr(pos+1, pos2-pos-1));
	    new_rule->set_info(line.substr(pos2+1, -1));
	  } else {
	    new_rule->set_dependence(line.substr(pos+1, -1));
	  }
	} else {
	  std::cerr << "Lack split symbol." << std::endl;
	  clear();
	  exit(1);
	}
	if (rules.size() == 1) {
	  Rules* head_rule = new Rules("_");
	  head_rule->set_dependence(rules[0]->name);
	  rules.insert(rules.begin(), head_rule);
	  rules.pop_back();
	}
      }
    }
    makefile.close();
    regist_file();
  }

  static void run_rule(std::string target) {
    if (rules_table.count(target) == 0) {
      std::cerr << "Error: " << target << " do not exist." << std::endl;
      clear();
      exit(1);
    }
    Rules* target_rule = rules_table[target];
    if (target_rule->checked) return;
    else target_rule->checked = true;
    // NOTE: the `load` makes every rule has a dependence, even it's `""`, so use this instead of empty().
    if (target_rule->dependence[0] == "") {
      target_rule->run_commands();
      return;
    }
    for (size_t i = 0; i < target_rule->dependence.size(); i++) {
      std::string relay = target_rule->dependence[i];
      if (check_rule_exist(relay)) {
	run_rule(relay);
	if (check_file_exist(relay)) {
	  if (check_time(target, relay) == STALE) {
	    target_rule->running = true;
	  } else {
	    continue;
	  }
	} else {
	  target_rule->running = true;
	}
      } else {
	if (check_file_exist(relay)) {
	  if (check_time(target, relay) == STALE) {
	    target_rule->running = true;
	  } else {
	    continue;
	  }
	} else {
	  std::cerr << "Error: " << relay << " do not exist." << std::endl;
	  clear();
	  exit(1);
	}
      }
    }
    if (target_rule->running) target_rule->run_commands();
  }

  static void run(void) {
    run_rule("_");
    if (rules[1]->running == false) {
      std::cout << "All files are lastest." << std::endl;
    }
  }

  static void dump(void) {
    for (size_t j = 0; j < rules.size(); j++) {
      std::cout << "[RULE] " << rules[j]->name << std::endl;
      printf("[DEPENDENCE] ");
      for (size_t i = 0; i < rules[j]->dependence.size(); i++) {
	std::cout << rules[j]->dependence[i] << " ";
      }
      puts("");
      printf("[COMMANDS] ");
      for (size_t i = 0; i < rules[j]->commands.size(); i++) {
	std::cout << rules[j]->commands[i] << std::endl;
      }
      std::cout << "[INFO] " << rules[j]->info <<std::endl;
      std::cout << "[FILE] " << rules[j]->file.string() << std::endl;
      puts("");
    }
  }

  static void clear(void) {
    for (size_t i = 0; i < rules.size(); i++) {
      delete rules[i];
    }
  }
};
std::vector<Rules*> Rules::rules;
std::unordered_map<std::string, Rules*> Rules::rules_table;

int main(int args, char* argv[]) {
  Rules::load();
  if (args > 1) {
    for (int i = 1; i < args; i++) {
      Rules::run_rule(argv[i]);
    }
  } else {
    Rules::run();
  }
  // puts("");
  // Rules::dump();
  Rules::clear();
  return 0;
}
