#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// Observer interface
class EventListener {
public:
  virtual void update(const std::string &event, const std::string &data) = 0;
  virtual ~EventListener() = default;
};

// Subject — giữ con trỏ không sở hữu; listener phải sống lâu hơn emitter
class EventEmitter {
  std::unordered_map<std::string, std::vector<EventListener *>> listeners_;

public:
  void on(const std::string &event, EventListener *listener) {
    if (!listener)
      return;
    listeners_[event].push_back(listener);
  }

  void off(const std::string &event, EventListener *listener) {
    auto it = listeners_.find(event);
    if (it == listeners_.end())
      return;
    auto &lst = it->second;
    lst.erase(std::remove(lst.begin(), lst.end(), listener), lst.end());
  }

  void emit(const std::string &event, const std::string &data = "") {
    auto it = listeners_.find(event);
    if (it == listeners_.end())
      return;
    for (auto *l : it->second)
      l->update(event, data);
  }
};

// Concrete Observers
class Logger : public EventListener {
public:
  void update(const std::string &evt, const std::string &data) override {
    std::cout << "[LOG] " << evt << ": " << data << "\n";
  }
};

class EmailAlert : public EventListener {
  std::string email_;

public:
  explicit EmailAlert(const std::string &e) : email_(e) {}
  void update(const std::string &evt, const std::string &data) override {
    std::cout << "[EMAIL->" << email_ << "] " << evt << " - " << data << "\n";
  }
};

int main() {
  EventEmitter store;
  Logger logger;
  EmailAlert admin("admin@example.com");

  store.on("purchase", &logger);
  store.on("purchase", &admin);
  store.on("refund", &logger);

  store.emit("purchase", "Item #42 by user John");
  store.emit("refund", "Order #7 refunded");

  store.off("purchase", &admin);      // unsubscribe
  store.emit("purchase", "Item #99"); // only logger notified
}
