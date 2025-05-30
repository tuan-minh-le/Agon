//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_IMPL_SAVED_HANDLER_HPP
#define BOOST_BEAST_CORE_IMPL_SAVED_HANDLER_HPP

#include <boost/beast/core/detail/allocator.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/assert.hpp>
#include <boost/core/empty_value.hpp>
#include <boost/core/exchange.hpp>
#include <utility>

namespace boost {
namespace beast {

//------------------------------------------------------------------------------

class saved_handler::base
{
protected:
    ~base() = default;
    saved_handler * owner_;
public:
    base(saved_handler * owner) : owner_(owner){}
    void set_owner(saved_handler * new_owner) { owner_ = new_owner;}
    virtual void destroy() = 0;
    virtual void invoke() = 0;
};

//------------------------------------------------------------------------------

template<class Handler, class Alloc>
class saved_handler::impl final : public base
{
    using alloc_type = typename
        beast::detail::allocator_traits<
            Alloc>::template rebind_alloc<impl>;

    using alloc_traits =
        beast::detail::allocator_traits<alloc_type>;

    struct ebo_pair : boost::empty_value<alloc_type>
    {
        Handler h;

        template<class Handler_>
        ebo_pair(
            alloc_type const& a,
            Handler_&& h_)
            : boost::empty_value<alloc_type>(
                boost::empty_init_t{}, a)
            , h(std::forward<Handler_>(h_))
        {
        }
    };

    ebo_pair v_;
    net::executor_work_guard<
        net::associated_executor_t<Handler>> wg2_;
    net::cancellation_slot slot_{net::get_associated_cancellation_slot(v_.h)};
public:
    template<class Handler_>
    impl(alloc_type const& a, Handler_&& h, 
         saved_handler * owner)
        : base(owner), v_(a, std::forward<Handler_>(h))
        , wg2_(net::get_associated_executor(v_.h))
    {
    }

    ~impl()
    {
    }

    void
    destroy() override
    {
        auto v = std::move(v_);
        slot_.clear();
        alloc_traits::destroy(v.get(), this);
        alloc_traits::deallocate(v.get(), this, 1);
    }

    void
    invoke() override
    {
        slot_.clear();
        auto v = std::move(v_);
        alloc_traits::destroy(v.get(), this);
        alloc_traits::deallocate(v.get(), this, 1);
        v.h();
    }

    void self_complete()
    {
        slot_.clear();
        owner_->p_ = nullptr;
        auto v = std::move(v_);
        alloc_traits::destroy(v.get(), this);
        alloc_traits::deallocate(v.get(), this, 1);
        v.h(net::error::operation_aborted);
    }
};

//------------------------------------------------------------------------------

template<class Handler, class Allocator>
void
saved_handler::
emplace(Handler&& handler, Allocator const& alloc,
        net::cancellation_type cancel_type)
{
    // Can't delete a handler before invoking
    BOOST_ASSERT(! has_value());
    using handler_type =
        typename std::decay<Handler>::type;
    using alloc_type = typename
        detail::allocator_traits<Allocator>::
            template rebind_alloc<impl<
                handler_type, Allocator>>;
    using alloc_traits =
        beast::detail::allocator_traits<alloc_type>;
    struct storage
    {
        alloc_type a;
        impl<Handler, Allocator>* p;

        explicit
        storage(Allocator const& a_)
            : a(a_)
            , p(alloc_traits::allocate(a, 1))
        {
        }

        ~storage()
        {
            if(p)
                alloc_traits::deallocate(a, p, 1);
        }
    };


    auto cancel_slot = net::get_associated_cancellation_slot(handler);
    storage s(alloc);
    alloc_traits::construct(s.a, s.p,
        s.a, std::forward<Handler>(handler), this);
    
    auto tmp = boost::exchange(s.p, nullptr);
    p_ = tmp;

    if (cancel_slot.is_connected())
    {
        struct cancel_op
        {
            impl<Handler, Allocator>* p;
            net::cancellation_type accepted_ct;
            cancel_op(impl<Handler, Allocator>* p,
                      net::cancellation_type accepted_ct) 
                        : p(p), accepted_ct(accepted_ct) {}

            void operator()(net::cancellation_type ct)
            {
                if ((ct & accepted_ct) != net::cancellation_type::none)
                    p->self_complete();
            }
        };
        cancel_slot.template emplace<cancel_op>(tmp, cancel_type);
    }
}

template<class Handler>
void
saved_handler::
emplace(Handler&& handler, net::cancellation_type cancel_type)
{
    // Can't delete a handler before invoking
    BOOST_ASSERT(! has_value());
    emplace(
        std::forward<Handler>(handler),
        net::get_associated_allocator(handler),
        cancel_type);
}

} // beast
} // boost

#endif
