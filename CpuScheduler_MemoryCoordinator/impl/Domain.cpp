//
// Created by itachi on 9/16/17.
//

#include "Domain.h"
#include "HypervisorConnection.h"

#include <cstring>
#include <cassert>

Domain::Domain(virConnectPtr connection, virDomainPtr dom)
  : domain(dom)
  , domainName_("")
  , numberOfVCpus_(0)
  , cpuUsage(0)
  , cpuUsageFraction(0)
  , vCpuInfos_()
  , memInfo_()
  , maxMemory_(0)
  , previousUnsedMemory_(0)
  , currentlyActive_(false)
  , previouslyActive_(false)
  , connection_(connection)
{
}

uint32_t Domain::getNumberOfVCpus()
{
  struct _virDomainInfo info;
  int result = virDomainGetInfo(domain, &info);
  if (result != -1)
  {
    numberOfVCpus_ = info.nrVirtCpu;
    vCpuInfos_.resize(info.nrVirtCpu);
  } else
  {
    std::cout << "failed to get the no. of virt cpus for domain " << domainName_ << std::endl;
  }
  return numberOfVCpus_;
}


unsigned long long Domain::currentCpuUsage()
{
  unsigned long long now = 0;

#if 0
  now = currentCpuUsage_1();
  cpuUsageFraction = convertNanoSecondsToSeconds(now - cpuUsage);
  cpuUsage = now;
#endif

#if 1
  now = currentCpuUsage_2();
  cpuUsageFraction = convertNanoSecondsToSeconds(now - cpuUsage);
  cpuUsage = now;
#endif

#if 0
  now = currentCpuUsage_3();
  cpuUsageFraction = convertNanoSecondsToSeconds(now - cpuUsage);
  cpuUsage = now;
#endif

  return now;
}

void Domain::currentCpuUsageOfVCpus()
{
  currentCpuUsage_2(); // updates the individual vCpu info
}

unsigned long long Domain::currentCpuUsage_1()
{
  unsigned long long totalCpuUsage = 0;
  if (domain == nullptr)
  {
    std::cout << "Domain is null for domain:" << domainName_ << std::endl;
    assert(false);
  }
  int ncpus = virDomainGetCPUStats(domain, NULL, 0, 0, 0, 0); // ncpus
  if (ncpus == -1)
  {
    std::cout << std::endl << "Error: failed to get total cpu usage of the domain: ncpus" << std::endl;
  }
  int nparams = virDomainGetCPUStats(domain, NULL, 0, 0, 1, 0); // nparams
  if (nparams == -1)
  {
    std::cout << std::endl << "Error: failed to get total cpu usage of the domain: nparams" << std::endl;
  }
  std::vector<virTypedParameter> params(nparams * ncpus);
  int ret = virDomainGetCPUStats(domain, params.data(), nparams, 0, ncpus, 0); // per-cpu stats
  if (ret == -1)
  {
    std::cout << std::endl << "Error: failed to get total cpu usage of the domain: stats" << std::endl;
  }

  for (uint32_t vCpu = 0; static_cast<int>(vCpu) < ncpus; ++vCpu)
  {
    std::vector<virTypedParameter> param(params.begin(), params.begin() + nparams);
    vCpuInfos_[vCpu].vCpuUsage = extractCpuTime(param);
    totalCpuUsage += vCpuInfos_[vCpu].vCpuUsage;
    params.erase(params.begin(), params.begin() + nparams);
  }
  return totalCpuUsage;
}

unsigned long long Domain::currentCpuUsage_2()
{
  unsigned long long totalCpuUsage = 0;

  std::vector<virVcpuInfo> nowVCpuUsage(numberOfVCpus_);
  int ret = virDomainGetVcpus(domain, nowVCpuUsage.data(), numberOfVCpus_, nullptr, 0);

  if (static_cast<unsigned int>(ret) != numberOfVCpus_)
  {
    std::cout << std::endl << "Error: failed to get vCpu info : method 2 " << std::endl;
    return 0;
  }

  for (auto& cpuInfo: nowVCpuUsage)
  {
    VCpuInfo &info = vCpuInfos_[cpuInfo.number];
    info.vCpuUsageFraction = convertNanoSecondsToSeconds(cpuInfo.cpuTime - info.vCpuUsage);
    info.vCpuUsage = cpuInfo.cpuTime;
    info.domainInfo = this;
    info.vCpuNumber = cpuInfo.number;
    info.pCpuNumber = cpuInfo.cpu;
    totalCpuUsage += info.vCpuUsage;
  }
  return totalCpuUsage;
}

unsigned long long Domain::currentCpuUsage_3()
{
  unsigned long long totalCpuUsage = 0;

  int nparams = virDomainGetCPUStats(domain, NULL, 0, -1, 1, 0); // nparams
  if (nparams == -1)
  {
    std::cout << std::endl << "Error: failed to get total cpu usage of the domain: nparams" << std::endl;
  }
  std::vector<virTypedParameter> params(nparams);
  int ret = virDomainGetCPUStats(domain, params.data(), nparams, -1, 1, 0); // per-cpu stats
  if (ret == -1)
  {
    std::cout << std::endl << "Error: failed to get total cpu usage of the domain: stats" << std::endl;
  }

  totalCpuUsage = extractCpuTime(params);
  return totalCpuUsage;
}

void Domain::currentCpuAffinity()
{
  std::vector<unsigned char> cpumaps(numberOfVCpus_);
  int nVCpus = virDomainGetVcpuPinInfo(domain, numberOfVCpus_, cpumaps.data(), sizeof(unsigned char),
                                       VIR_DOMAIN_AFFECT_LIVE);
  if (static_cast<unsigned int>(nVCpus) == numberOfVCpus_)
  {
    for (unsigned long i = 0; i < numberOfVCpus_; ++i)
    {
      vCpuInfos_[i].cpuAffinity.c = cpumaps[i];
    }
  } else
  {
    std::cout << "Error: didn't find the vCpu affinity for domain :" << domainName_ << std::endl;
  }
}

unsigned long long Domain::extractCpuTime(const std::vector<virTypedParameter>& params)
{
  unsigned long long cpuUsage = 0;
  for (const auto &pa: params)
  {
    if (std::strcmp(pa.field, VIR_DOMAIN_CPU_STATS_VCPUTIME) == 0)
    {
      if (VIR_TYPED_PARAM_ULLONG == pa.type)
      {
        cpuUsage = pa.value.ul;
        return cpuUsage;
      }
    }
  }
  std::cout << "Error: Unhandled parameter type in domain cpu time (only ull used)" << std::endl;
  return cpuUsage;
}
