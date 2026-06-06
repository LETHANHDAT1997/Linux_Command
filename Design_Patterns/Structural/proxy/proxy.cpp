// Subject interface
class Image {
public:
  virtual void display() = 0;
  virtual ~Image() = default;
};

// Real Subject (expensive to create)
class HighResImage : public Image {
  std::string filename_;
  void loadFromDisk() {
    std::cout << "Loading " << filename_ << " from disk (slow!)\n";
  }

public:
  HighResImage(const std::string &f) : filename_(f) { loadFromDisk(); }
  void display() override { std::cout << "Displaying " << filename_ << "\n"; }
};

// Virtual Proxy - lazy initialization
class ImageProxy : public Image {
  std::string filename_;
  mutable std::unique_ptr<HighResImage> realImage_;

public:
  ImageProxy(const std::string &f) : filename_(f) {}

  void display() override {
    if (!realImage_) // load only when needed
      realImage_ = std::make_unique<HighResImage>(filename_);
    realImage_->display();
  }
};

// Protection Proxy - access control
class SecureImageProxy : public Image {
  std::unique_ptr<HighResImage> real_;
  std::string userRole_;

public:
  SecureImageProxy(const std::string &f, const std::string &role)
      : real_(std::make_unique<HighResImage>(f)), userRole_(role) {}

  void display() override {
    if (userRole_ == "admin")
      real_->display();
    else
      std::cout << "Access denied!\n";
  }
};

int main() {
  ImageProxy img("4K_photo.jpg");
  std::cout << "Proxy created, no disk I/O yet\n";
  img.display(); // loading happens here
  img.display(); // uses cached real image
}