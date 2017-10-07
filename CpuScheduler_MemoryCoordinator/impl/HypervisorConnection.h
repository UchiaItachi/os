//
// Created by itachi on 9/13/17.
//
#pragma once

#include "Domain.h"
#include "Host.h"

#include <libvirt/libvirt.h>

#include <map>

class HypervisorConnection
{
public:
  HypervisorConnection();

  void open();
  void close();
  void getUsageOfAllDomains();
  void updateUnusedMemoryAtStartOfAllDomains();
  void printInitInfo();

protected:
  void initDomains();
  void initHost();
  void getDomains();

  void initConnection();
  void closeConnection();

  virConnectPtr connection_;

  std::map<uint32_t, Domain*> domains_;
  Host *host_;

  uint32_t numOfActiveDomains_;
  uint32_t numOfCpusOnHost_;
  uint32_t numOfVCpus_;

  static const std::string HYPERVISOR_HOST;
};
