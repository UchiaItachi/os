//
// Created by itachi on 9/13/17.
//

#pragma once

#include "PCpuInfo.h"
#include "utils.h"

#include <libvirt/libvirt.h>

#include <vector>
#include <map>
#include <cstdint>

class Domain;

class Host
{
public:
  Host(virConnectPtr connection, std::map<uint32_t, Domain*>& domains);

  unsigned long long currentCpuUsage();
  unsigned long long getFreeMemory();
  void currentCpuUsageOfPCpus();
  uint32_t getNumberOfHostCpus();
  void extractHostCpuUtilization(std::vector<virNodeCPUStats>&, unsigned long long& cpuUtil);
  bool arePCpusBalanced(unsigned long long lowLimit, unsigned long long highLimit) const;
  typedef uint32_t HostCpuId_t;


  unsigned long long hostCpuTime;
  unsigned long long cpuUsageFraction;
  uint32_t numOfCpusOnHost_;
  virConnectPtr connection_;
  std::map<HostCpuId_t, PCpuInfo> pCpuInfo_;
  unsigned long long freeMemory_;
  unsigned long long maxMemory_;

private:
  unsigned long long currentCpuUsage_1();
  unsigned long long currentCpuUsage_2();
  std::map<uint32_t, Domain*>& domains_;
};