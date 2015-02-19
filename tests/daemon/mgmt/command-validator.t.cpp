/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mgmt/command-validator.hpp"
#include "core/config-file.hpp"

#include "tests/test-common.hpp"

#include <ndn-cxx/util/command-interest-generator.hpp>
#include <ndn-cxx/util/io.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

namespace nfd {
namespace tests {

NFD_LOG_INIT("CommandValidatorTest");

BOOST_FIXTURE_TEST_SUITE(MgmtCommandValidator, BaseFixture)

// authorizations
// {
//   authorize
//   {
//     certfile "tests/daemon/mgmt/cert1.ndncert"
//     privileges
//     {
//       fib
//       stats
//     }
//   }

//   authorize
//   {
//     certfile "tests/daemon/mgmt/cert2.ndncert"
//     privileges
//     {
//       faces
//     }
//   }
// }

const std::string CONFIG =
"authorizations\n"
"{\n"
"  authorize\n"
"  {\n"
"    certfile \"tests/daemon/mgmt/cert1.ndncert\"\n"
"    privileges\n"
"    {\n"
"      fib\n"
"      stats\n"
"    }\n"
"  }\n"
"  authorize\n"
"  {\n"
"    certfile \"tests/daemon/mgmt/cert2.ndncert\"\n"
"    privileges\n"
"    {\n"
"      faces\n"
"    }\n"
"  }\n"
  "}\n";

const boost::filesystem::path CONFIG_PATH =
  boost::filesystem::current_path() /= std::string("unit-test-nfd.conf");

class CommandValidatorTester
{
public:

  CommandValidatorTester()
    : m_validated(false),
      m_validationFailed(false)
  {

  }

  void
  generateIdentity(const Name& prefix)
  {
    m_identityName = prefix;
    m_identityName.appendVersion();

    const Name certName = m_keys.createIdentity(m_identityName);

    m_certificate = m_keys.getCertificate(certName);
  }

  void
  saveIdentityToFile(const char* filename)
  {
    std::ofstream out;
    out.open(filename);

    BOOST_REQUIRE(out.is_open());
    BOOST_REQUIRE(static_cast<bool>(m_certificate));

    ndn::io::save<ndn::IdentityCertificate>(*m_certificate, out);

    out.close();
  }

  const Name&
  getIdentityName() const
  {
    BOOST_REQUIRE_NE(m_identityName, Name());
    return m_identityName;
  }

  const Name&
  getPublicKeyName() const
  {
    BOOST_REQUIRE(static_cast<bool>(m_certificate));
    return m_certificate->getPublicKeyName();
  }

  void
  onValidated(const shared_ptr<const Interest>& interest)
  {
    // NFD_LOG_DEBUG("validated command");
    m_validated = true;
  }

  void
  onValidationFailed(const shared_ptr<const Interest>& interest, const std::string& info)
  {
    NFD_LOG_DEBUG("validation failed: " << info);
    m_validationFailed = true;
  }

  bool
  commandValidated() const
  {
    return m_validated;
  }

  bool
  commandValidationFailed() const
  {
    return m_validationFailed;
  }

  void
  resetValidation()
  {
    m_validated = false;
    m_validationFailed = false;
  }

  ~CommandValidatorTester()
  {
    m_keys.deleteIdentity(m_identityName);
  }

private:
  bool m_validated;
  bool m_validationFailed;

  ndn::KeyChain m_keys;
  Name m_identityName;
  shared_ptr<ndn::IdentityCertificate> m_certificate;
};

class TwoValidatorFixture : public BaseFixture
{
public:
  TwoValidatorFixture()
  {
    m_tester1.generateIdentity("/test/CommandValidator/TwoKeys/id1");
    m_tester1.saveIdentityToFile("tests/daemon/mgmt/cert1.ndncert");

    m_tester2.generateIdentity("/test/CommandValidator/TwoKeys/id2");
    m_tester2.saveIdentityToFile("tests/daemon/mgmt/cert2.ndncert");
  }

  ~TwoValidatorFixture()
  {
    boost::system::error_code error;
    boost::filesystem::remove("tests/daemon/mgmt/cert1.ndncert", error);
    boost::filesystem::remove("tests/daemon/mgmt/cert2.ndncert", error);
  }

protected:
  CommandValidatorTester m_tester1;
  CommandValidatorTester m_tester2;
};

BOOST_FIXTURE_TEST_CASE(TwoKeys, TwoValidatorFixture)
{
  shared_ptr<Interest> fibCommand = make_shared<Interest>("/localhost/nfd/fib/insert");
  shared_ptr<Interest> statsCommand = make_shared<Interest>("/localhost/nfd/stats/dosomething");
  shared_ptr<Interest> facesCommand = make_shared<Interest>("/localhost/nfd/faces/create");

  ndn::CommandInterestGenerator generator;
  generator.generateWithIdentity(*fibCommand, m_tester1.getIdentityName());
  generator.generateWithIdentity(*statsCommand, m_tester1.getIdentityName());
  generator.generateWithIdentity(*facesCommand, m_tester2.getIdentityName());

  ConfigFile config;
  CommandValidator validator;
  validator.addSupportedPrivilege("faces");
  validator.addSupportedPrivilege("fib");
  validator.addSupportedPrivilege("stats");

  validator.setConfigFile(config);

  config.parse(CONFIG, false, CONFIG_PATH.native());

  validator.validate(*fibCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester1, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester1, _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidated());
  m_tester1.resetValidation();

  validator.validate(*statsCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester1, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester1, _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidated());

  validator.validate(*facesCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester2, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester2, _1, _2));

  BOOST_REQUIRE(m_tester2.commandValidated());
  m_tester2.resetValidation();

  // use cert2 for fib command (authorized for cert1 only)
  shared_ptr<Interest> unauthorizedFibCommand = make_shared<Interest>("/localhost/nfd/fib/insert");
  generator.generateWithIdentity(*unauthorizedFibCommand, m_tester2.getIdentityName());

  validator.validate(*unauthorizedFibCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester2, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester2, _1, _2));

  BOOST_REQUIRE(m_tester2.commandValidationFailed());
}

BOOST_FIXTURE_TEST_CASE(TwoKeysDryRun, TwoValidatorFixture)
{
  CommandValidatorTester tester1;
  tester1.generateIdentity("/test/CommandValidator/TwoKeys/id1");
  tester1.saveIdentityToFile("tests/daemon/mgmt/cert1.ndncert");

  CommandValidatorTester tester2;
  tester2.generateIdentity("/test/CommandValidator/TwoKeys/id2");
  tester2.saveIdentityToFile("tests/daemon/mgmt/cert2.ndncert");

  shared_ptr<Interest> fibCommand = make_shared<Interest>("/localhost/nfd/fib/insert");
  shared_ptr<Interest> statsCommand = make_shared<Interest>("/localhost/nfd/stats/dosomething");
  shared_ptr<Interest> facesCommand = make_shared<Interest>("/localhost/nfd/faces/create");

  ndn::CommandInterestGenerator generator;
  generator.generateWithIdentity(*fibCommand, m_tester1.getIdentityName());
  generator.generateWithIdentity(*statsCommand, m_tester1.getIdentityName());
  generator.generateWithIdentity(*facesCommand, m_tester2.getIdentityName());

  ConfigFile config;
  CommandValidator validator;
  validator.addSupportedPrivilege("faces");
  validator.addSupportedPrivilege("fib");
  validator.addSupportedPrivilege("stats");

  validator.setConfigFile(config);

  config.parse(CONFIG, true, CONFIG_PATH.native());

  validator.validate(*fibCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester1, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester1, _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidationFailed());
  m_tester1.resetValidation();

  validator.validate(*statsCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester1, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester1, _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidationFailed());

  validator.validate(*facesCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester2, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester2, _1, _2));

  BOOST_REQUIRE(m_tester2.commandValidationFailed());
  m_tester2.resetValidation();

  // use cert2 for fib command (authorized for cert1 only)
  shared_ptr<Interest> unauthorizedFibCommand = make_shared<Interest>("/localhost/nfd/fib/insert");
  generator.generateWithIdentity(*unauthorizedFibCommand, m_tester2.getIdentityName());

  validator.validate(*unauthorizedFibCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester2, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester2, _1, _2));

  BOOST_REQUIRE(m_tester2.commandValidationFailed());
}

BOOST_AUTO_TEST_CASE(NoAuthorizeSections)
{
  const std::string NO_AUTHORIZE_CONFIG =
    "authorizations\n"
    "{\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);
  BOOST_CHECK_THROW(config.parse(NO_AUTHORIZE_CONFIG, false, CONFIG_PATH.native()), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(NoPrivilegesSections)
{
  const std::string NO_PRIVILEGES_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/cert1.ndncert\"\n"
    "  }\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);

  BOOST_CHECK_THROW(config.parse(NO_PRIVILEGES_CONFIG, false, CONFIG_PATH.native()), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(InvalidCertfile)
{
  const std::string INVALID_CERT_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/notacertfile.ndncert\"\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);
  BOOST_CHECK_THROW(config.parse(INVALID_CERT_CONFIG, false, CONFIG_PATH.native()), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(NoCertfile)
{
  const std::string NO_CERT_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "}\n";


  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);
  BOOST_CHECK_THROW(config.parse(NO_CERT_CONFIG, false, CONFIG_PATH.native()), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(MalformedCert)
{
    const std::string MALFORMED_CERT_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/malformed.ndncert\"\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "}\n";


  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);
  BOOST_CHECK_THROW(config.parse(MALFORMED_CERT_CONFIG, false, CONFIG_PATH.native()), ConfigFile::Error);
}

bool
validateErrorMessage(const std::string& expectedMessage, const ConfigFile::Error& error)
{
  bool gotExpected = error.what() == expectedMessage;
  if (!gotExpected)
    {
      NFD_LOG_WARN("\ncaught exception: " << error.what()
                    << "\n\nexpected exception: " << expectedMessage);
    }
  return gotExpected;
}

BOOST_AUTO_TEST_CASE(NoAuthorizeSectionsDryRun)
{
  const std::string NO_AUTHORIZE_CONFIG =
    "authorizations\n"
    "{\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);
  BOOST_CHECK_EXCEPTION(config.parse(NO_AUTHORIZE_CONFIG, true, CONFIG_PATH.native()),
                        ConfigFile::Error,
                        bind(&validateErrorMessage,
                             "No authorize sections found", _1));
}

BOOST_FIXTURE_TEST_CASE(NoPrivilegesSectionsDryRun, TwoValidatorFixture)
{
  const std::string NO_PRIVILEGES_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/cert1.ndncert\"\n"
    "  }\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/cert2.ndncert\"\n"
    "  }\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);

  std::stringstream expectedError;
  expectedError << "No privileges section found for certificate file tests/daemon/mgmt/cert1.ndncert "
                << "(" << m_tester1.getPublicKeyName().toUri() << ")\n"
                << "No privileges section found for certificate file tests/daemon/mgmt/cert2.ndncert "
                << "(" << m_tester2.getPublicKeyName().toUri() << ")";

  BOOST_CHECK_EXCEPTION(config.parse(NO_PRIVILEGES_CONFIG, true, CONFIG_PATH.native()),
                        ConfigFile::Error,
                        bind(&validateErrorMessage, expectedError.str(), _1));
}

BOOST_AUTO_TEST_CASE(InvalidCertfileDryRun)
{
  using namespace boost::filesystem;

  const std::string INVALID_KEY_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/notacertfile.ndncert\"\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/stillnotacertfile.ndncert\"\n"
    "    privileges\n"
    "    {\n"
    "    }\n"
    "  }\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);

  std::stringstream error;
  error << "Unable to open certificate file "
        << absolute("tests/daemon/mgmt/notacertfile.ndncert").native() << "\n"
        << "Unable to open certificate file "
        << absolute("tests/daemon/mgmt/stillnotacertfile.ndncert").native();

  BOOST_CHECK_EXCEPTION(config.parse(INVALID_KEY_CONFIG, true, CONFIG_PATH.native()),
                        ConfigFile::Error,
                        bind(&validateErrorMessage, error.str(), _1));
}

BOOST_AUTO_TEST_CASE(NoCertfileDryRun)
{
  const std::string NO_CERT_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "  authorize\n"
    "  {\n"
    "  }\n"
    "}\n";


  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);
  BOOST_CHECK_EXCEPTION(config.parse(NO_CERT_CONFIG, true, CONFIG_PATH.native()),
                        ConfigFile::Error,
                        bind(&validateErrorMessage,
                             "No certfile specified\n"
                             "No certfile specified", _1));
}

BOOST_AUTO_TEST_CASE(MalformedCertDryRun)
{
  using namespace boost::filesystem;

  const std::string MALFORMED_CERT_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/malformed.ndncert\"\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "  authorize\n"
    "  {\n"
    "    certfile \"tests/daemon/mgmt/malformed.ndncert\"\n"
    "  }\n"
    "}\n";


  ConfigFile config;
  CommandValidator validator;

  validator.setConfigFile(config);

  std::stringstream error;
  error << "Malformed certificate file "
        << absolute("tests/daemon/mgmt/malformed.ndncert").native() << "\n"
        << "Malformed certificate file "
        << absolute("tests/daemon/mgmt/malformed.ndncert").native();

  BOOST_CHECK_EXCEPTION(config.parse(MALFORMED_CERT_CONFIG, true, CONFIG_PATH.native()),
                        ConfigFile::Error,
                        bind(&validateErrorMessage, error.str(), _1));
}

BOOST_FIXTURE_TEST_CASE(Wildcard, TwoValidatorFixture)
{
  const std::string WILDCARD_CERT_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    certfile any\n"
    "    privileges\n"
    "    {\n"
    "      faces\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "}\n";

  shared_ptr<Interest> fibCommand = make_shared<Interest>("/localhost/nfd/fib/insert");
  shared_ptr<Interest> statsCommand = make_shared<Interest>("/localhost/nfd/stats/dosomething");
  shared_ptr<Interest> facesCommand = make_shared<Interest>("/localhost/nfd/faces/create");

  ndn::CommandInterestGenerator generator;
  generator.generateWithIdentity(*fibCommand, m_tester1.getIdentityName());
  generator.generateWithIdentity(*statsCommand, m_tester1.getIdentityName());
  generator.generateWithIdentity(*facesCommand, m_tester1.getIdentityName());

  ConfigFile config;
  CommandValidator validator;
  validator.addSupportedPrivilege("faces");
  validator.addSupportedPrivilege("fib");
  validator.addSupportedPrivilege("stats");

  validator.setConfigFile(config);

  config.parse(WILDCARD_CERT_CONFIG, false, CONFIG_PATH.native());

  validator.validate(*fibCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester1, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester1, _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidationFailed());
  m_tester1.resetValidation();

  validator.validate(*statsCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester1, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester1, _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidated());
  m_tester1.resetValidation();

  validator.validate(*facesCommand,
                     bind(&CommandValidatorTester::onValidated, &m_tester1, _1),
                     bind(&CommandValidatorTester::onValidationFailed, &m_tester1, _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidated());
  m_tester1.resetValidation();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
