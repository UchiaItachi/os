//
// Created by itachi on 9/13/17.
//

#include "MemoryCoordinator.h"

#include <thread>
#include <chrono>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <exception>
#include <cmath>

MemoryCoordinator::MemoryCoordinator()
  : thresholdFreeMemory_(1024 * 150) // 150 MB
  , increaseMemoryThreshold_(thresholdFreeMemory_ << 1) // 300 MB
  , reclaimMemoryThreshold_(thresholdFreeMemory_ >> 1)
  , activeMemoryConsumptionThreshold_(3 * 1024) // 3 MB
  , memoryStatsPeriod_(1)
  , wakeUpPeriod_(1) // 1 s
  , reclaimedDomains_()
{
}

void MemoryCoordinator::updateMemoryStatsCollectionFrequency(int memoryStatsPeriod)
{
  memoryStatsPeriod_ = memoryStatsPeriod;
  for (auto &domain: domains_)
  {
    int ret = virDomainSetMemoryStatsPeriod(domain.second->domain, memoryStatsPeriod_, VIR_DOMAIN_AFFECT_LIVE);
    if (-1 == ret)
    {
      std::cout << std::endl << "Error: failed to update the memory stats period for domain: "
                << domain.second->domainName_ << std::endl;
    }
  }
}

void MemoryCoordinator::initialize()
{
  HypervisorConnection::open();
  updateMemoryStatsCollectionFrequency(memoryStatsPeriod_);
}

void MemoryCoordinator::close()
{
  HypervisorConnection::close();
}

void MemoryCoordinator::analyse(uint32_t wakeUpIntervalInSec)
{
  wakeUpPeriod_ = wakeUpIntervalInSec;
  // warm up, todo: handle gracefully and remove this
  getDomainMemoryInfo();
  getHostMemoryInfo();
  std::this_thread::sleep_for(std::chrono::seconds(wakeUpPeriod_));
  while (1)
  {
    getDomainMemoryInfo();
    getHostMemoryInfo();
    coordinateMemoryRequirements();
#ifdef ENABLE_MEM_DEBUG
    for (auto& domain: domains_)
    {
      std::cout << domain.second->domainName_ << " previouslyActive : " << domain.second->previouslyActive_ << "currentlyActive : " << domain.second->currentlyActive_ << std::endl;
    }
#endif
    std::this_thread::sleep_for(std::chrono::seconds(wakeUpPeriod_));
  }
}

void MemoryCoordinator::coordinateMemoryRequirements()
{
  handleMemoryDeficienyWithoutReclaiming();
  reclaimMemoryInactiveVms();           // from inactive VMs
  reclaimMemoryPreviouslyActiveVms();   // to host
}


void MemoryCoordinator::handleMemoryDeficienyWithoutReclaiming()
{
  for (auto &domain: domains_)
  {
    Domain& currentDomain = *domain.second;
    if (isMemoryDeficient(currentDomain))
    {
      if (canIncreaseWithoutReclaiming(currentDomain))
      {
        increaseMemory(*domain.second);
      }
    }
    else
    {
      // std::cout << "No need to increase more memory to domain : " << domain.second->maxMemory_ << std::endl;
    }
  }
}

void MemoryCoordinator::reclaimMemoryInactiveVms()
{
  for (auto& currentDomain: domains_)
  {
    if (currentDomain.second->currentlyActive_ || wasDomainPreviouslyActiveAndInactiveNow(*currentDomain.second))
    {
      continue;
    }

    if (!canReclaimMemory(*currentDomain.second))
    {
#ifdef ENABLE_MEM_DEBUG
      std::cout << std::endl << "-------- Cannot reclaim memory due to memory hog of the domain " << currentDomain.second->domainName_ << std::endl;
#endif
      continue;
    }

    unsigned long long newMemorySize = currentDomain.second->memInfo_[VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON] - getDecreaseValue();
    int ret = virDomainSetMemory(currentDomain.second->domain, newMemorySize);
    if (ret == -1)
    {
      std::cout << std::endl << "Error: couldn't increase the memory, domain: " << currentDomain.second->domainName_ << std::endl;
    }
    else
    {
      reclaimedDomains_[currentDomain.second] = 0;
      std::cout << std::endl << "******* Reclaim memory of memory inactive domain " << currentDomain.second->domainName_ << " : " << newMemorySize << std::endl;
    }
  }
}

void MemoryCoordinator::reclaimMemoryPreviouslyActiveVms()
{
  for (auto& domain: domains_)
  {
    if (!wasDomainPreviouslyActiveAndInactiveNow(*domain.second))
    {
      continue;
    }

    if (!canReclaimMemory(*domain.second))
    {
#ifdef ENABLE_MEM_DEBUG
      std::cout << std::endl << "-------- Cannot reclaim memory due to memory hog of the previously active domain " << domain.second->domainName_ << std::endl;
#endif
      continue;
    }

    unsigned long long newMemorySize = domain.second->memInfo_[VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON] - getDecreaseValue();
    int ret = virDomainSetMemory(domain.second->domain, newMemorySize);
    if (ret == -1)
    {
      std::cout << std::endl << "Error: couldn't increase the memory, domain: " << domain.second->domainName_ << std::endl;
    }
    else
    {
      reclaimedDomains_[domain.second] = 0;
      std::cout << std::endl << "******* Reclaim memory of previously active now inactive domain " << domain.second->domainName_ << " : " << newMemorySize << std::endl;
    }
  }
}

// -------------------------- HELPERS ---------------------------------------------------------------------------

void MemoryCoordinator::increaseMemory(Domain &domain)
{
  const unsigned long long newMemorySize = domain.memInfo_[VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON] + getIncreaseValue();
  int ret = virDomainSetMemory(domain.domain, newMemorySize);
  if (ret == -1)
  {
    std::cout << std::endl << "Error: couldn't increase the memory, domain: " << domain.domainName_ << std::endl;
  }
  else
  {
    std::cout << std::endl << "---------- Increase memory of domain " << domain.domainName_ << " : " << newMemorySize << std::endl;
  }
}

bool MemoryCoordinator::canReclaimMemory(Domain& currentDomain)
{
  const unsigned long long unusedMemory = currentDomain.memInfo_[VIR_DOMAIN_MEMORY_STAT_UNUSED];
  const long long someFreeMemoryAvailable = unusedMemory - thresholdFreeMemory_;

  if (someFreeMemoryAvailable > 0)
  {
    if (static_cast<unsigned long long>(someFreeMemoryAvailable) > getDecreaseValue())
    {
      return true;
    }
  }
  return false;
}

bool MemoryCoordinator::canIncreaseWithoutReclaiming(Domain &domain)
{
  unsigned long long newMemorySize = domain.memInfo_[VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON] + getIncreaseValue();
  unsigned long long reserveMemory = std::min(2 * getIncreaseValue(), host_->freeMemory_);
  unsigned long long safeLimit = host_->freeMemory_ - reserveMemory;
  if ((newMemorySize < domain.maxMemory_) && (getIncreaseValue() < safeLimit) && (!wasDomainPreviouslyActiveAndInactiveNow(domain)))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool MemoryCoordinator::isMemoryDeficient(Domain &domain)
{
  const unsigned long long freeMemory = domain.memInfo_[VIR_DOMAIN_MEMORY_STAT_UNUSED];
  if (freeMemory < thresholdFreeMemory_)
  {
    return true;
  }
  return false;
}

bool MemoryCoordinator::isDomainActivelyConsumingMemory(Domain &domain)
{
  const unsigned long long diffMemory = std::abs(domain.previousUnsedMemory_ - domain.memInfo_[VIR_DOMAIN_MEMORY_STAT_UNUSED]);

  if (diffMemory > activeMemoryConsumptionThreshold_)
  {
    return true;
  }
  return false;
}

void MemoryCoordinator::checkAndMarkIfActiveDomain(Domain& domain)
{
  const unsigned long long diffMemory = std::abs(domain.previousUnsedMemory_ - domain.memInfo_[VIR_DOMAIN_MEMORY_STAT_UNUSED]);

  if ((!domain.previouslyActive_) && (!domain.currentlyActive_) && (!isDomainReclaimed(&domain)))
  {
    if ((diffMemory > activeMemoryConsumptionThreshold_) && domain.previousUnsedMemory_)
    {
      domain.currentlyActive_ = true;
    }
  }
}

void MemoryCoordinator::checkAndMarkIfInactiveDomain(Domain& domain)
{
  if (domain.currentlyActive_)
  {
    bool isStagnant = domain.previousUnsedMemory_ == domain.memInfo_[VIR_DOMAIN_MEMORY_STAT_UNUSED];
    if (isStagnant)
    {
      domain.currentlyActive_ = false;
      domain.previouslyActive_ = true;
    }
  }
}

bool MemoryCoordinator::wasDomainPreviouslyActiveAndInactiveNow(Domain &domain)
{
  return domain.previouslyActive_ && (!domain.currentlyActive_);
}

bool MemoryCoordinator::isDomainReclaimed(Domain* domain)
{
  return reclaimedDomains_.find(domain) != reclaimedDomains_.end();
}

unsigned long long MemoryCoordinator::getIncreaseValue() const
{
  return increaseMemoryThreshold_;
}

unsigned long long MemoryCoordinator::getDecreaseValue() const
{
  return reclaimMemoryThreshold_;
}

// -------------------------- UPDATE STATISTICS AFTER WAKEUP -----------------------------------------------------

void MemoryCoordinator::getHostMemoryInfo()
{
  getHostMemoryStats();
#ifdef ENABLE_MEM_DEBUG
  std::cout << "host: " << " total : " << host_->maxMemory_ << " unused : " << host_->freeMemory_ << std::endl;
#endif
}

void MemoryCoordinator::getDomainMemoryInfo()
{
  for (auto &domain: domains_)
  {
    getDomainMemoryStats(*domain.second);
    getDomainMaxMemory(*domain.second);

#ifdef ENABLE_MEM_DEBUG
    std::cout << domain.second->domainName_ << " : ";
    std::cout << "max : " << domain.second->maxMemory_ << " , ";
    for (auto &stat: domain.second->memInfo_)
    {
      if (stat.first == VIR_DOMAIN_MEMORY_STAT_AVAILABLE)
      {
        std::cout << "available : ";
        std::cout << stat.second << " , ";
      } else if (stat.first == VIR_DOMAIN_MEMORY_STAT_UNUSED)
      {
        std::cout << "unused : ";
        std::cout << stat.second << " , ";
      } else if (stat.first == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)
      {
        std::cout << "balloon : ";
        std::cout << stat.second << " , ";
      }
    }
    std::cout << std::endl;
#endif
  }
}

// ---------------------------- DOMAIN MEMORY STATS ---------------------------------------------------------------

void MemoryCoordinator::getDomainMemoryStats(Domain &domain)
{
  std::vector<virDomainMemoryStatStruct> stats(VIR_DOMAIN_MEMORY_STAT_NR);
  int ret = virDomainMemoryStats(domain.domain, stats.data(), stats.size(), 0);
  if (ret == VIR_DOMAIN_MEMORY_STAT_NR)
  {
    domain.previousUnsedMemory_ = domain.memInfo_[VIR_DOMAIN_MEMORY_STAT_UNUSED];
    for (auto &stat: stats)
    {
      domain.memInfo_[static_cast<virDomainMemoryStatTags>(stat.tag)] = stat.val;
    }
    checkAndMarkIfActiveDomain(domain);
    checkAndMarkIfInactiveDomain(domain);
  }
  else
  {
    std::cout << std::endl << "Error: failed to get stats for the domain :  " << domain.domainName_ << std::endl;
  }
}

void MemoryCoordinator::getDomainMaxMemory(Domain &domain)
{
  unsigned long memSize = virDomainGetMaxMemory(domain.domain);
  if (memSize)
  {
    domain.maxMemory_ = memSize;
  } else
  {
    std::cout << std::endl << "Error: failed to get max memory for the domain :  " << domain.domainName_ << std::endl;
  }
}

// ---------------------------- HOST MEMORY STATS ---------------------------------------------------------------

void MemoryCoordinator::getHostMemoryStats()
{
  int nparams = 0;
  if (virNodeGetMemoryStats(connection_, 0, nullptr, &nparams, 0) == 0 && nparams != 0)
  {
    std::vector<virNodeMemoryStats> stats(nparams);
    if (virNodeGetMemoryStats(connection_, 0, stats.data(), &nparams, 0))
    {
      std::cout << "Error: Couldn't get domain memory params" << std::endl;
      return;
    }
    for (auto &st: stats)
    {
      if (std::strcmp(st.field, VIR_NODE_MEMORY_STATS_FREE) == 0)
      {
        host_->freeMemory_ = st.value;
      }
      if (std::strcmp(st.field, VIR_NODE_MEMORY_STATS_TOTAL) == 0)
      {
        host_->maxMemory_ = st.value;
      }
    }
  } else
  {
    std::cout << std::endl << "Error: Couldn't get memory stats of host" << std::endl;
  }
}

// ---------------------------------------------------------------------------------------

#if 0
void MemoryCoordinator::getHostMemoryParam()
{
  int nparams = 0;
  if (virNodeGetMemoryParameters(connection_, NULL, &nparams, 0) == 0 && nparams != 0)
  {
    std::vector<virTypedParameter> params;
    params.resize(nparams);
    if (virNodeGetMemoryParameters(connection_, params.data(), &nparams, 0))
    {
      std::cout << std::endl << "Error: Couldn't get domain memory params" << std::endl;
      return;
    }
    std::cout << std::endl << "Memory params of host : " << std::endl;
    for (auto &pa: params)
    {
      std::cout << pa.field << " : " << pa.value.ul << std::endl;
    }
  }
  else
  {
    std::cout << std::endl << "Error: Couldn't get memory params of host" << std::endl;
  }
}

void MemoryCoordinator::getDomainMemoryParam()
{
  std::cout << std::endl << "Memory params for Domain : " << std::endl;
  for (auto &domain: domains_)
  {
    std::cout << domain.second->domainName_ << " -> ";

    int nparams = 0;
    if (virDomainGetMemoryParameters(domain.second->domain, NULL, &nparams, 0) == 0 && nparams != 0)
    {
      std::vector<virTypedParameter> params;
      params.resize(nparams);
      if (virDomainGetMemoryParameters(domain.second->domain, params.data(), &nparams, 0))
      {
        std::cout << std::endl << "Error: Could get domain memory params" << std::endl;
        return;
      }
      for (auto &pa: params)
      {
        std::cout << pa.field << " : " << pa.value.ul << " ";
      }
    }
    else
    {
      std::cout << std::endl << "Error: Could get domain memory params" << std::endl;
    }
    std::cout << std::endl;
  }
}

#endif