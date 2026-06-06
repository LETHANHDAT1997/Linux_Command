// Abstract Products
class Button {
public:
  virtual void paint() = 0;
  virtual ~Button() = default;
};
class Checkbox {
public:
  virtual void paint() = 0;
  virtual ~Checkbox() = default;
};

// Concrete Products - Windows
class WinButton : public Button {
  void paint() override { std::cout << "[WinButton]\n"; }
};
class WinCheckbox : public Checkbox {
  void paint() override { std::cout << "[WinCheckbox]\n"; }
};

// Concrete Products - Mac
class MacButton : public Button {
  void paint() override { std::cout << "(MacButton)\n"; }
};
class MacCheckbox : public Checkbox {
  void paint() override { std::cout << "(MacCheckbox)\n"; }
};

// Abstract Factory
class GUIFactory {
public:
  virtual std::unique_ptr<Button> createButton() = 0;
  virtual std::unique_ptr<Checkbox> createCheckbox() = 0;
  virtual ~GUIFactory() = default;
};

// Concrete Factories
class WinFactory : public GUIFactory {
public:
  std::unique_ptr<Button> createButton() override {
    return std::make_unique<WinButton>();
  }
  std::unique_ptr<Checkbox> createCheckbox() override {
    return std::make_unique<WinCheckbox>();
  }
};

class MacFactory : public GUIFactory {
public:
  std::unique_ptr<Button> createButton() override {
    return std::make_unique<MacButton>();
  }
  std::unique_ptr<Checkbox> createCheckbox() override {
    return std::make_unique<MacCheckbox>();
  }
};

// Client
class Application {
  std::unique_ptr<Button> btn_;
  std::unique_ptr<Checkbox> chk_;

public:
  Application(std::unique_ptr<GUIFactory> f)
      : btn_(f->createButton()), chk_(f->createCheckbox()) {}
  void paint() {
    btn_->paint();
    chk_->paint();
  }
};

int main() {
  auto factory = std::make_unique<WinFactory>();
  Application app(std::move(factory));
  app.paint(); // [WinButton] [WinCheckbox]
}