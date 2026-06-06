// Strategy interface
class SortStrategy 
{
public:
    virtual void sort(std::vector<int>& data) = 0;
    virtual std::string name() const = 0;
    virtual ~SortStrategy() = default;
};

// Concrete Strategies
class BubbleSort : public SortStrategy 
{
public:
    std::string name() const override { return "BubbleSort"; }
    void sort(std::vector<int>& data) override 
    {
        for (size_t i = 0; i < data.size(); ++i)
            for (size_t j = 0; j < data.size()-i-1; ++j)
                if (data[j] > data[j+1]) std::swap(data[j], data[j+1]);
    }
};

class QuickSort : public SortStrategy 
{
    void qsort(std::vector<int>& d, int l, int r) 
    {
        if (l >= r) return;
        int pivot = d[r], i = l;
        for (int j = l; j < r; ++j)
            if (d[j] <= pivot) std::swap(d[i++], d[j]);
        std::swap(d[i], d[r]);
        qsort(d, l, i-1); qsort(d, i+1, r);
    }
public:
    std::string name() const override { return "QuickSort"; }
    void sort(std::vector<int>& data) override 
    {
        if (!data.empty()) qsort(data, 0, data.size()-1);
    }
};

// Context
class Sorter 
{
    std::unique_ptr<SortStrategy> strategy_;
public:
    Sorter(std::unique_ptr<SortStrategy> s) : strategy_(std::move(s)) {}
    void setStrategy(std::unique_ptr<SortStrategy> s) { strategy_ = std::move(s); }
    void sort(std::vector<int>& data) 
    {
        std::cout << "Using " << strategy_->name() << "\n";
        strategy_->sort(data);
    }
};

int main() 
{
    std::vector<int> data = {5, 2, 8, 1, 9, 3};
    Sorter sorter(std::make_unique<QuickSort>());
    sorter.sort(data); // Using QuickSort

    sorter.setStrategy(std::make_unique<BubbleSort>());
    sorter.sort(data); // Using BubbleSort
}