/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/command-validator.hpp"
#include "mgmt/config-file.hpp"

#include "tests/test-common.hpp"

#include <boost/test/unit_test.hpp>
#include <ndn-cpp-dev/util/command-interest-generator.hpp>
#include <ndn-cpp-dev/util/io.hpp>
#include <boost/filesystem.hpp>

namespace nfd {

namespace tests {

NFD_LOG_INIT("CommandValidatorTest");

BOOST_FIXTURE_TEST_SUITE(MgmtCommandValidator, BaseFixture)

// authorizations
// {
//   ; an authorize section grants privileges to a key
//   authorize
//   {
//     keyfile "tests/mgmt/key1.pub" ; public key file
//     privileges ; set of privileges granted to this public key
//     {
//       fib
//       stats
//     }
//   }

//   authorize
//   {
//     keyfile "tests/mgmt/key2.pub" ; public key file
//     privileges ; set of privileges granted to this public key
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
"    keyfile \"tests/mgmt/key1.pub\"\n"
"    privileges\n"
"    {\n"
"      fib\n"
"      stats\n"
"    }\n"
"  }\n"
"  authorize\n"
"  {\n"
"    keyfile \"tests/mgmt/key2.pub\"\n"
"    privileges\n"
"    {\n"
"      faces\n"
"    }\n"
"  }\n"
  "}\n";

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
    m_identityName.append(boost::lexical_cast<std::string>(ndn::time::now()));

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
    m_tester1.saveIdentityToFile("tests/mgmt/key1.pub");

    m_tester2.generateIdentity("/test/CommandValidator/TwoKeys/id2");
    m_tester2.saveIdentityToFile("tests/mgmt/key2.pub");
  }

  ~TwoValidatorFixture()
  {
    boost::system::error_code error;
    boost::filesystem::remove("tests/mgmt/key1.pub", error);
    boost::filesystem::remove("tests/mgmt/key2.pub", error);
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

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  config.parse(CONFIG, false, "dummy-config");

  validator.validate(*fibCommand,
                     bind(&CommandValidatorTester::onValidated, boost::ref(m_tester1), _1),
                     bind(&CommandValidatorTester::onValidationFailed, boost::ref(m_tester1), _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidated());
  m_tester1.resetValidation();

  validator.validate(*statsCommand,
                     bind(&CommandValidatorTester::onValidated, boost::ref(m_tester1), _1),
                     bind(&CommandValidatorTester::onValidationFailed, boost::ref(m_tester1), _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidated());

  validator.validate(*facesCommand,
                     bind(&CommandValidatorTester::onValidated, boost::ref(m_tester2), _1),
                     bind(&CommandValidatorTester::onValidationFailed, boost::ref(m_tester2), _1, _2));

  BOOST_REQUIRE(m_tester2.commandValidated());
  m_tester2.resetValidation();

  // use key2 for fib command (authorized for key1 only)
  shared_ptr<Interest> unauthorizedFibCommand = make_shared<Interest>("/localhost/nfd/fib/insert");
  generator.generateWithIdentity(*unauthorizedFibCommand, m_tester2.getIdentityName());

  validator.validate(*unauthorizedFibCommand,
                     bind(&CommandValidatorTester::onValidated, boost::ref(m_tester2), _1),
                     bind(&CommandValidatorTester::onValidationFailed, boost::ref(m_tester2), _1, _2));

  BOOST_REQUIRE(m_tester2.commandValidationFailed());
}

BOOST_FIXTURE_TEST_CASE(TwoKeysDryRun, TwoValidatorFixture)
{
  CommandValidatorTester tester1;
  tester1.generateIdentity("/test/CommandValidator/TwoKeys/id1");
  tester1.saveIdentityToFile("tests/mgmt/key1.pub");

  CommandValidatorTester tester2;
  tester2.generateIdentity("/test/CommandValidator/TwoKeys/id2");
  tester2.saveIdentityToFile("tests/mgmt/key2.pub");

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

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  config.parse(CONFIG, true, "dummy-config");

  validator.validate(*fibCommand,
                     bind(&CommandValidatorTester::onValidated, boost::ref(m_tester1), _1),
                     bind(&CommandValidatorTester::onValidationFailed, boost::ref(m_tester1), _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidationFailed());
  m_tester1.resetValidation();

  validator.validate(*statsCommand,
                     bind(&CommandValidatorTester::onValidated, boost::ref(m_tester1), _1),
                     bind(&CommandValidatorTester::onValidationFailed, boost::ref(m_tester1), _1, _2));

  BOOST_REQUIRE(m_tester1.commandValidationFailed());

  validator.validate(*facesCommand,
                     bind(&CommandValidatorTester::onValidated, boost::ref(m_tester2), _1),
                     bind(&CommandValidatorTester::onValidationFailed, boost::ref(m_tester2), _1, _2));

  BOOST_REQUIRE(m_tester2.commandValidationFailed());
  m_tester2.resetValidation();

  // use key2 for fib command (authorized for key1 only)
  shared_ptr<Interest> unauthorizedFibCommand = make_shared<Interest>("/localhost/nfd/fib/insert");
  generator.generateWithIdentity(*unauthorizedFibCommand, m_tester2.getIdentityName());

  validator.validate(*unauthorizedFibCommand,
                     bind(&CommandValidatorTester::onValidated, boost::ref(m_tester2), _1),
                     bind(&CommandValidatorTester::onValidationFailed, boost::ref(m_tester2), _1, _2));

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

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  BOOST_CHECK_THROW(config.parse(NO_AUTHORIZE_CONFIG, false, "dummy-config"), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(NoPrivilegesSections)
{
  const std::string NO_PRIVILEGES_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    keyfile \"tests/mgmt/key1.pub\"\n"
    "  }\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  BOOST_CHECK_THROW(config.parse(NO_PRIVILEGES_CONFIG, false, "dummy-config"), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(InvalidKeyFile)
{
  const std::string INVALID_KEY_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    keyfile \"tests/mgmt/notakeyfile.pub\"\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  BOOST_CHECK_THROW(config.parse(INVALID_KEY_CONFIG, false, "dummy-config"), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(NoKeyFile)
{
  const std::string NO_KEY_CONFIG =
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

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  BOOST_CHECK_THROW(config.parse(NO_KEY_CONFIG, false, "dummy-config"), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(MalformedKey)
{
    const std::string MALFORMED_KEY_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    keyfile \"tests/mgmt/malformedkey.pub\"\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "}\n";


  ConfigFile config;
  CommandValidator validator;

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  BOOST_CHECK_THROW(config.parse(MALFORMED_KEY_CONFIG, false, "dummy-config"), ConfigFile::Error);
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

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  BOOST_CHECK_EXCEPTION(config.parse(NO_AUTHORIZE_CONFIG, true, "dummy-config"),
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
    "    keyfile \"tests/mgmt/key1.pub\"\n"
    "  }\n"
    "  authorize\n"
    "  {\n"
    "    keyfile \"tests/mgmt/key2.pub\"\n"
    "  }\n"
    "}\n";

  // CommandValidatorTester tester1;
  // tester1.generateIdentity("/tests/CommandValidator/TwoKeys/id1");
  // tester1.saveIdentityToFile("tests/mgmt/key1.pub");

  // CommandValidatorTester tester2;
  // tester2.generateIdentity("/tests/CommandValidator/TwoKeys/id2");
  // tester2.saveIdentityToFile("tests/mgmt/key2.pub");

  ConfigFile config;
  CommandValidator validator;

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));

  std::stringstream expectedError;
  expectedError << "No privileges section found for key file tests/mgmt/key1.pub "
                << "(" << m_tester1.getPublicKeyName().toUri() << ")\n"
                << "No privileges section found for key file tests/mgmt/key2.pub "
                << "(" << m_tester2.getPublicKeyName().toUri() << ")";

  BOOST_CHECK_EXCEPTION(config.parse(NO_PRIVILEGES_CONFIG, true, "dummy-config"),
                        ConfigFile::Error,
                        bind(&validateErrorMessage, expectedError.str(), _1));
}

BOOST_AUTO_TEST_CASE(InvalidKeyFileDryRun)
{
  const std::string INVALID_KEY_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    keyfile \"tests/mgmt/notakeyfile.pub\"\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "  authorize\n"
    "  {\n"
    "    keyfile \"tests/mgmt/stillnotakeyfile.pub\"\n"
    "    privileges\n"
    "    {\n"
    "    }\n"
    "  }\n"
    "}\n";

  ConfigFile config;
  CommandValidator validator;

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));

  BOOST_CHECK_EXCEPTION(config.parse(INVALID_KEY_CONFIG, true, "dummy-config"),
                        ConfigFile::Error,
                        bind(&validateErrorMessage,
                             "Unable to open key file tests/mgmt/notakeyfile.pub\n"
                             "Unable to open key file tests/mgmt/stillnotakeyfile.pub", _1));
}

BOOST_AUTO_TEST_CASE(NoKeyFileDryRun)
{
  const std::string NO_KEY_CONFIG =
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

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  BOOST_CHECK_EXCEPTION(config.parse(NO_KEY_CONFIG, true, "dummy-config"),
                        ConfigFile::Error,
                        bind(&validateErrorMessage,
                             "No keyfile specified\n"
                             "No keyfile specified", _1));
}

BOOST_AUTO_TEST_CASE(MalformedKeyDryRun)
{
    const std::string MALFORMED_KEY_CONFIG =
    "authorizations\n"
    "{\n"
    "  authorize\n"
    "  {\n"
    "    keyfile \"tests/mgmt/malformedkey.pub\"\n"
    "    privileges\n"
    "    {\n"
    "      fib\n"
    "      stats\n"
    "    }\n"
    "  }\n"
    "  authorize\n"
    "  {\n"
    "    keyfile \"tests/mgmt/malformedkey.pub\"\n"
    "  }\n"
    "}\n";


  ConfigFile config;
  CommandValidator validator;

  config.addSectionHandler("authorizations",
                           bind(&CommandValidator::onConfig, boost::ref(validator), _1, _2));
  BOOST_CHECK_EXCEPTION(config.parse(MALFORMED_KEY_CONFIG, true, "dummy-config"),
                        ConfigFile::Error,
                        bind(&validateErrorMessage,
                             "Malformed key file tests/mgmt/malformedkey.pub\n"
                             "Malformed key file tests/mgmt/malformedkey.pub", _1));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests

} // namespace nfd

