#include <iostream>
#include <vector>
#include <string>
#include <functional>

void p(const std::string &s, int n)
{
    std::cout << s << n<<std::endl;
}
using task_t = std::function<void()>;
int main()
{
    std::vector<task_t> t;
    t.push_back(std::bind(p, "aaaa", 1));
    t.push_back(std::bind(p, "aaaaa", 1));

    t.push_back(std::bind(p, "aaaaaa", 1));

    t.push_back(std::bind(p, "aaaaaaa", 1));
    auto a = std::bind(p,"vvvv",std::placeholders::_1);
    a(2);
    for(auto & e:t)
    {
        e();
    }
    return 0;
}