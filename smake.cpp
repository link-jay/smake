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

typedef enum {NO_FILE, STALE, NOT_STALE} file_status;

class Rules {
private:
  std::string name;
  std::vector<std::string> dependence;
  std::vector<std::string> commands;
  std::string info;
  fs::path file;
  fs::file_time_type last_modify_time;

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

  void run_command() {
    for (size_t i = 0; i < commands.size(); i++) {
      std::cout << "[CMD] " << commands[i] << std::endl;
      int res = system(commands[i].c_str());
      int res_code = WEXITSTATUS(res);
      if (res_code != 0) {
	Rules::free();
	exit(res_code);
      }
    }
    if (!info.empty()) std::cout << "[INFO] " << info << std::endl;
  }

public:
  static std::vector<Rules*> rules;
  static std::unordered_map<std::string, Rules*> rules_table;

  Rules(std::string rule_name): name(rule_name) {
    rules.push_back(this);
    rules_table[name] = this;
  }

  static bool check_file_exsit(std::string target_name) {
    for (const auto& entry : fs::directory_iterator("./")) {
      fs::path file_name = entry.path().filename().string();
      if (file_name == target_name) return true;
    }
    return false;
  }

  static bool check_rule_exsit(std::string target_rule) {
    if (Rules::rules_table.count(target_rule) != 0) return true;
    else return false;
  }

  static file_status check_time(std::string target, std::string relay) {
    Rules* target_rule = Rules::rules_table[target];
    if (target_rule->file.empty()) return NO_FILE;
    if (target_rule->last_modify_time < fs::last_write_time(relay)) {
      return STALE;
    } else {
      return NO_STALE;
    }
  }

  static void regist_file() {
    for (const auto& entry : fs::directory_iterator("./")) {
      fs::path file_name = entry.path().filename();
      if ((Rules::rules_table.count(file_name.string())) != 0) {
	if (Rules::rules_table[file_name.string()]->file.empty()) {
	  Rules::rules_table[file_name.string()]->set_file(file_name);
	} else {
	  ;;
	}
	Rules::rules_table[file_name.string()]->set_time(fs::last_write_time(file_name));
      }
    }
  }

  static void load(void) {
    std::ifstream makefile("Makefile");
    if (!makefile) {
      std::cerr << "Error: Must prepare a Makefile." << std::endl;
      exit(1);
    }
    std::string line;
    while (getline(makefile, line)) {
      if (line[0] == '\t') {
	if (rules.empty()) assert(0);
	rules.back()->set_command(line.substr(1, -1));
      }
      else if (line[0] == '\0') continue;
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
	  assert(0);
	}
	if (Rules::rules.size() == 1) {
	  Rules* head_rule = new Rules("_");
	  head_rule->set_dependence(Rules::rules[0]->name);
	  Rules::rules.insert(Rules::rules.begin(), head_rule);
	  Rules::rules.pop_back();
	}
      }
    }
    makefile.close();
    regist_file();
  }

  static void run_rule(std::string target) {
    if (Rules::rules_table.count(target) == 0) {
      std::cerr << "Error: " << target << " do not exsit." << std::endl;
      Rules::free();
      exit(1);
    }
    Rules* target_rule = Rules::rules_table[target];
    if (target_rule->dependence[0] == "") {
      target_rule->run_command();
      return;
    }
    for (size_t i = 0; i < target_rule->dependence.size(); i++) {
      std::string relay = target_rule->dependence[i];
      if (Rules::check_file_exsit(relay)) {
	if (Rules::check_time(target, relay) == STALE) {
	  target_rule->run_command();
	}
	else if (Rules::check_time(target, relay) == NO_FILE) {
	  if (check_rule_exsit(relay)) {
	    run_rule(relay);
	    target_rule->run_command();
	  }
	  else {
	    continue;
	  }
	}
	else {
	  if (Rules::check_rule_exsit(relay)) {
	    run_rule(relay);
	    if (Rules::check_time(target, relay) == STALE) {
	      target_rule->run_command();
	    } else {
	      continue;
	    }
	  } else {
	    continue;
	  }
	}
      } else {
	if (!Rules::check_rule_exsit(relay)) {
	  std::cerr << "Error: " << relay << " do not exsit." << std::endl;
	  Rules::free();
	  exit(1);
	}
	run_rule(relay);
	target_rule->run_command();
      }
    }
  }

  static void run(void) {
    run_rule("_");
  }

  static void dump(void) {
    for (size_t j = 0; j < Rules::rules.size(); j++) {
      std::cout << "[RULE] " << Rules::rules[j]->name << std::endl;
      printf("[DEPENDENCE] ");
      for (size_t i = 0; i < Rules::rules[j]->dependence.size(); i++) {
	std::cout << Rules::rules[j]->dependence[i] << " ";
      }
      puts("");
      printf("[COMMANDS] ");
      for (size_t i = 0; i < Rules::rules[j]->commands.size(); i++) {
	std::cout << Rules::rules[j]->commands[i] << std::endl;
      }
      std::cout << "[INFO] " << Rules::rules[j]->info <<std::endl;
      std::cout << "[FILE] " << Rules::rules[j]->file.string() << std::endl;
      puts("");
    }
  }

  static void free(void) {
    for (size_t i = 0; i < Rules::rules.size(); i++) {
      delete rules[i];
    }
  }

};
std::vector<Rules*> Rules::rules;
std::unordered_map<std::string, Rules*> Rules::rules_table;

int main() {
  Rules::load();
  Rules::run();
  puts("");
  Rules::dump();
  Rules::free();
  return 0;
}
