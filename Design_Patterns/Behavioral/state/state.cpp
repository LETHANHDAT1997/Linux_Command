class TrafficLight; // forward declaration

class LightState {
public:
    virtual void handle(TrafficLight& light) = 0;
    virtual std::string name() const = 0;
    virtual ~LightState() = default;
};

// Context
class TrafficLight {
    std::unique_ptr<LightState> state_;
public:
    TrafficLight(std::unique_ptr<LightState> initial)
        : state_(std::move(initial)) {}
    void setState(std::unique_ptr<LightState> s) {
        std::cout << "Transitioning to: " << s->name() << "\n";
        state_ = std::move(s);
    }
    void tick() { state_->handle(*this); }
};

class RedState    : public LightState { public: std::string name() const override { return "RED";    } void handle(TrafficLight& light) override; };
class YellowState : public LightState { public: std::string name() const override { return "YELLOW"; } void handle(TrafficLight& light) override; };
class GreenState  : public LightState { public: std::string name() const override { return "GREEN";  } void handle(TrafficLight& light) override; };

void RedState::handle(TrafficLight& light) {
    std::cout << "RED -> Stopping traffic\n";
    light.setState(std::make_unique<GreenState>());
}
void GreenState::handle(TrafficLight& light) {
    std::cout << "GREEN -> Traffic flowing\n";
    light.setState(std::make_unique<YellowState>());
}
void YellowState::handle(TrafficLight& light) {
    std::cout << "YELLOW -> Slow down\n";
    light.setState(std::make_unique<RedState>());
}

int main() {
    TrafficLight light(std::make_unique<RedState>());
    for (int i = 0; i < 6; ++i) light.tick();
    // RED -> GREEN -> YELLOW -> RED -> GREEN -> YELLOW
}