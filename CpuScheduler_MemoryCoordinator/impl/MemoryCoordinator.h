//
// Created by itachi on 9/13/17.
//

#ifndef IMPL_MEMORYCOORDINATOR_H
#define IMPL_MEMORYCOORDINATOR_H

#include "HypervisorConnection.h"

class MemoryCoordinator: public HypervisorConnection
{
public:
   MemoryCoordinator();
   void initialize();
   void analyse(uint32_t wakeUpIntervalInSec);
   void close();

private:
  typedef std::vector<virDomainMemoryStatStruct> DomainMemoryStats_t;

  void coordinateMemoryRequirements();

  void getDomainMemoryInfo();
  void getHostMemoryInfo();

  void getDomainMemoryStats(Domain& domain);
  void getDomainMaxMemory(Domain& domain);
  void getHostMemoryStats();

  void handleMemoryDeficienyWithoutReclaiming();
  void reclaimMemoryInactiveVms();
  void reclaimMemoryPreviouslyActiveVms();

  void increaseMemory(Domain& domain);
  bool isDomainActivelyConsumingMemory(Domain& domain);
  bool wasDomainPreviouslyActiveAndInactiveNow(Domain& domain);
  bool isMemoryDeficient(Domain& domain);
  bool canIncreaseWithoutReclaiming(Domain& domain);
  void checkAndMarkIfActiveDomain(Domain& domain);
  void checkAndMarkIfInactiveDomain(Domain& domain);
  bool isDomainReclaimed(Domain* domain);
  bool canReclaimMemory(Domain& currentDomain);

  unsigned long long getIncreaseValue() const;
  unsigned long long getDecreaseValue() const;
  unsigned long long extractDomainFreeMemoryInMB(Domain& domain, DomainMemoryStats_t& domainMemoryStats);
  void updateMemoryStatsCollectionFrequency(int memoryStatsPeriod);

  unsigned long long thresholdFreeMemory_;
  unsigned long long increaseMemoryThreshold_;
  unsigned long long reclaimMemoryThreshold_;
  unsigned long long activeMemoryConsumptionThreshold_;
  int memoryStatsPeriod_;
  uint32_t wakeUpPeriod_;

  std::map<Domain*, uint32_t> reclaimedDomains_;

};
#endif //IMPL_MEMORYCOORDINATOR_H
