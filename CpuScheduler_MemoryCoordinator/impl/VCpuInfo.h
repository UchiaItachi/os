//
// Created by itachi on 9/16/17.
//

#pragma once

#include <cstdint>

class Domain;

struct VCpuInfo
{
  VCpuInfo(Domain* domain)
    : vCpuNumber(0)
    , pCpuNumber(0)
    , cpuAffinity{0}
    , domainInfo(domain)
    , vCpuUsageFraction(0)
    , vCpuUsage(0)
  {

  }

  VCpuInfo()
    : VCpuInfo(nullptr)
  {

  }

  uint32_t vCpuNumber;
  uint32_t pCpuNumber;

  union
  {
    uint32_t u;
    unsigned char c;
  } cpuAffinity;

  Domain* domainInfo;
  unsigned long long vCpuUsageFraction;
  unsigned long long vCpuUsage;
};
