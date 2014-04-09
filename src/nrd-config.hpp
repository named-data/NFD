#ifndef NRD_CONFIG_HPP
#define NRD_CONFIG_HPP

#include "common.hpp"

namespace ndn {
namespace nrd {

typedef boost::property_tree::ptree ConfigSection;

class NrdConfig
{

public:
  NrdConfig();

  virtual
  ~NrdConfig()
  {
  }

  void
  load(const std::string& filename);

  void
  load(const std::string& input, const std::string& filename);

  void
  load(std::istream& input, const std::string& filename);

  const ConfigSection&
  getSecuritySection() const
  {
    return m_securitySection;
  }

private:
  void
  process(const ConfigSection& configSection,
          const std::string& filename);

  void
  onSectionSecurity(const ConfigSection& section,
                    const std::string& filename);

  const void
  setSecturitySection(const ConfigSection& securitySection)
  {
    m_securitySection = securitySection;
  }

private:
  bool m_isSecuritySectionDefined;
  ConfigSection m_securitySection;
  std::string m_filename;
};

}//namespace nrd
}//namespace ndn

#endif // NRD_CONFIG_HPP
