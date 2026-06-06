struct Pizza {
  std::string dough, sauce, topping;
  void show() const {
    std::cout << "Dough:" << dough << " Sauce:" << sauce
              << " Topping:" << topping << "\n";
  }
};

// Builder interface
class PizzaBuilder {
protected:
  Pizza pizza_;

public:
  virtual void buildDough() = 0;
  virtual void buildSauce() = 0;
  virtual void buildTopping() = 0;
  Pizza getResult() { return pizza_; }
  virtual ~PizzaBuilder() = default;
};

// Concrete Builders
class MargheritaBuilder : public PizzaBuilder {
public:
  void buildDough() override { pizza_.dough = "thin"; }
  void buildSauce() override { pizza_.sauce = "tomato"; }
  void buildTopping() override { pizza_.topping = "mozzarella"; }
};

class BBQBuilder : public PizzaBuilder {
public:
  void buildDough() override { pizza_.dough = "thick"; }
  void buildSauce() override { pizza_.sauce = "BBQ"; }
  void buildTopping() override { pizza_.topping = "chicken+onion"; }
};

// Director
class Cook {
  PizzaBuilder *builder_;

public:
  Cook(PizzaBuilder *b) : builder_(b) {}
  void makePizza() {
    builder_->buildDough();
    builder_->buildSauce();
    builder_->buildTopping();
  }
};

int main() {
  MargheritaBuilder mb;
  Cook cook(&mb);
  cook.makePizza();
  mb.getResult().show();
  // Dough:thin Sauce:tomato Topping:mozzarella
}
