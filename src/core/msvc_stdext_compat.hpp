// SPDX-License-Identifier: LGPL-3.0-only
//
// MSVC stdext compatibility polyfill / MSVC stdext 兼容垫片
//
// Qt 6.5's qcompilerdetection.h maps QT_MAKE_CHECKED_ARRAY_ITERATOR to
// stdext::make_checked_array_iterator on MSVC unconditionally. Microsoft
// removed stdext::checked_array_iterator / make_*_array_iterator in MSVC
// Build Tools 14.51, so those TUs fail to compile against newer toolsets.
// This header is force-included before Qt headers and reintroduces thin
// iterator wrappers with the same call shape; they forward straight to the
// underlying raw pointer iterator.
//
// Qt 6.5 的 qcompilerdetection.h 在 MSVC 下无条件将
// QT_MAKE_CHECKED_ARRAY_ITERATOR 映射到 stdext::make_checked_array_iterator；
// 微软在 MSVC 14.51 中移除了这些非标准扩展。本头文件在 Qt 头之前强制包含，
// 用相同调用形态的薄包装补回对应迭代器，内部直接转发到底层原生指针迭代器。
#ifndef GC_MSVC_STDEXT_COMPAT_HPP
#define GC_MSVC_STDEXT_COMPAT_HPP

#if defined(_MSC_VER)

#include <cstddef>
#include <iterator>

namespace stdext {

template <typename Iter>
class checked_array_iterator
{
public:
    using iterator_type = Iter;

    checked_array_iterator(Iter it, std::size_t) noexcept : m_it(it) {}

    Iter base() const noexcept { return m_it; }

    decltype(*std::declval<Iter>()) operator*() const noexcept { return *m_it; }
    Iter operator->() const noexcept { return m_it; }

    checked_array_iterator &operator++() noexcept { ++m_it; return *this; }
    checked_array_iterator operator++(int) noexcept
    {
        checked_array_iterator r(*this);
        ++m_it;
        return r;
    }
    checked_array_iterator &operator+=(std::ptrdiff_t n) noexcept
    {
        m_it += n;
        return *this;
    }
    checked_array_iterator operator+(std::ptrdiff_t n) const noexcept
    {
        return checked_array_iterator(m_it + n, 0);
    }
    std::ptrdiff_t operator-(const checked_array_iterator &other) const noexcept
    {
        return m_it - other.m_it;
    }
    bool operator==(const checked_array_iterator &other) const noexcept
    {
        return m_it == other.m_it;
    }
    bool operator!=(const checked_array_iterator &other) const noexcept
    {
        return m_it != other.m_it;
    }

private:
    Iter m_it;
};

template <typename Iter>
checked_array_iterator<Iter> make_checked_array_iterator(Iter it, std::size_t n)
{
    return checked_array_iterator<Iter>(it, n);
}

template <typename Iter>
Iter make_unchecked_array_iterator(Iter it) noexcept
{
    return it;
}

} // namespace stdext

#endif // _MSC_VER

#endif // GC_MSVC_STDEXT_COMPAT_HPP
