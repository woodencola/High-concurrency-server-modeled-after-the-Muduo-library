#pragma once
#include<iostream>
#include<algorithm>
#include <typeinfo>
class Any
{
private:
    class Holder
    {
    public:
        Holder() {};
        virtual ~Holder() {};
       virtual const std::type_info& Type()const =0;
       //这个地方要考虑const 对象也能够进行调用
        virtual Holder *clone() const= 0;
    };
    template <typename T>
    class PlaceHolder : public Holder
    {
    public:
        PlaceHolder(const T &val) : _val(val) {}
        virtual ~PlaceHolder() {}
        virtual const std::type_info& Type() const override
        {
            //此处不能用用decltype,这是编译期的,能够定义对象,但是对于我们的any来说
            //当前对象可能再程序运行期间多次更换类型
            return typeid(_val);
        };
        virtual Holder *clone() const override
        {
            //根据当前自身的对象的值构造出一个新的对象,防止浅拷贝的问题
            return new PlaceHolder<T>(_val);
        };

    public:
        T _val;
    };
    Holder *_content;

public:
    Any() :_content(nullptr){}
    template <typename T>
    Any(const T &val) {
        _content = new PlaceHolder<T>(val);
    }
    Any(const  Any &other) {
        _content = other._content!=nullptr?other._content->clone():nullptr;
    }

    Any &operator=(const Any &other) {
        Any(other).swap(*this);
        return *this;

    }
    template <typename T>
    Any &operator=(const T &val) {
        Any(val).swap(*this);
        return*this;

    }

    template <typename T>
    T *Get()
    {
        if(_content==nullptr)return nullptr;
        //这里需要把基类指针强转成子类指针,才能够访问子类对象
        //这里有优先级问题,能加括号就加括号,或者采用c++的方式
        //(PlaceHolder<T>*)_content
        auto it = dynamic_cast<PlaceHolder<T>*>(_content);
        //需要判断是否转换成功,不成功但是不判断会出事
    //    你存了int，却调用Get<std::string>()，dynamic_cast返回nullptr，你直接对空指针取值 ,程序直接闪退
        return it?&((it)->_val):nullptr;
    }
    ~Any() {
        delete _content;
    }
public:
    Any& swap( Any& other) noexcept  
    {
        //交换两个指针,用临时对象构造出来一个新类型的对象
        //另外一个会随临时对象销毁
        std::swap(_content,other._content);
        return *this;
    }
};