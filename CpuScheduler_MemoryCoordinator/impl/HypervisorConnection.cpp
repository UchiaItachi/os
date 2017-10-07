//
// Created by itachi on 9/13/17.
//

#include "HypervisorConnection.h"

const std::string HypervisorConnection::HYPERVISOR_HOST = "qemu:///system";

HypervisorConnection::HypervisorConnection()
  : connection_(nullptr)
  , domains_()
  , host_(nullptr)
  , numOfActiveDomains_(0)
  , numOfCpusOnHost_(0)
  , numOfVCpus_(0)
{
}

void HypervisorConnection::open()
{
  initConnection();
  initDomains();
  initHost();
}

void HypervisorConnection::close()
{
  for (auto& domain: domains_)
  {
    delete domain.second;
  }
  domains_.clear();
  delete host_;
  host_ = nullptr;

  closeConnection();
}

void HypervisorConnection::initConnection()
{
  connection_ = virConnectOpen(HYPERVISOR_HOST.c_str());
  if (connection_ == nullptr)
  {
    std::cout << "Couldn't connect to the Hypervisor host : " << HYPERVISOR_HOST << std::endl;
  }
  else
  {
    std::cout << " Connected to the Hypervisor host : " << HYPERVISOR_HOST << std::endl;
  }
}


void HypervisorConnection::initDomains()
{
  getDomains();
  for (auto& domain: domains_)
  {
    numOfVCpus_ += domain.second->getNumberOfVCpus();
  }
  numOfActiveDomains_ = domains_.size();
}

void HypervisorConnection::initHost()
{
  host_ = new Host(connection_, domains_);
  numOfCpusOnHost_ = host_->getNumberOfHostCpus();
}

void HypervisorConnection::printInitInfo()
{
  std::cout << "num_of_pcpus = " << numOfCpusOnHost_ << " " << "num_of_vcpus = " << numOfVCpus_ << " " << "num_of_domains = " << numOfActiveDomains_ << std::endl;

  std::cout << "domains { ";
  for (auto& domain: domains_)
  {
    std::cout << domain.second->domainName_ << " , ";
  }
  std::cout << " }" << std::endl;
}


void HypervisorConnection::closeConnection()
{
  virConnectClose(connection_);
}

void HypervisorConnection::getDomains()
{
  // get the number of active domains
  int num = 0;
  num = virConnectNumOfDomains(connection_);
  if (num == -1)
  {
    std::cout << std::endl << "Error: no active domains found" << std::endl;
    return;
  }
  numOfActiveDomains_ = num;
  std::vector<int> ids(numOfActiveDomains_);

  // get the unique id of each of the domain
  int numOfDomainsFound = virConnectListDomains(connection_, ids.data(), numOfActiveDomains_);
  if (numOfDomainsFound != static_cast<int>(numOfActiveDomains_))
  {
    std::cout << "Failed to get list of all active domains " << std::endl;
    return;
  }

  // get the domain handle for each domain and the domain name
  for (auto id: ids)
  {
    virDomainPtr dom = virDomainLookupByID(connection_, id);
    if (!dom)
    {
      std::cout << std::endl << "Error: didn't find the domain with id : " << id << std::endl;
    }
    else
    {
      Domain *info = new Domain(connection_, dom);
      domains_[id] = info;
      const char *name = virDomainGetName(dom);
      if (name)
      {
        info->domainName_ = name;
      }
    }
  }
}

void HypervisorConnection::getUsageOfAllDomains()
{
  for (auto& domain: domains_)
  {
    domain.second->currentCpuUsage();
  }
}

void HypervisorConnection::updateUnusedMemoryAtStartOfAllDomains()
{
  for (auto& domain: domains_)
  {
    domain.second->previousUnsedMemory_ = domain.second->memInfo_[VIR_DOMAIN_MEMORY_STAT_UNUSED];
  }
}
