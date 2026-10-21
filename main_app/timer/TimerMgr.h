#pragma once

#include <vector>

#include "ITimer.h"

class TimerMgr
{

private:

    /*Container holding all timer clients*/
    std::vector<ITimer*>m_vecClients;

    /*Returns true while the client is still registered in m_vecClients*/
    bool IsClientRegistered(const ITimer *client) const;

public:
    virtual ~TimerMgr() {};
    void StartTimer(ITimer* client, unsigned int unID, unsigned int unPeriod);
    void StopTimer(ITimer* client, unsigned int unID);
    void EraseClient(ITimer* client);

    /*Method called every game cycle*/
    void Process();

};
