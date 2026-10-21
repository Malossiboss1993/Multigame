#include "TimerMgr.h"

#include <algorithm>
#include <chrono>

#include <debug/Logger.h>

namespace
{
    bool IsTimerDue(const Timer &timer, const std::chrono::system_clock::time_point &currentTime)
    {
        const auto deltaTime = currentTime - timer.startTime;
        const auto deltaTimeInMillis =
            std::chrono::duration_cast<std::chrono::milliseconds>(deltaTime).count();

        return deltaTimeInMillis >= timer.unPeriod;
    }
}

void TimerMgr::StartTimer(ITimer *client, unsigned int unID, unsigned int unPeriod)
{
    if (client == nullptr)
    {
        LOG_ERROR("TimerMgr - Cannot start timer \"{0}\" for a null client", unID);
        return;
    }

    const auto currentTime = std::chrono::system_clock::now();

    /*Check if there is already a timer with this ID*/
    for (auto &timer : client->m_vecTimers)
    {
        if (timer.unID == unID)
        {
            if (timer.bNeedToDeleteTimer)
            {
                timer.bNeedToDeleteTimer = false;
                timer.unTimes = 0;
                timer.unPeriod = unPeriod;
                timer.startTime = currentTime;
            }

            /*Starting an already-running timer is intentionally idempotent.
              Restarting it here could postpone a timer forever when callers
              request it repeatedly, so keep the existing cadence and return.*/
            return;
        }
    }

    /*Create current timer object*/
    Timer timerObj;
    timerObj.unID = unID;
    timerObj.unPeriod = unPeriod;
    timerObj.startTime = currentTime;

    /*Register current timer object in vector of timers in the current client*/
    client->m_vecTimers.emplace_back(timerObj);

    /*Register the current client in the vector of observer clients*/
    if (!IsClientRegistered(client))
    {
        m_vecClients.emplace_back(client);
    }
}

void TimerMgr::StopTimer(ITimer *client, unsigned int unID)
{
    if (client == nullptr)
    {
        return;
    }

    /*Mark timer for remove*/
    for (auto &timer : client->m_vecTimers)
    {
        if (timer.unID == unID)
        {
            timer.bNeedToDeleteTimer = true;
        }
    }
}

bool TimerMgr::IsClientRegistered(const ITimer *client) const
{
    return std::find(m_vecClients.begin(), m_vecClients.end(), client) != m_vecClients.end();
}

void TimerMgr::EraseClient(ITimer *client)
{
    if (client == nullptr)
    {
        return;
    }

    m_vecClients.erase(std::remove(m_vecClients.begin(), m_vecClients.end(), client),
                       m_vecClients.end());
}

void TimerMgr::Process()
{
    const auto currentTime = std::chrono::system_clock::now();

    /*OnTick callbacks are allowed to start/stop timers and register/unregister
      clients. Iterating m_vecClients or m_vecTimers directly would invalidate
      iterators when a vector reallocates, so process snapshots instead.*/
    const std::vector<ITimer *> vecClientsSnapshot = m_vecClients;

    for (ITimer *client : vecClientsSnapshot)
    {
        if (client == nullptr || !IsClientRegistered(client))
        {
            continue;
        }

        /*Remove timers stopped during the previous frame*/
        auto &vecTimers = client->m_vecTimers;
        vecTimers.erase(std::remove_if(vecTimers.begin(), vecTimers.end(),
                                       [](const Timer &timer)
                                       {
                                           return timer.bNeedToDeleteTimer;
                                       }),
                        vecTimers.end());

        /*Collect due timers before invoking callbacks. A callback can append to
          vecTimers, so the iterator returned by begin() must not be kept across
          an OnTick call.*/
        std::vector<Timer> vecDueTimers;
        vecDueTimers.reserve(vecTimers.size());

        for (const auto &timer : vecTimers)
        {
            if (IsTimerDue(timer, currentTime))
            {
                vecDueTimers.emplace_back(timer);
            }
        }

        for (const auto &dueTimer : vecDueTimers)
        {
            if (!IsClientRegistered(client))
            {
                break;
            }

            /*The timer may have been stopped or restarted by a previous callback
              in this same frame. Re-fetch it before invoking OnTick.*/
            const auto it = std::find_if(vecTimers.begin(), vecTimers.end(),
                                         [&dueTimer](const Timer &timer)
                                         {
                                             return timer.unID == dueTimer.unID;
                                         });

            if (it == vecTimers.end() || it->bNeedToDeleteTimer)
            {
                continue;
            }

            if (!IsTimerDue(*it, currentTime))
            {
                continue;
            }

            it->startTime = currentTime;
            const unsigned int unTimes = ++it->unTimes;
            client->OnTick(it->unID, unTimes);
        }
    }
}
