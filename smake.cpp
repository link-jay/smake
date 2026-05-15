#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <stack>
#include <cstdlib>
#include <sys/wait.h>
#include <assert.h>

class Rules {
private:
  std::string name;
  std::stack<std::string> dependence;
  std::stack<std::string> commands;
  std::string info;
  
public:
  static std::stack<Rules*> rules;
  static std::unordered_map<std::string, Rules*> rules_table;
  
  Rules(std::string rule_name): name(rule_name) {
    if (rules.empty()) rules_table["head"] = this;
    rules.push(this);
    rules_table[name] = this;
  }

  void set_command(std::string cmds) {
    commands.push(cmds);
  }
  
  void set_dependence(std::string dep) {
    size_t s_pos = 0;
    size_t e_pos = dep.find(' ', s_pos);
     while (e_pos != std::string::npos) {
      this->dependence.push(dep.substr(s_pos, e_pos));
      s_pos = e_pos + 1;
      e_pos = dep.find(' ', s_pos);
    }
    this->dependence.push(dep.substr(s_pos, -1));
  }
  
  void set_info(std::string _info) {
    info = _info;
  }
  
  static void load(void) {
    std::ifstream fin("Makefile");
    std::string line;
    while (getline(fin, line)) {
      if (line[0] == '\t') {
	if (rules.empty()) assert(0);
	rules.top()->set_command(line.substr(1, -1));
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
    fin.close();
  }  
  
  static void run_rule(std::string target) {
    if (!Rules::rules_table.count(target)) return;
    Rules* head = Rules::rules_table[target];
    std::stack<std::string> tmp2, tmp = head->dependence;
    while (!tmp.empty()) {
      run_rule(tmp.top());
      tmp.pop();
    }
    tmp = std::stack<std::string>();
    tmp2 = head->commands;
    while (!tmp2.empty()) {
      tmp.push(tmp2.top());
      tmp2.pop();
    }
    while (!tmp.empty()) {
      std::cout << "[CMD] " << tmp.top() << std::endl;
      size_t res = system(tmp.top().c_str());
      tmp.pop();
      size_t res_code = WEXITSTATUS(res);
      if (res_code != 0) {
	Rules::free();
	exit(res_code);
      }
    }
  }
  
  static void run(void) {
    run_rule("head");
  }
  
  static void dump(void) {
    std::stack<Rules*> ttmp = rules;
    std::stack<Rules*> tmp;
    while (!ttmp.empty()) {
      tmp.push(ttmp.top());
      ttmp.pop();
    }
    while (!tmp.empty()) {
      Rules* it = tmp.top();
      std::cout << "[RULE] " << it->name << std::endl;
      printf("[DEPENDENCE] ");
      std::stack<std::string> tmp1, tmp2 = it->dependence;
      while (!tmp2.empty()) {
	tmp1.push(tmp2.top());
	tmp2.pop();
      }
      while (!tmp1.empty()) {
	std::cout << tmp1.top() << " ";
	tmp1.pop();
      }
      puts("");
      printf("[COMMANDS] ");
      tmp1 = std::stack<std::string>();
      tmp2 = it->commands;
      while (!tmp2.empty()) {
	tmp1.push(tmp2.top());
	tmp2.pop();
      }
      while (!tmp1.empty()) {
	std::cout << tmp1.top() << " ";
	tmp1.pop();
      }
      puts("");
      std::cout << "[INFO] " << it->info <<std::endl;
      puts("");
      tmp.pop();
    }
  }
  
  static void free(void) {
    while (!Rules::rules.empty()) {
      delete rules.top();
      rules.pop();
    }
  }
  
};
std::stack<Rules*> Rules::rules;
std::unordered_map<std::string, Rules*> Rules::rules_table;

int main() {
  Rules::load();
  Rules::run();
  // Rules::dump();
  Rules::free();
  return 0;
}
