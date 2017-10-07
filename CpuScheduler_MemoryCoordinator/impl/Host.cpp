//
// Created by itachi on 9/16/17.
//

#include "Host.h"
#include "Domain.h"

#include <iostream>
#include <cstring>

Host::Host(virConnectPtr connection, std::map<uint32_t, Domain*>& domains)
  : connection_(connection)
  , freeMemory_(0)
  , maxMemory_(0)
  , domains_(domains)

{
}

uint32_t Host::getNumberOfHostCpus()
{
  int nPCpus = virNodeGetCPUMap(connection_, nullptr, nullptr, 0);
  if (nPCpus != -1)
  {
    numOfCpusOnHost_ = nPCpus;
    for (uint32_t cpuNum = 0; cpuNum < static_cast<uint32_t>(nPCpus); ++cpuNum)
    {
      pCpuInfo_[cpuNum].cpuUsageFraction = 0;
      pCpuInfo_[cpuNum].cpuUsage = 0;
    }
  }
  return numOfCpusOnHost_;
}

unsigned long long Host::currentCpuUsage()
{
  unsigned long long totalCpuUsage = 0;
#if 0
  totalCpuUsage = currentCpuUsage_1();
#endif

#if 1
  totalCpuUsage = currentCpuUsage_2();
#endif
  return totalCpuUsage;
}

void Host::currentCpuUsageOfPCpus()
{
  currentCpuUsage_1();
}


unsigned long long Host::currentCpuUsage_1()
{
  unsigned long long totalCpuUsage = 0;
  for (uint32_t cpuNum = 0; cpuNum < numOfCpusOnHost_; ++cpuNum)
  {
    int nparams = 0;
    if (virNodeGetCPUStats(connection_, cpuNum, NULL, &nparams, 0) == 0 && nparams != 0)
    {
      std::vector<virNodeCPUStats> params(nparams);
      int ret = virNodeGetCPUStats(connection_, cpuNum, params.data(), &nparams, 0);
      if (ret != -1)
      {
        unsigned long long nowTime = 0;
        extractHostCpuUtilization(params, nowTime);
        pCpuInfo_[cpuNum].cpuUsageFraction = convertNanoSecondsToSeconds(nowTime - pCpuInfo_[cpuNum].cpuUsage);
        pCpuInfo_[cpuNum].cpuUsage = nowTime;
      }
    }
  }
  return totalCpuUsage;
}

unsigned long long Host::currentCpuUsage_2()
{
  unsigned long long totalCpuUsage = 0;
  for (auto& item: pCpuInfo_)
  {
    unsigned long long pCpuUsage = 0;
    for (auto& domain: domains_)
    {
      for (auto &vCpuInfo: domain.second->vCpuInfos_)
      {
        if (item.first == vCpuInfo.pCpuNumber)
        {
          pCpuUsage += vCpuInfo.vCpuUsage;
        }
      }
    }
    item.second.cpuUsageFraction = convertNanoSecondsToSeconds(pCpuUsage - item.second.cpuUsage);
    item.second.cpuUsage = pCpuUsage;
    totalCpuUsage += item.second.cpuUsageFraction;
  }
  return totalCpuUsage;
}


void Host::extractHostCpuUtilization(std::vector<virNodeCPUStats>& params, unsigned long long& cpuUtil)
{
  for (auto &pa: params)
  {
    if (strcmp(pa.field, VIR_NODE_CPU_STATS_UTILIZATION) == 0)
    {
      cpuUtil += pa.value;
      return;
    }

    if (strcmp(pa.field, VIR_NODE_CPU_STATS_KERNEL) == 0)
    {
      cpuUtil += pa.value;
    }
    if (strcmp(pa.field, VIR_NODE_CPU_STATS_USER) == 0)
    {
      cpuUtil += pa.value;
    }
  }
}

bool Host::arePCpusBalanced(unsigned long long lowLimit, unsigned long long highLimit) const
{
  for (auto& pCpu: pCpuInfo_)
  {
    std::cout << "pCpu Balanced - " << pCpu.first << " : " << pCpu.second.cpuUsageFraction << std::endl;
    if ((pCpu.second.cpuUsageFraction < lowLimit) || (pCpu.second.cpuUsageFraction > highLimit))
    {
      return false;
    }
  }
  return true;
}
