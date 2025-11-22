#include <iostream>
#include <random>
#include <coroutine>

class Generator
{
public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

    int operator()()
    {
        g.resume();
        return g.promise().value;
    }

    bool done() const { return !g || g.done(); }

    struct promise_type
    {
        int value = 0;

        void unhandled_exception() { std::terminate(); }
        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }
        auto yield_value(int v)
        {
            value = v;
            std::cout << "Generated number: " << value << std::endl;
            return std::suspend_always{};
        }
        auto get_return_object() { return Generator{ handle::from_promise(*this) }; }
        void return_void() {}
    };

    Generator(Generator const&) = delete;
    Generator& operator=(Generator const&) = delete;
    Generator(Generator&& other) : g(other.g) { other.g = nullptr; }
    ~Generator() { if (g) g.destroy(); }

private:
    Generator(handle h) : g(h) {}
    handle g;
};

struct Awaiter
{
    int prevNum, currNum;

    bool await_ready() const noexcept { return true; }

    void await_suspend(std::coroutine_handle<> h) const noexcept {}

    bool await_resume() const noexcept { return currNum > prevNum - 16 && currNum < prevNum + 16; }
};

Generator coro_gen()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 256);

    int prevNum;
    int currNum = dist(gen);

    co_yield currNum;

    while (true)
    {
        prevNum = currNum;
        currNum = dist(gen);

        co_yield currNum;

        if (co_await Awaiter{ prevNum, currNum }) break;
    }
}

int main()
{
    auto Gen = coro_gen();

    std::cout << "Begin generating!\n";

    while (!Gen.done()) Gen();

    std::cout << "The end.";

    return 0;
}