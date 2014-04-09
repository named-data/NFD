#include "nrd-config.hpp"

namespace ndn {
namespace nrd {

NrdConfig::NrdConfig()
  : m_isSecuritySectionDefined(false)
{
}

void
NrdConfig::load(const std::string& filename)
{
  std::ifstream inputFile;
  inputFile.open(filename.c_str());
  if (!inputFile.good() || !inputFile.is_open())
  {
    std::string msg = "Failed to read configuration file: ";
    msg += filename;
    std::cerr << filename << std::endl;
    throw Error(msg);
  }
  load(inputFile, filename);
  inputFile.close();
}

void
NrdConfig::load(const std::string& input, const std::string& filename)
{
  std::istringstream inputStream(input);
  load(inputStream, filename);
}

void
NrdConfig::load(std::istream& input, const std::string& filename)
{
  BOOST_ASSERT(!filename.empty());

  ConfigSection ptree;
  try
  {
    boost::property_tree::read_info(input, ptree);
  }
  catch (const boost::property_tree::info_parser_error& error)
  {
    std::stringstream msg;
    msg << "Failed to parse configuration file";
    msg << " " << filename;
    msg << " " << error.message() << " line " << error.line();
    throw Error(msg.str());
  }
  process(ptree, filename);
}

void
NrdConfig::process(const ConfigSection& configSection,
                   const std::string& filename)
{
  BOOST_ASSERT(!filename.empty());

  if (configSection.begin() == configSection.end())
    {
      std::string msg = "Error processing configuration file";
      msg += ": ";
      msg += filename;
      msg += " no data";
      throw Error(msg);
    }

  for (ConfigSection::const_iterator i = configSection.begin();
       i != configSection.end(); ++i)
    {
      const std::string& sectionName = i->first;
      const ConfigSection& section = i->second;

      if (boost::iequals(sectionName, "security"))
        {
          onSectionSecurity(section, filename);
        }
      //Add more sections here as needed
      //else if (boost::iequals(sectionName, "another-section"))
        //{
          //onSectionAnotherSection(section, filename);
        //}
      else
        {
          std::string msg = "Error processing configuration file";
          msg += " ";
          msg += filename;
          msg += " unrecognized section: " + sectionName;
          throw Error(msg);
        }
    }
}

void
NrdConfig::onSectionSecurity(const ConfigSection& section,
                             const std::string& filename)
{
  if (!m_isSecuritySectionDefined) {
    //setSecturitySection(section);
    m_securitySection = section;
    m_filename = filename;
    m_isSecuritySectionDefined = true;
  }
  else {
    std::string msg = "Error processing configuration file";
    msg += " ";
    msg += filename;
    msg += " security section can appear only once";
    throw Error(msg);
  }
}

} //namespace nrd
} //namespace ndn
