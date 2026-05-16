#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <sys/wait.h>
#include <assert.h>

class Rules {
private:
  std::string name;
  std::vector<std::string> dependence;
  std::vector<std::string> commands;
  std::string info;
  
  void set_command(std::string cmds) {
    commands.push_back(cmds);
  }
  
  void set_dependence(std::string dep) {
    size_t s_pos = 0;
    size_t e_pos = dep.find(' ', s_pos);
     while (e_pos != std::string::npos) {
      this->dependence.push_back(dep.substr(s_pos, e_pos));
      s_pos = e_pos + 1;
      e_pos = dep.find(' ', s_pos);
    }
    this->dependence.push_back(dep.substr(s_pos, -1));
  }
  
  void set_info(std::string _info) {
    info = _info;
  }
  
public:
  static std::vector<Rules*> rules;
  static std::unordered_map<std::string, Rules*> rules_table;
  
  Rules(std::string rule_name): name(rule_name) {
    if (rules.empty()) rules_table["head"] = this;
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
      }
    }
    makefile.close();
  }  
  
  static void run_rule(std::string target) {
    if (!Rules::rules_table.count(target)) return;
    Rules* head = Rules::rules_table[target];
    for (size_t i = 0; i < head->dependence.size(); i++) {
      run_rule(head->dependence[i]);
    }
    for (size_t i = 0; i < head->commands.size(); i++) {
      std::cout << "[CMD] " << head->commands[i] << std::endl;
      size_t res = system(head->commands[i].c_str());
      size_t res_code = WEXITSTATUS(res);
      if (res_code != 0) {
	Rules::free();
	exit(res_code);
      }
    }
    if (head->info != "") std::cout << "[INFO] " << head->info << std::endl;
  }
  
  static void run(void) {
    run_rule("head");
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
      for (size_t i = 0; i < Rules::rules[i]->commands.size(); i++) {
	std::cout << Rules::rules[j]->commands[i] << std::endl;
      }
      std::cout << "[INFO] " << Rules::rules[j]->info <<std::endl;
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
