//
// Created by itachi on 9/13/17.
//

#pragma once

#include "VCpuInfo.h"
#include "utils.h"
#include "DomainMemoryInfo.h"

#include <libvirt/libvirt.h>

#include <vector>
#include <iostream>
#include <string>
#include <map>

class Domain
{
public:
  Domain(virConnectPtr connection, virDomainPtr domain);

  unsigned long long currentCpuUsage();
  unsigned long long getFreeMemory() const;
  void currentCpuUsageOfVCpus();
  void currentCpuAffinity();
  uint32_t getNumberOfVCpus();

  virDomainPtr domain;
  std::string domainName_;
  unsigned int numberOfVCpus_;
  unsigned long long cpuUsage;
  unsigned long long cpuUsageFraction;
  std::vector<VCpuInfo> vCpuInfos_;
  std::map<virDomainMemoryStatTags, unsigned long long> memInfo_;
  unsigned long long maxMemory_;
  unsigned long long previousUnsedMemory_;
  bool currentlyActive_;
  bool previouslyActive_;
private:
  unsigned long long extractCpuTime(const std::vector<virTypedParameter>& params);

  unsigned long long currentCpuUsage_1();
  unsigned long long currentCpuUsage_2();
  unsigned long long currentCpuUsage_3();

  unsigned long long getVCpuTime();
  void getDomainCpuAffinity();
  virConnectPtr connection_;
};
