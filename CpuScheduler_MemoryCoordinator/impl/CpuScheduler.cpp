//
// Created by itachi on 9/10/17.
//


#include "CpuScheduler.h"

#include <libvirt/libvirt.h>

#include <thread>
#include <iomanip>
#include <bitset>
#include <algorithm>

CpuScheduler::CpuScheduler()
  : cpuPinMapping_()
  , scheduleInterval_(0)
  , checkFrequency_(5)
  , currentSchedule_(0)
  , toleranceLimit_(10)
{
}

void CpuScheduler::initialize()
{
  HypervisorConnection::open();
  HypervisorConnection::printInitInfo();
}

void CpuScheduler::schedule(uint32_t timeIntervalInSec)
{
  scheduleInterval_ = timeIntervalInSec;

  while (1)
  {
    currentSchedule_++;
    std::this_thread::sleep_for(std::chrono::seconds(scheduleInterval_));
    for (auto& item: cpuPinMapping_)
    {
      std::cout << item.first << " : " << host_->pCpuInfo_[item.first].cpuUsageFraction << " [ ";
      for (auto &vCpu: item.second)
      {
        std::cout << vCpu->domainInfo->domainName_ << ", ";
      }
      std::cout << " ] " << std::endl;
    }
    std::cout << "-------------------------------------------------- " << std::endl;
    getStatisticsOfDomain();
    getStatisticsOfHost();
    pinCpus();
  }
}

void CpuScheduler::close()
{
  HypervisorConnection::close();
}

void CpuScheduler::getStatisticsOfDomain()
{
  HypervisorConnection::getUsageOfAllDomains();
}

unsigned long long getPCpuBuget(std::vector<VCpuInfo *> &vCpus)
{
  unsigned long long pCpuBuget = 0;
  for (auto &i: vCpus)
  {
    pCpuBuget += i->vCpuUsageFraction;
  }
  return pCpuBuget;
};


void CpuScheduler::getStatisticsOfHost()
{
  host_->currentCpuUsage();
  for (auto& pCpuInfo: host_->pCpuInfo_)
  {
    std::cout << pCpuInfo.first << " : " << pCpuInfo.second.cpuUsageFraction << std::endl;
  }
}


static bool pinned = false;

bool CpuScheduler::arePCpusBalanced(unsigned long long balancedCpuUsage)
{
  toleranceLimit_ = balancedCpuUsage / 10;
  const unsigned long long low = balancedCpuUsage - toleranceLimit_;
  const unsigned long long high = balancedCpuUsage + toleranceLimit_;
  std::cout <<"balanced range" << low << " , " << high << std::endl;
  return host_->arePCpusBalanced(low, high);
}


void CpuScheduler::pinCpus()
{
  std::cout << "Iteration - " << currentSchedule_ << std::endl;

  if (currentSchedule_ % checkFrequency_)
  {
    return;
  }

  unsigned long long totalVCpuUsage = 0;
  std::vector<VCpuInfo*> vCpuInfos_;

  std::function<bool(const VCpuInfo*, const VCpuInfo*)> compareVCpuUsageGreater =
          [](const VCpuInfo* left, const VCpuInfo* right) -> bool { return left->vCpuUsageFraction > right->vCpuUsageFraction; };

  // create a sorted list of vCpu usages
  for (auto& domain: domains_)
  {
    totalVCpuUsage += domain.second->cpuUsageFraction;
    for (auto& info: domain.second->vCpuInfos_)
    {
      vCpuInfos_.push_back(&info);
    }
  }

  std::sort(vCpuInfos_.begin(), vCpuInfos_.end(), compareVCpuUsageGreater);

  std::cout << "Sorted ... \n";
  for (auto& info: vCpuInfos_)
  {
    std::cout << info->vCpuUsageFraction << " ";
  }
  std::cout << std::endl;

  unsigned long long balancedPCputime = (totalVCpuUsage / numOfCpusOnHost_); // in seconds

  std::cout << "Total vCpuUsage : " << totalVCpuUsage << std::endl;
  if (arePCpusBalanced(balancedPCputime))
  {
    std::cout << "Cpu's balanced - no pinning " << std::endl;
    return;
  }
  else
  {
    std::cout << "Pinning - expected balance time : (" << balancedPCputime - toleranceLimit_ << " , " << balancedPCputime << " , " << balancedPCputime + toleranceLimit_ << " ) " << std::endl;
  }
  for (auto& item: cpuPinMapping_)
  {
    item.second.clear();
  }

  // Map the first half
  size_t firstHalfCpus = vCpuInfos_.size() > static_cast<size_t>(numOfCpusOnHost_) ? static_cast<size_t>(numOfCpusOnHost_) : vCpuInfos_.size();
  for (size_t pCpu = 0; pCpu < firstHalfCpus; ++pCpu)
  {
    cpuPinMapping_[pCpu].push_back(*vCpuInfos_.begin());
    vCpuInfos_.erase(vCpuInfos_.begin());
  }

  // Map the remaining
  for (;vCpuInfos_.size();)
  {
    for (uint32_t pCpu = numOfCpusOnHost_; pCpu > 0; --pCpu)
    {
      cpuPinMapping_[pCpu - 1].push_back(*vCpuInfos_.begin());
      vCpuInfos_.erase(vCpuInfos_.begin());
    }
  }
  for (auto& item: cpuPinMapping_)
  {
    unsigned char cpumap = static_cast<unsigned char>(0x1 << item.first);
    //std::cout << item.first << " : " << host_->pCpuInfo_[item.first].cpuUsageFraction << " [ ";
    for (auto& vCpu: item.second)
    {
      int ret = virDomainPinVcpu(vCpu->domainInfo->domain, vCpu->vCpuNumber, &cpumap, sizeof(cpumap));
      if (ret != 0)
      {
        std::cout << "Error: Couldn't ping the vcpu " << vCpu->vCpuNumber << " of domain " << vCpu->domainInfo->domainName_ << " to pCpu " << item.first << std::endl;
      }
      else
      {
      //  std::cout << vCpu->domainInfo->domainName_ << ", ";
      }
    }
    //std::cout << " ] " << std::endl;
  }
  pinned = true;
}