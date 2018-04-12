/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "mgmt/command-authenticator.hpp"

#include "tests/test-common.hpp"
#include "tests/manager-common-fixture.hpp"

#include <boost/filesystem.hpp>

namespace nfd {
namespace tests {

class CommandAuthenticatorFixture : public CommandInterestSignerFixture
{
protected:
  CommandAuthenticatorFixture()
    : authenticator(CommandAuthenticator::create())
  {
  }

  void
  makeModules(std::initializer_list<std::string> modules)
  {
    for (const std::string& module : modules) {
      authorizations.emplace(module, authenticator->makeAuthorization(module, "verb"));
    }
  }

  void
  loadConfig(const std::string& config)
  {
    auto configPath = boost::filesystem::current_path() / "command-authenticator-test.conf";
    ConfigFile cf;
    authenticator->setConfigFile(cf);
    cf.parse(config, false, configPath.c_str());
  }

  bool
  authorize(const std::string& module, const Name& identity,
            const std::function<void(Interest&)>& modifyInterest = nullptr)
  {
    Interest interest = this->makeControlCommandRequest(Name("/prefix/" + module + "/verb"),
                                                        ControlParameters(), identity);
    if (modifyInterest != nullptr) {
      modifyInterest(interest);
    }

    ndn::mgmt::Authorization authorization = authorizations.at(module);

    bool isAccepted = false;
    bool isRejected = false;
    authorization(Name("/prefix"), interest, nullptr,
      [this, &isAccepted, &isRejected] (const std::string& requester) {
        BOOST_REQUIRE_MESSAGE(!isAccepted && !isRejected,
                              "authorization function should invoke only one continuation");
        isAccepted = true;
        lastRequester = requester;
      },
      [this, &isAccepted, &isRejected] (ndn::mgmt::RejectReply act) {
        BOOST_REQUIRE_MESSAGE(!isAccepted && !isRejected,
                              "authorization function should invoke only one continuation");
        isRejected = true;
        lastRejectReply = act;
      });

    this->advanceClocks(1_ms, 10);
    BOOST_REQUIRE_MESSAGE(isAccepted || isRejected,
                          "authorization function should invoke one continuation");
    return isAccepted;
  }

protected:
  shared_ptr<CommandAuthenticator> authenticator;
  std::unordered_map<std::string, ndn::mgmt::Authorization> authorizations;
  std::string lastRequester;
  ndn::mgmt::RejectReply lastRejectReply;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestCommandAuthenticator, CommandAuthenticatorFixture)

BOOST_AUTO_TEST_CASE(Certs)
{
  Name id0("/localhost/CommandAuthenticator/0");
  Name id1("/localhost/CommandAuthenticator/1");
  Name id2("/localhost/CommandAuthenticator/2");
  BOOST_REQUIRE(addIdentity(id0));
  BOOST_REQUIRE(saveIdentityCertificate(id1, "1.ndncert", true));
  BOOST_REQUIRE(saveIdentityCertificate(id2, "2.ndncert", true));

  makeModules({"module0", "module1", "module2", "module3", "module4", "module5", "module6", "module7"});
  const std::string& config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile any
        privileges
        {
          module1
          module3
          module5
          module7
        }
      }
      authorize
      {
        certfile "1.ndncert"
        privileges
        {
          module2
          module3
          module6
          module7
        }
      }
      authorize
      {
        certfile "2.ndncert"
        privileges
        {
          module4
          module5
          module6
          module7
        }
      }
    }
  )CONFIG";
  loadConfig(config);

  // module0: none
  BOOST_CHECK_EQUAL(authorize("module0", id0), false);
  BOOST_CHECK_EQUAL(authorize("module0", id1), false);
  BOOST_CHECK_EQUAL(authorize("module0", id2), false);

  // module1: any
  BOOST_CHECK_EQUAL(authorize("module1", id0), true);
  BOOST_CHECK_EQUAL(authorize("module1", id1), true);
  BOOST_CHECK_EQUAL(authorize("module1", id2), true);

  // module2: id1
  BOOST_CHECK_EQUAL(authorize("module2", id0), false);
  BOOST_CHECK_EQUAL(authorize("module2", id1), true);
  BOOST_CHECK_EQUAL(authorize("module2", id2), false);

  // module3: any,id1
  BOOST_CHECK_EQUAL(authorize("module3", id0), true);
  BOOST_CHECK_EQUAL(authorize("module3", id1), true);
  BOOST_CHECK_EQUAL(authorize("module3", id2), true);

  // module4: id2
  BOOST_CHECK_EQUAL(authorize("module4", id0), false);
  BOOST_CHECK_EQUAL(authorize("module4", id1), false);
  BOOST_CHECK_EQUAL(authorize("module4", id2), true);

  // module5: any,id2
  BOOST_CHECK_EQUAL(authorize("module5", id0), true);
  BOOST_CHECK_EQUAL(authorize("module5", id1), true);
  BOOST_CHECK_EQUAL(authorize("module5", id2), true);

  // module6: id1,id2
  BOOST_CHECK_EQUAL(authorize("module6", id0), false);
  BOOST_CHECK_EQUAL(authorize("module6", id1), true);
  BOOST_CHECK_EQUAL(authorize("module6", id2), true);

  // module7: any,id1,id2
  BOOST_CHECK_EQUAL(authorize("module7", id0), true);
  BOOST_CHECK_EQUAL(authorize("module7", id1), true);
  BOOST_CHECK_EQUAL(authorize("module7", id2), true);
}

BOOST_AUTO_TEST_CASE(Requester)
{
  Name id0("/localhost/CommandAuthenticator/0");
  Name id1("/localhost/CommandAuthenticator/1");
  BOOST_REQUIRE(addIdentity(id0));
  BOOST_REQUIRE(saveIdentityCertificate(id1, "1.ndncert", true));

  makeModules({"module0", "module1"});
  const std::string& config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile any
        privileges
        {
          module0
        }
      }
      authorize
      {
        certfile "1.ndncert"
        privileges
        {
          module1
        }
      }
    }
  )CONFIG";
  loadConfig(config);

  // module0: any
  BOOST_CHECK_EQUAL(authorize("module0", id0), true);
  BOOST_CHECK_EQUAL(lastRequester, "*");
  BOOST_CHECK_EQUAL(authorize("module0", id1), true);
  BOOST_CHECK_EQUAL(lastRequester, "*");

  // module1: id1
  BOOST_CHECK_EQUAL(authorize("module1", id0), false);
  BOOST_CHECK_EQUAL(authorize("module1", id1), true);
  BOOST_CHECK(id1.isPrefixOf(lastRequester));
}

class IdentityAuthorizedFixture : public CommandAuthenticatorFixture
{
protected:
  IdentityAuthorizedFixture()
    : id1("/localhost/CommandAuthenticator/1")
  {
    BOOST_REQUIRE(saveIdentityCertificate(id1, "1.ndncert", true));

    makeModules({"module1"});
    const std::string& config = R"CONFIG(
      authorizations
      {
        authorize
        {
          certfile "1.ndncert"
          privileges
          {
            module1
          }
        }
      }
    )CONFIG";
    loadConfig(config);
  }

  bool
  authorize1(const std::function<void(Interest&)>& modifyInterest)
  {
    return authorize("module1", id1, modifyInterest);
  }

protected:
  Name id1;
};

BOOST_FIXTURE_TEST_SUITE(Rejects, IdentityAuthorizedFixture)

BOOST_AUTO_TEST_CASE(BadKeyLocator_NameTooShort)
{
  BOOST_CHECK_EQUAL(authorize1(
    [] (Interest& interest) {
      interest.setName("/prefix");
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);
}

BOOST_AUTO_TEST_CASE(BadKeyLocator_BadSigInfo)
{
  BOOST_CHECK_EQUAL(authorize1(
    [] (Interest& interest) {
      setNameComponent(interest, ndn::signed_interest::POS_SIG_INFO, "not-SignatureInfo");
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);
}

BOOST_AUTO_TEST_CASE(BadKeyLocator_MissingKeyLocator)
{
  BOOST_CHECK_EQUAL(authorize1(
    [] (Interest& interest) {
      ndn::SignatureInfo sigInfo(tlv::SignatureSha256WithRsa);
      setNameComponent(interest, ndn::signed_interest::POS_SIG_INFO,
                       sigInfo.wireEncode().begin(), sigInfo.wireEncode().end());
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);
}

BOOST_AUTO_TEST_CASE(BadKeyLocator_BadKeyLocatorType)
{
  BOOST_CHECK_EQUAL(authorize1(
    [] (Interest& interest) {
      ndn::KeyLocator kl;
      kl.setKeyDigest(ndn::encoding::makeBinaryBlock(tlv::KeyDigest, "\xDD\xDD\xDD\xDD\xDD\xDD\xDD\xDD", 8));
      ndn::SignatureInfo sigInfo(tlv::SignatureSha256WithRsa);
      sigInfo.setKeyLocator(kl);
      setNameComponent(interest, ndn::signed_interest::POS_SIG_INFO,
                       sigInfo.wireEncode().begin(), sigInfo.wireEncode().end());
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::SILENT);
}

BOOST_AUTO_TEST_CASE(NotAuthorized)
{
  Name id0("/localhost/CommandAuthenticator/0");
  BOOST_REQUIRE(addIdentity(id0));

  BOOST_CHECK_EQUAL(authorize("module1", id0), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_AUTO_TEST_CASE(BadSig)
{
  BOOST_CHECK_EQUAL(authorize1(
    [] (Interest& interest) {
      setNameComponent(interest, ndn::command_interest::POS_SIG_VALUE, "bad-signature-bits");
    }
  ), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_AUTO_TEST_CASE(InvalidTimestamp)
{
  name::Component timestampComp;
  BOOST_CHECK_EQUAL(authorize1(
    [&timestampComp] (const Interest& interest) {
      timestampComp = interest.getName().at(ndn::command_interest::POS_TIMESTAMP);
    }
  ), true); // accept first command
  BOOST_CHECK_EQUAL(authorize1(
    [&timestampComp] (Interest& interest) {
      setNameComponent(interest, ndn::command_interest::POS_TIMESTAMP, timestampComp);
    }
  ), false); // reject second command because timestamp equals first command
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_FIXTURE_TEST_CASE(MissingAuthorizationsSection, CommandAuthenticatorFixture)
{
  Name id0("/localhost/CommandAuthenticator/0");
  BOOST_REQUIRE(addIdentity(id0));

  makeModules({"module42"});
  loadConfig("");

  BOOST_CHECK_EQUAL(authorize("module42", id0), false);
  BOOST_CHECK(lastRejectReply == ndn::mgmt::RejectReply::STATUS403);
}

BOOST_AUTO_TEST_SUITE_END() // Rejects

BOOST_AUTO_TEST_SUITE(BadConfig)

BOOST_AUTO_TEST_CASE(EmptyAuthorizationsSection)
{
  const std::string& config = R"CONFIG(
    authorizations
    {
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(UnrecognizedKey)
{
  const std::string& config = R"CONFIG(
    authorizations
    {
      unrecognized_key
      {
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(CertfileMissing)
{
  const std::string& config = R"CONFIG(
    authorizations
    {
      authorize
      {
        privileges
        {
        }
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(CertUnreadable)
{
  const std::string& config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile "1.ndncert"
        privileges
        {
        }
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(PrivilegesMissing)
{
  const std::string& config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile any
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(UnregisteredModule)
{
  const std::string& config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile any
        privileges
        {
          nosuchmodule
        }
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(loadConfig(config), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // BadConfig

BOOST_AUTO_TEST_SUITE_END() // TestCommandAuthenticator
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
