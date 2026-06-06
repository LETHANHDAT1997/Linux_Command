// Existing incompatible class (Adaptee)
class XmlLogger {
public:
  void logXml(const std::string &xml) {
    std::cout << "<log>" << xml << "</log>\n";
  }
};

// Target interface expected by client
class JsonLogger {
public:
  virtual void logJson(const std::string &json) = 0;
  virtual ~JsonLogger() = default;
};

// Object Adapter (composition)
class LoggerAdapter : public JsonLogger {
  XmlLogger adaptee_;

public:
  void logJson(const std::string &json) override {
    std::string xml = "data:" + json; // simplified conversion
    adaptee_.logXml(xml);
  }
};

// Class Adapter (multiple inheritance)
class LoggerAdapter2 : public JsonLogger, private XmlLogger {
public:
  void logJson(const std::string &json) override {
    logXml("data:" + json); // direct access via inheritance
  }
};

// Client only knows JsonLogger
void clientCode(JsonLogger &logger) {
  logger.logJson("{\"level\":\"INFO\",\"msg\":\"hello\"}");
}

int main() {
  LoggerAdapter adapter;
  clientCode(adapter);
  // Output: <log>data:{"level":"INFO","msg":"hello"}</log>
}