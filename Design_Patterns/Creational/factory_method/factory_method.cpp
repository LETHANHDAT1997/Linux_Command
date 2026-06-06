// Product interface
class Button {
public:
  virtual void render() = 0;
  virtual void onClick() = 0;
  virtual ~Button() = default;
};

// Concrete Products
class WindowsButton : public Button {
public:
  void render() override { std::cout << "Rendering Windows button\n"; }
  void onClick() override { std::cout << "Windows click\n"; }
};

class MacButton : public Button {
public:
  void render() override { std::cout << "Rendering Mac button\n"; }
  void onClick() override { std::cout << "Mac click\n"; }
};

// Creator (abstract)
class Dialog {
public:
  virtual std::unique_ptr<Button> createButton() = 0;
  void render() {
    auto btn = createButton(); // Factory Method
    btn->render();
  }
  virtual ~Dialog() = default;
};

// Concrete Creators
class WindowsDialog : public Dialog {
public:
  std::unique_ptr<Button> createButton() override {
    return std::make_unique<WindowsButton>();
  }
};

class MacDialog : public Dialog {
public:
  std::unique_ptr<Button> createButton() override {
    return std::make_unique<MacButton>();
  }
};

// Usage
int main() {
  std::unique_ptr<Dialog> dialog;
  if (currentOS == "Windows")
    dialog = std::make_unique<WindowsDialog>();
  else
    dialog = std::make_unique<MacDialog>();
  dialog->render();
}