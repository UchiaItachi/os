//
// Created by itachi on 9/10/17.
//

#pragma once

#include "HypervisorConnection.h"

#include <libvirt/libvirt.h>

#include <map>
#include <vector>
#include <iostream>
#include <string>

class CpuScheduler: public HypervisorConnection
{
public:
    CpuScheduler();
    ~CpuScheduler() = default;
    CpuScheduler(CpuScheduler&) = delete;

    void initialize();
    void schedule(uint32_t timeIntervalInSec);
    void close();

private:

    typedef std::vector<VCpuInfo*> VCpuInfoList_t;
    void printInitializationInfo();
    // Top level api's
    void getStatisticsOfDomain();
    void getStatisticsOfHost();


  // Host related info
    void getHostCpuUsage();
    void getNumberOfHostCpus();

    // Debug outs
    void printDomainCpuStatistics();
    void printHostCpuStatistics();

    // Helpers
    void extractCpuTime(Domain *dom, const std::vector<virTypedParameter>& params);

    void calculateHostCpuUsage();

    void pinCpus();

    bool arePCpusBalanced(unsigned long long balancedCpuTime);
    unsigned long long getBalancedPCpuUsage();


    std::map<Host::HostCpuId_t, VCpuInfoList_t> cpuPinMapping_;
    uint32_t scheduleInterval_;
    const uint32_t checkFrequency_;
    uint32_t currentSchedule_;
    uint32_t toleranceLimit_;
};
