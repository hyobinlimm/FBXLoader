#pragma once

#include <functional>
#include <list>
#include <algorithm>
#include <utility>
#include <mutex>
#include <future>

namespace TL_Common
{
    template <typename... Args>
    class Event
    {
    public:
        using THandler = std::function<void(Args ...)>;

        Event() { }

        // copy constructor
        Event(const Event& src)
        {
            std::lock_guard<std::mutex> lock(src.m_HandlersLocker);

            m_Handlers = src.m_Handlers;
        }

        // move constructor
        Event(Event&& src)
        {
            std::lock_guard<std::mutex> lock(src.m_HandlersLocker);

            m_Handlers = std::move(src.m_Handlers);
        }

        // copy assignment operator
        Event& operator=(const Event& src)
        {
            std::lock_guard<std::mutex> lock(m_HandlersLocker);
            std::lock_guard<std::mutex> lock2(src.m_HandlersLocker);

            m_Handlers = src.m_Handlers;

            return *this;
        }

        // move assignment operator
        Event& operator=(Event&& src)
        {
            std::lock_guard<std::mutex> lock(m_HandlersLocker);
            std::lock_guard<std::mutex> lock2(src.m_HandlersLocker);

            std::swap(m_Handlers, src.m_Handlers);

            return *this;
        }

        void AddListener(const THandler& handler)
        {
            std::lock_guard<std::mutex> lock(m_HandlersLocker);

            m_Handlers.push_back(handler);
        }

        bool RemoveListener(const THandler& handler)
        {
            std::lock_guard<std::mutex> lock(m_HandlersLocker);

            const auto _iter = std::find(m_Handlers.begin(), m_Handlers.end(), handler);
            if(_iter != m_Handlers.end())
            {
                m_Handlers.erase(_iter);
                return true;
            }

            return false;
        }

        void Invoke(Args ... params) const
        {
            auto handlersCopy = GetHandlersCopy();

            for (const auto& handler : handlersCopy)
            {
                handler(params...);
            }
        }

        std::future<void> InvokeAsync(Args ... params) const
        {
            return std::async(std::launch::async, [this](Args ... asyncParams) { Invoke(asyncParams...); }, params...);
        }

        inline void operator+=(const THandler& handler)
        {
            AddListener(handler);
        }

        inline bool operator-=(const THandler& handler)
        {
            return RemoveListener(handler);
        }

    protected:
        std::list<THandler> GetHandlersCopy() const
        {
            std::lock_guard<std::mutex> lock(m_HandlersLocker);

            // Since the function return value is by copy, 
            // before the function returns (and destruct the lock_guard object),
            // it creates a copy of the m_handlers container.

            return m_Handlers;
        }

    private:
        std::list<THandler> m_Handlers;
        mutable std::mutex m_HandlersLocker;
    };
}
